/*
	The agent's hands and pistol from the game itself (hotd2-vr), or the hero's staff. See
	xr_hands.h.

	hands.bin in the data directory (little endian, made from the player's own game by
	hands_rip.cpp, or on the PC by hotd2-vr/assets/rip_hands.py), and in the same format the
	hero's staff of The Maze of the Kings, staff-mok.bin, and Confidential Mission's guns,
	gun-cm-*.bin (only the running game's files are read, again when one is made or another game
	starts; Confidential Mission's guns each on first use):
		"HND1", u32 version (1, or 2 with the two u32s below)
		f32 muzzle[3], grab[3], travel, onSlide[16] (column major)
		version 2: u32 flags (1 gun only: no hand meshes, travel 0; 2 complete: all of the
			gun was in the game's frame), u32 sourcePolys (the game's polygons it was made from)
		u32 textures, each: u32 width, height, flags (1 clamp u, 2 clamp v, 4 mirror u,
			8 mirror v, 16 alpha test), then width * height RGBA bytes
		u32 meshes (frame, slide, gun hand, open hand), each: u32 parts, each: u32 texture,
			u32 vertices, then the vertices: f32 position[3], normal[3], uv[2], u8 rgba[4]
	A model without a slide (the staff, a gun only) has travel 0 and no parts in the slide
	mesh; a gun only none in the hands' either (the other hand keeps its controller).

	Lit like the arcade gun (xr_gun.cpp): a fixed key light and a dim fill, so the hands
	read as solid in the game's dark scenes, rather than with the light baked in that one
	frame of the game.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_hands.h"
#include "hands_rip.h"
#include "rend/vr_reproject.h"
#include "rend/gles/gles.h"
#include "rend/gles/glcache.h"
#include "cfg/option.h"
#include "log/Log.h"
#include "stdclass.h"
#include "types.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <glm/gtc/type_ptr.hpp>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace vr::xr
{
namespace
{

void logGlErrors()
{
	// (hotd2-vr: the first few seconds only, at most 10 lines: checked after every draw it
	// costs a GPU round trip and filled the log at headset rate)
	static int checks, logged;
	if (checks >= 1000 || logged >= 10)
		return;
	checks++;
	for (GLenum error; (error = glGetError()) != GL_NO_ERROR && logged < 10; logged++)
		ERROR_LOG(RENDERER, "VR hands OpenGL error 0x%04x", error);
}

enum { MeshFrame, MeshSlide, MeshGunHand, MeshOpenHand, MeshCount };

struct Texture
{
	u32 width, height, flags;
	std::vector<u8> rgba;
	GLuint id = 0;
};
struct Part
{
	u32 texture;
	u32 first, count;	// vertices
};
constexpr size_t VertexSize = 36;

struct Model
{
	HandsModel info;
	std::vector<Texture> textures;
	std::vector<Part> meshes[MeshCount];
	std::vector<u8> vertices;
	bool staff = false;	// the hero's staff (its fist off unless vr.StaffHand)
	u32 flags = 0;		// HND1 v2's (2: complete)
	GLuint vbo = 0, vao = 0;	// (and the textures' ids) once drawn
};
// The running game's models: its own (handsModel()), and Confidential Mission's guns, each in
// the slot of its GunKind (the arcade gun's, 0, is the game's own model's).
struct Slot
{
	Model model;
	bool tried = false, loaded = false;
	bool missing = false;	// no file (else unreadable)
};
constexpr int GameSlot = 0, Slots = 4;
Slot slots[Slots];
std::string gunLogged;	// chosenGun()'s line, as last logged
bool rifleHeld[2];		// each player's hand holds the assault rifle (handGun()), as last logged

GLuint program;
GLint uMvp = -1, uModel = -1, uEye = -1, uSpecular = -1, uShininess = -1, uAlphaTest = -1, uLight = -1, uTex = -1;

// A model's file into `model`; false without one it can read (`missing`: no file at all).
bool load(Model& model, std::string path, const std::string& legacy, bool& missing)
{
	const std::string first = path;
	// where older builds had it: the app's internal files
	FILE *f = nowide::fopen(path.c_str(), "rb");
	if (f == nullptr && !legacy.empty())
	{
		path = legacy;
		f = nowide::fopen(path.c_str(), "rb");
	}
	missing = f == nullptr;
	if (f == nullptr)
	{
		INFO_LOG(RENDERER, "XR: no %s, the arcade gun it is", first.c_str());
		return false;
	}
	bool ok = true;
	auto read = [&](void *to, size_t size) {
		ok = ok && fread(to, 1, size, f) == size;
		return ok;
	};
	auto u32At = [&]() {
		u32 v = 0;
		read(&v, 4);
		return v;
	};
	char magic[4] {};
	read(magic, 4);
	const u32 version = u32At();
	if (!ok || memcmp(magic, "HND1", 4) != 0 || (version != 1 && version != 2))
	{
		fclose(f);
		WARN_LOG(RENDERER, "XR: %s is not a hands model this build reads", path.c_str());
		return false;
	}
	float head[3 + 3 + 1 + 16];
	read(head, sizeof(head));
	model.info.muzzle = glm::make_vec3(&head[0]);
	model.info.grab = glm::make_vec3(&head[3]);
	model.info.travel = head[6];
	model.info.onSlide = glm::make_mat4(&head[7]);
	u32 flags = 0, sourcePolys = 0;
	if (version >= 2)
	{
		flags = u32At();
		sourcePolys = u32At();
	}
	model.info.gunOnly = (flags & 1) != 0;
	model.flags = flags;
	const u32 textures = u32At();
	for (u32 i = 0; ok && i < textures && i < 64; i++)
	{
		Texture t;
		t.width = u32At();
		t.height = u32At();
		t.flags = u32At();
		if (!ok || t.width == 0 || t.height == 0 || t.width > 1024 || t.height > 1024)
		{
			ok = false;
			break;
		}
		t.rgba.resize((size_t)t.width * t.height * 4);
		read(t.rgba.data(), t.rgba.size());
		model.textures.push_back(std::move(t));
	}
	const u32 meshes = u32At();
	for (u32 m = 0; ok && m < meshes && m < MeshCount; m++)
	{
		const u32 parts = u32At();
		for (u32 p = 0; ok && p < parts && p < 64; p++)
		{
			Part part;
			part.texture = u32At();
			part.count = u32At();
			part.first = (u32)(model.vertices.size() / VertexSize);
			if (!ok || part.texture >= model.textures.size() || part.count > 65536)
			{
				ok = false;
				break;
			}
			const size_t at = model.vertices.size();
			model.vertices.resize(at + part.count * VertexSize);
			read(&model.vertices[at], part.count * VertexSize);
			model.meshes[m].push_back(part);
		}
	}
	fclose(f);
	if (!ok || meshes < MeshCount)
	{
		WARN_LOG(RENDERER, "XR: %s is cut short or damaged", path.c_str());
		model = Model();
		return false;
	}
	const hands::Source *source = gameModelSource();
	model.staff = source != nullptr && source->kind == hands::Source::HerosStaff;
	NOTICE_LOG(RENDERER, "XR: the game's own model from %s (%zu textures, %zu vertices%s%s)", path.c_str(),
			model.textures.size(), model.vertices.size() / VertexSize, model.info.travel > 0.f ? "" : ", no slide",
			!model.info.gunOnly ? "" : (flags & 2) ? ", gun only" : ", gun only, not all of it");
	if (version >= 2)
		INFO_LOG(RENDERER, "XR: HND1 version %u, flags %u, from %u of the game's polygons", version, flags, sourcePolys);
	return true;
}

bool initGl()
{
	if (program != 0)
		return true;
	OpenGlSource vertex;
	vertex.addSource(VertexCompatShader).addSource(R"(
in highp vec3 in_pos;
in highp vec3 in_normal;
in highp vec2 in_uv;
in lowp vec4 in_base;
uniform highp mat4 mvp;
uniform highp mat4 model;
out highp vec3 vtx_pos;
out highp vec3 vtx_normal;
out highp vec2 vtx_uv;
out lowp vec4 vtx_color;
void main()
{
	vtx_pos = (model * vec4(in_pos, 1.0)).xyz;
	vtx_normal = mat3(model) * in_normal;
	vtx_uv = in_uv;
	vtx_color = in_base;
	gl_Position = mvp * vec4(in_pos, 1.0);
}
)");
	OpenGlSource fragment;
	fragment.addSource(PixelCompatShader).addSource(R"(
uniform sampler2D tex;
uniform highp vec3 eyePos;
uniform mediump float specular;
uniform mediump float shininess;
uniform lowp float alphaTest;
uniform highp vec4 muzzleLight;	// room position, strength
in highp vec3 vtx_pos;
in highp vec3 vtx_normal;
in highp vec2 vtx_uv;
in lowp vec4 vtx_color;
void main()
{
	lowp vec4 base = texture(tex, vtx_uv) * vtx_color;
	if (base.a < alphaTest)
		discard;
	highp vec3 n = normalize(vtx_normal);
	highp vec3 v = normalize(eyePos - vtx_pos);
	if (dot(n, v) < 0.0)
		n = -n;		// a mirrored model, or inside faces seen through gaps
	const highp vec3 key = vec3(0.32, 0.86, 0.40);
	const highp vec3 fill = vec3(-0.45, -0.75, -0.48);
	mediump float diffuse = max(dot(n, key), 0.0);
	mediump float back = max(dot(n, fill), 0.0);
	highp vec3 h = normalize(key + v);
	mediump float highlight = pow(max(dot(n, h), 0.0), shininess) * specular;
	highp float edge = 1.0 - clamp(dot(n, v), 0.0, 1.0);
	mediump vec3 color = base.rgb * (0.32 + 0.72 * diffuse + 0.22 * back + 0.12 * edge * edge)
			+ vec3(highlight);
	if (muzzleLight.w > 0.0)
	{
		highp vec3 toFlash = muzzleLight.xyz - vtx_pos;
		highp float fd = length(toFlash);
		mediump float lit = muzzleLight.w * max(dot(n, toFlash / fd), 0.15) / (1.0 + fd * fd * 600.0);
		color += (base.rgb * 2.2 + 0.2) * vec3(1.0, 0.62, 0.28) * lit;
	}
	gl_FragColor = vec4(color, 1.0);
}
)");
	program = gl_CompileAndLink(vertex.generate().c_str(), fragment.generate().c_str());
	if (program == 0)
		return false;
	uMvp = glGetUniformLocation(program, "mvp");
	uModel = glGetUniformLocation(program, "model");
	uEye = glGetUniformLocation(program, "eyePos");
	uSpecular = glGetUniformLocation(program, "specular");
	uShininess = glGetUniformLocation(program, "shininess");
	uAlphaTest = glGetUniformLocation(program, "alphaTest");
	uLight = glGetUniformLocation(program, "muzzleLight");
	uTex = glGetUniformLocation(program, "tex");
	return true;
}

// A model's vertices and textures to the GPU, on its first draw.
void upload(Model& model)
{
	if (model.vao != 0)
		return;
	model.vao = createVertexArray();
	bindVertexArray(model.vao);
	glGenBuffers(1, &model.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, model.vbo);
	glBufferData(GL_ARRAY_BUFFER, model.vertices.size(), model.vertices.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(VERTEX_POS_ARRAY, 3, GL_FLOAT, GL_FALSE, VertexSize, (const void *)0);
	glVertexAttribPointer(VERTEX_NORM_ARRAY, 3, GL_FLOAT, GL_FALSE, VertexSize, (const void *)12);
	glVertexAttribPointer(VERTEX_UV_ARRAY, 2, GL_FLOAT, GL_FALSE, VertexSize, (const void *)24);
	glVertexAttribPointer(VERTEX_COL_BASE_ARRAY, 4, GL_UNSIGNED_BYTE, GL_TRUE, VertexSize, (const void *)32);
	glEnableVertexAttribArray(VERTEX_POS_ARRAY);
	glEnableVertexAttribArray(VERTEX_NORM_ARRAY);
	glEnableVertexAttribArray(VERTEX_UV_ARRAY);
	glEnableVertexAttribArray(VERTEX_COL_BASE_ARRAY);
	bindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glActiveTexture(GL_TEXTURE0);
	for (Texture& t : model.textures)
	{
		glGenTextures(1, &t.id);
		glcache.BindTexture(GL_TEXTURE_2D, t.id);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t.width, t.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, t.rgba.data());
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, t.flags & 1 ? GL_CLAMP_TO_EDGE : t.flags & 4 ? GL_MIRRORED_REPEAT : GL_REPEAT);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, t.flags & 2 ? GL_CLAMP_TO_EDGE : t.flags & 8 ? GL_MIRRORED_REPEAT : GL_REPEAT);
	}
	glcache.BindTexture(GL_TEXTURE_2D, 0);
}

void drawMesh(const Model& model, int mesh, const glm::mat4& viewProj, const glm::mat4& pose, float specular, float shininess)
{
	const glm::mat4 mvp = viewProj * pose;
	glUniformMatrix4fv(uMvp, 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(uModel, 1, GL_FALSE, &pose[0][0]);
	glUniform1f(uSpecular, specular);
	glUniform1f(uShininess, shininess);
	for (const Part& part : model.meshes[mesh])
	{
		const Texture& t = model.textures[part.texture];
		glcache.BindTexture(GL_TEXTURE_2D, t.id);
		glUniform1f(uAlphaTest, t.flags & 16 ? 0.5f : -1.f);
		glDrawArrays(GL_TRIANGLES, (GLint)part.first, (GLsizei)part.count);
	}
}

// A model just made, or another game: every model again, and the gun line.
void refresh()
{
	if (!hands::takeNewModel())
		return;
	termHands();
	for (Slot& slot : slots)
		slot = Slot();
	gunLogged.clear();
	rifleHeld[0] = rifleHeld[1] = false;
}

bool cmGuns()
{
	const hands::Source *source = gameModelSource();
	return source != nullptr && source->kind == hands::Source::CmGuns;
}

}	// namespace

const HandsModel *handsModel()
{
	refresh();
	if (cmGuns())
		return gunModel(chosenGun());
	Slot& slot = slots[GameSlot];
	if (!slot.tried)
	{
		slot.tried = true;
		const std::string path = hands::modelPath();
		if (path.empty())
			INFO_LOG(RENDERER, "XR: no model of its own for this game, the arcade gun it is");
		else
			slot.loaded = load(slot.model, path, hands::legacyModelPath(), slot.missing);
	}
	return slot.loaded ? &slot.model.info : nullptr;
}

const HandsModel *gunModel(GunKind kind)
{
	refresh();
	if (kind == GunKind::Arcade || !cmGuns())
		return nullptr;
	Slot& slot = slots[(int)kind];
	if (!slot.tried)
	{
		slot.tried = true;
		// (GunKind's order is the profile's guns[], the cabinet gun first)
		const hands::GunTarget& gun = gameModelSource()->guns[(int)kind - 1];
		slot.missing = true;
		if (gun.used())
			slot.loaded = load(slot.model, get_writable_data_path(gun.file), std::string(), slot.missing);
	}
	return slot.loaded ? &slot.model.info : nullptr;
}

bool gunComplete(GunKind kind)
{
	return kind == GunKind::Arcade || (gunModel(kind) != nullptr && (slots[(int)kind].model.flags & 2) != 0);
}

const char *gunName(GunKind kind)
{
	switch (kind)
	{
	case GunKind::Cabinet:
		return "cabinet gun";
	case GunKind::Pistol:
		return "agents' pistol";
	case GunKind::Rifle:
		return "assault rifle";
	default:
		return "arcade gun";
	}
}

GunKind chosenGun()
{
	refresh();
	if (!cmGuns())
		return GunKind::Arcade;	// (another game's own model takes no choosing: handModel())
	// the assault rifle's model too, read at the start with the other guns (drawHands() puts it on
	// the GPU with them): at the pickup, mid-play, it could cost the headset a frame (Ruling S33)
	if (config::VrGameHands)
		gunModel(GunKind::Rifle);
	// (a setting this build doesn't know is Auto, as the menu's row shows it)
	int setting = config::VrGun;
	if (setting < 0 || setting > 3)
		setting = 0;
	const GunKind wanted = setting == 1 ? GunKind::Pistol : setting == 2 ? GunKind::Cabinet : GunKind::Arcade;
	GunKind kind = GunKind::Arcade;
	std::string why;	// each gun passed over, and why
	auto passOver = [&](GunKind gun, const char *how) {
		why += std::string(why.empty() ? "" : ", ") + "the " + gunName(gun) + how;
	};
	auto notThere = [&](GunKind gun) { passOver(gun, slots[(int)gun].missing ? " is not made yet" : " can't be read"); };
	if (setting == 3)
		kind = GunKind::Arcade;
	else if (!config::VrGameHands)
		why = "the game's own models are off";
	else if (setting != 0 && gunModel(wanted) != nullptr)
		kind = wanted;
	else
	{
		// auto, or the one chosen isn't there: the agents' pistol if complete, else the cabinet
		// gun if there, else the arcade gun
		if (setting != 0)
			notThere(wanted);
		if (wanted != GunKind::Pistol && gunModel(GunKind::Pistol) != nullptr && gunComplete(GunKind::Pistol))
			kind = GunKind::Pistol;
		else
		{
			if (wanted != GunKind::Pistol)
			{
				if (gunModel(GunKind::Pistol) != nullptr)
					passOver(GunKind::Pistol, " is incomplete");
				else
					notThere(GunKind::Pistol);
			}
			if (wanted != GunKind::Cabinet && gunModel(GunKind::Cabinet) != nullptr)
				kind = GunKind::Cabinet;
			else if (wanted != GunKind::Cabinet)
				notThere(GunKind::Cabinet);
		}
	}
	const std::string line = std::string(gunName(kind)) + " (" + (setting == 0 ? "auto" : "chosen")
			+ (why.empty() ? "" : ": " + why) + ")";
	if (line != gunLogged)
	{
		gunLogged = line;
		NOTICE_LOG(RENDERER, "XR: gun: %s", line.c_str());
	}
	return kind;
}

GunKind handGun(int player)
{
	if (player < 0 || player > 1 || !cmGuns())
		return chosenGun();
	// (not read: the emulator stopped, the VR menu open: the hand keeps what it holds)
	vr::GunState gun;
	if (vr::gunState(player, gun) && gun.valid && gun.rifle != rifleHeld[player])
	{
		rifleHeld[player] = gun.rifle;
		if (gun.rifle)
			NOTICE_LOG(RENDERER, "XR: P%d hand: assault rifle (%s)", player + 1, handModel(GunKind::Rifle) != nullptr
					? gameModelSource()->guns[(int)GunKind::Rifle - 1].file : "arcade gun, recoloured");
		else
			NOTICE_LOG(RENDERER, "XR: P%d hand: back to the %s", player + 1, gunName(chosenGun()));
	}
	return rifleHeld[player] ? GunKind::Rifle : chosenGun();
}

const HandsModel *handModel(GunKind kind)
{
	if (!config::VrGameHands)
		return nullptr;
	return cmGuns() ? gunModel(kind) : handsModel();
}

std::vector<GunChoice> gunChoices()
{
	// another game's own model takes no choosing: one choice, the row locked (vr.Gun kept)
	if (!cmGuns())
		return { { (int)config::VrGun, config::VrGameHands && handsModel() != nullptr ? "The game's own" : "Arcade gun", true, true } };
	// (with the game's own models off, the hand holds the arcade gun: theirs aren't offered)
	const bool own = config::VrGameHands;
	return {
		{ 0, "Auto", true, true },
		{ 1, "Agents' pistol", own && gunModel(GunKind::Pistol) != nullptr, gunComplete(GunKind::Pistol) },
		{ 2, "Cabinet gun", own && gunModel(GunKind::Cabinet) != nullptr, gunComplete(GunKind::Cabinet) },
		{ 3, "Arcade gun", true, true },
	};
}

void drawHands(const glm::mat4& viewProj, const glm::vec3& eyePos, const HandsView& view)
{
	Model *found = nullptr;
	for (Slot& slot : slots)
		if (slot.loaded && view.model == &slot.model.info)
			found = &slot.model;
	if (found == nullptr || !initGl())
		return;
	// every model loaded goes to the GPU at the first draw, not just this one: the assault
	// rifle's at its pickup mid-play could cost a frame (Ruling S33)
	for (Slot& slot : slots)
		if (slot.loaded)
			upload(slot.model);
	Model& model = *found;
	glcache.UseProgram(program);
	glUniform3f(uEye, eyePos.x, eyePos.y, eyePos.z);
	glUniform4f(uLight, view.muzzleLight.x, view.muzzleLight.y, view.muzzleLight.z, view.muzzleLight.w);
	glUniform1i(uTex, 0);
	glActiveTexture(GL_TEXTURE0);
	bindVertexArray(model.vao);

	// metal with a sheen, skin and cotton matt
	drawMesh(model, MeshFrame, viewProj, view.gunPose, 0.35f, 40.f);
	drawMesh(model, MeshSlide, viewProj, glm::translate(view.gunPose, glm::vec3(0.f, 0.f, view.slide)), 0.45f, 50.f);
	// The hero's gloves came out mirrored in the headset (the other hand a right glove on
	// the left controller: the one around his staff seems to be his left), so both are
	// drawn mirrored in x about their own origin; the staff itself stays as it is.
	const glm::mat4 gloves = model.staff ? glm::scale(glm::mat4(1.f), glm::vec3(-1.f, 1.f, 1.f)) : glm::mat4(1.f);
	// (the hero's fist sits where the player's own hand is, but made for a staff held
	// upright: it looked better left out)
	if (!model.staff || config::VrStaffHand)
		drawMesh(model, MeshGunHand, viewProj, view.gunPose * gloves, 0.06f, 10.f);
	if (view.otherHand)
		drawMesh(model, MeshOpenHand, viewProj, view.handPose * gloves, 0.06f, 10.f);

	bindVertexArray(0);
	glcache.BindTexture(GL_TEXTURE_2D, 0);
	logGlErrors();
}

void termHands()
{
	if (program != 0)
		glcache.DeleteProgram(program);
	program = 0;
	for (Slot& slot : slots)
	{
		Model& model = slot.model;
		if (model.vbo != 0)
			glDeleteBuffers(1, &model.vbo);
		if (model.vao != 0)
			deleteVertexArray(model.vao);
		model.vbo = model.vao = 0;
		for (Texture& t : model.textures)
			if (t.id != 0)
			{
				glcache.DeleteTextures(1, &t.id);
				t.id = 0;
			}
	}
	uMvp = uModel = uEye = uSpecular = uShininess = uAlphaTest = uLight = uTex = -1;
}

}
