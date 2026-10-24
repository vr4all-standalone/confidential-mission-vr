/*
	The player's controllers drawn in the hands (hotd2-vr). See xr_controllers.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_controllers.h"
#include "xr_gun.h"
#include "xr_sim.h"
#include "glb_mesh.h"
#include "xr_own.h"
#include "controller_model.h"
#include "rend/gles/gles.h"
#include "rend/gles/glcache.h"
#include "log/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cfloat>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace vr::xr::controllers
{
namespace
{

constexpr int Left = 0, Right = 1;
const char *const HandName[2] { "left", "right" };
const char *const ModelPath[2] { "/model_fb/controller/left", "/model_fb/controller/right" };

// Grip space <- the runtime's model space. Identity until the headset shows otherwise
// (the 1.1 headset checklist, Task 8: the Quest's own controllers sit right in the hands).
const glm::mat4 kRuntimeModelOffset { 1.f };

struct Material
{
	glm::vec3 color;
	float specular;		// highlight strength
	float shininess;	// highlight tightness
};
// the built-in shape's parts, by name: light grey plastic, a dark face plate and controls
struct PartMaterial { const char *name; Material material; };
const PartMaterial Materials[] = {
	{ "body", { { 0.72f, 0.73f, 0.75f }, 0.30f, 30.f } },
	{ "face", { { 0.09f, 0.09f, 0.10f }, 0.45f, 50.f } },
	{ "a", { { 0.26f, 0.26f, 0.28f }, 0.50f, 60.f } },
	{ "b", { { 0.26f, 0.26f, 0.28f }, 0.50f, 60.f } },
	{ "menu", { { 0.26f, 0.26f, 0.28f }, 0.50f, 60.f } },
	{ "stick", { { 0.06f, 0.06f, 0.07f }, 0.40f, 40.f } },
	{ "trigger", { { 0.40f, 0.41f, 0.43f }, 0.35f, 40.f } },
	{ "grip", { { 0.40f, 0.41f, 0.43f }, 0.35f, 40.f } },
};
// the runtime's model: its own texture times white; untextured (a Quest's textures are KTX2,
// not decoded here), a light grey close to a Quest 3 controller's
const Material Textured { { 1.f, 1.f, 1.f }, 0.25f, 30.f };
const Material Untextured { { 0.80f, 0.80f, 0.82f }, 0.25f, 30.f };

// A model: interleaved position, normal, uv (8 floats a vertex), 16- or 32-bit indices, parts
// (ranges of the indices, each with its material and base colour texture, if any: the runtime's
// model at most one, the player's own one a part). Kept on the CPU until the first draw puts it on
// the GPU.
struct Part { unsigned first, count; Material material; int texture = -1; };	// (an index into textures; -1: none)
struct Model
{
	std::vector<float> verts;
	std::vector<u32> indices;
	std::vector<Part> parts;
	std::vector<GlbImage> images;	// (until uploaded)
	unsigned triangles = 0;

	GLuint vao = 0, vbo = 0, ibo = 0;
	std::vector<GLuint> textures;	// the images', once uploaded
	GLenum indexType = GL_UNSIGNED_SHORT;
	bool uploaded = false;
	bool failed = false;
	size_t primitives = 0;	// the GLB's (the player's own: merged into parts by colour and image)
};

Model builtIn;
Model runtime[2];
// The player's own (xr_own.h): controllers/left.glb, right.glb, drawn on the aim pose (GLB space ->
// aim space: ownToAim) instead of the runtime's or the built-in one; read again when the folder
// changes (own::generation()).
Model own[2];
glm::mat4 ownToAim[2];
bool ownLoaded[2];
// its sidecar's "buttons" points (GLB space) that passed, by Button; the sidecar as logged
glm::vec3 ownButton[2][Buttons];
bool ownHasButton[2][Buttons];
std::string ownSidecarShown[2];
static_assert(sizeof(own::ButtonNames) / sizeof(own::ButtonNames[0]) == Buttons, "a sidecar name for each Button");
unsigned ownGeneration;		// own::controllersGeneration() they were read at
std::string ownKey[2];		// each hand's file and its stamp as read: only a hand whose files changed is read again

// Asking the runtime for its models: init, then retry() for each hand still built in.
struct Ask
{
	XrSession session = XR_NULL_HANDLE;
	// XR_FB_render_model's functions (null: the runtime has none)
	PFN_xrEnumerateRenderModelPathsFB enumeratePaths = nullptr;
	PFN_xrGetRenderModelPropertiesFB getProperties = nullptr;
	PFN_xrLoadRenderModelFB loadModel = nullptr;
	XrPath path[2] { XR_NULL_PATH, XR_NULL_PATH };
	bool listed[2] {};		// the runtime lists that hand's path
	bool loaded[2] {};		// that hand's model is the runtime's (runtime[hand])
	bool transient[2] {};	// that hand's last failure may pass: worth asking again
	std::string why[2];		// why that hand is built in
	std::string loggedWhy;	// the "built in" reason last logged
};
Ask ask;

GLuint program;
GLint uMvp = -1, uModel = -1, uEye = -1, uColor = -1, uSpecular = -1, uShininess = -1, uTextured = -1, uTex = -1;

void makeBuiltIn(Model& m)
{
	using namespace controllermodel;
	m = Model();
	m.verts.resize(VertexCount * 8);
	for (unsigned i = 0; i < VertexCount; i++)
		for (int k = 0; k < 3; k++)
		{
			m.verts[i * 8 + k] = Positions[i * 3 + k] * PositionScale;
			m.verts[i * 8 + 3 + k] = Normals[i * 3 + k] / 127.f;
		}
	m.indices.assign(std::begin(Indices), std::end(Indices));
	for (const controllermodel::Part& p : Parts)
	{
		Material material = Materials[0].material;
		for (const PartMaterial& pm : Materials)
			if (!strcmp(pm.name, p.name))
				material = pm.material;
		m.parts.push_back({ p.first, p.count, material });
	}
	m.triangles = TriangleCount;
}

// perPart (the player's own): a part per primitive, its base colour factor and image; else one part,
// the runtime's texture times white, or untextured grey
void fromGlb(Model& m, GlbMesh&& mesh, bool perPart = false)
{
	m = Model();
	const size_t vertexCount = mesh.positions.size() / 3;
	m.verts.resize(vertexCount * 8);
	for (size_t i = 0; i < vertexCount; i++)
	{
		for (int k = 0; k < 3; k++)
		{
			m.verts[i * 8 + k] = mesh.positions[i * 3 + k];
			m.verts[i * 8 + 3 + k] = mesh.normals[i * 3 + k];
		}
		m.verts[i * 8 + 6] = mesh.uvs[i * 2];
		m.verts[i * 8 + 7] = mesh.uvs[i * 2 + 1];
	}
	m.indices = std::move(mesh.indices);
	m.images = std::move(mesh.images);
	m.primitives = mesh.parts.size();
	if (perPart)
	{
		// the primitives that share a colour and an image drawn as one part (their indices put together)
		std::vector<u32> indices;
		indices.reserve(m.indices.size());
		std::vector<bool> done(mesh.parts.size());
		for (size_t i = 0; i < mesh.parts.size(); i++)
		{
			if (done[i])
				continue;
			const GlbPart& p = mesh.parts[i];
			const unsigned first = (unsigned)indices.size();
			for (size_t j = i; j < mesh.parts.size(); j++)
			{
				const GlbPart& q = mesh.parts[j];
				if (done[j] || q.image != p.image || memcmp(q.color, p.color, 3 * sizeof(float)) != 0)
					continue;
				done[j] = true;
				indices.insert(indices.end(), m.indices.begin() + q.firstIndex, m.indices.begin() + q.firstIndex + q.indexCount);
			}
			m.parts.push_back({ first, (unsigned)indices.size() - first, { { p.color[0], p.color[1], p.color[2] }, 0.25f, 30.f }, p.image });
		}
		m.indices = std::move(indices);
	}
	else
		m.parts.push_back({ 0, (unsigned)m.indices.size(), !m.images.empty() ? Textured : Untextured, m.images.empty() ? -1 : 0 });
	m.triangles = (unsigned)(m.indices.size() / 3);
}

bool initProgram()
{
	if (program != 0)
		return true;
	OpenGlSource vertex;
	vertex.addSource(VertexCompatShader).addSource(R"(
in highp vec3 in_pos;
in highp vec3 in_normal;
in highp vec2 in_uv;
uniform highp mat4 mvp;
uniform highp mat4 model;
out highp vec3 vtx_pos;
out highp vec3 vtx_normal;
out highp vec2 vtx_uv;
void main()
{
	vtx_pos = (model * vec4(in_pos, 1.0)).xyz;
	vtx_normal = mat3(model) * in_normal;
	vtx_uv = in_uv;
	gl_Position = mvp * vec4(in_pos, 1.0);
}
)");
	OpenGlSource fragment;
	fragment.addSource(PixelCompatShader).addSource(ModelLightingSource).addSource(R"(
uniform highp vec3 eyePos;
uniform mediump vec3 baseColor;
uniform mediump float specular;
uniform mediump float shininess;
uniform mediump float textured;
uniform sampler2D tex;
in highp vec3 vtx_pos;
in highp vec3 vtx_normal;
in highp vec2 vtx_uv;
void main()
{
	highp vec3 n = normalize(vtx_normal);
	highp vec3 v = normalize(eyePos - vtx_pos);
	if (dot(n, v) < 0.0)
		n = -n;		// inside faces seen through gaps
	mediump vec3 base = baseColor;
	if (textured > 0.5)
		base *= texture(tex, vtx_uv).rgb;
	gl_FragColor = vec4(modelLighting(n, v, base, specular, shininess, vec3(0.0)), 1.0);
}
)");
	program = gl_CompileAndLink(vertex.generate().c_str(), fragment.generate().c_str());
	if (program == 0)
		return false;
	uMvp = glGetUniformLocation(program, "mvp");
	uModel = glGetUniformLocation(program, "model");
	uEye = glGetUniformLocation(program, "eyePos");
	uColor = glGetUniformLocation(program, "baseColor");
	uSpecular = glGetUniformLocation(program, "specular");
	uShininess = glGetUniformLocation(program, "shininess");
	uTextured = glGetUniformLocation(program, "textured");
	uTex = glGetUniformLocation(program, "tex");
	return true;
}

bool upload(Model& m)
{
	if (m.uploaded || m.failed)
		return m.uploaded;
	if (m.indices.empty())
	{
		m.failed = true;
		return false;
	}
	const size_t vertexCount = m.verts.size() / 8;
	m.vao = createVertexArray();
	bindVertexArray(m.vao);
	glGenBuffers(1, &m.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
	glBufferData(GL_ARRAY_BUFFER, m.verts.size() * sizeof(float), m.verts.data(), GL_STATIC_DRAW);
	glGenBuffers(1, &m.ibo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
	if (vertexCount <= 0x10000)
	{
		std::vector<u16> shorts(m.indices.begin(), m.indices.end());
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, shorts.size() * sizeof(u16), shorts.data(), GL_STATIC_DRAW);
		m.indexType = GL_UNSIGNED_SHORT;
	}
	else
	{
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m.indices.size() * sizeof(u32), m.indices.data(), GL_STATIC_DRAW);
		m.indexType = GL_UNSIGNED_INT;
	}
	glVertexAttribPointer(VERTEX_POS_ARRAY, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void *)0);
	glVertexAttribPointer(VERTEX_NORM_ARRAY, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void *)(3 * sizeof(float)));
	glVertexAttribPointer(VERTEX_UV_ARRAY, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void *)(6 * sizeof(float)));
	glEnableVertexAttribArray(VERTEX_POS_ARRAY);
	glEnableVertexAttribArray(VERTEX_NORM_ARRAY);
	glEnableVertexAttribArray(VERTEX_UV_ARRAY);
	bindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	for (const GlbImage& image : m.images)
	{
		GLuint id = 0;
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &id);
		m.textures.push_back(id);
		glcache.BindTexture(GL_TEXTURE_2D, id);
		// mipmapped: the model is small in the view, its texture often large (GLES 3: any size)
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image.width, image.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image.rgba.data());
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glGenerateMipmap(GL_TEXTURE_2D);
		glcache.BindTexture(GL_TEXTURE_2D, 0);
	}
	// on the GPU now
	m.verts = std::vector<float>();
	m.images = std::vector<GlbImage>();
	m.uploaded = true;
	return true;
}

void release(Model& m)
{
	if (m.vbo != 0)
		glDeleteBuffers(1, &m.vbo);
	if (m.ibo != 0)
		glDeleteBuffers(1, &m.ibo);
	deleteVertexArray(m.vao);
	for (GLuint& id : m.textures)
		if (id != 0)
			glcache.DeleteTextures(1, &id);
	m = Model();
}

// One hand's model from the runtime (its path listed) into runtime[hand]: true; else false, with
// why not in ask.why[hand] and whether that may pass in ask.transient[hand]. A model the runtime
// doesn't have now (hand tracking on, the controller asleep, no interaction profile bound yet)
// costs one call.
bool loadRuntimeModel(int hand)
{
	auto fail = [hand](const std::string& why, bool transient) {
		ask.why[hand] = why;
		ask.transient[hand] = transient;
		return false;
	};
	// glTF 2.0 subset 1: its textures are KTX2 (Basis Universal), which glb_mesh doesn't decode
	// (the model is then drawn untextured); a runtime may send plain glTF (PNG, JPEG) instead
	XrRenderModelCapabilitiesRequestFB capabilities { XR_TYPE_RENDER_MODEL_CAPABILITIES_REQUEST_FB };
	capabilities.flags = XR_RENDER_MODEL_SUPPORTS_GLTF_2_0_SUBSET_1_BIT_FB;
	XrRenderModelPropertiesFB properties { XR_TYPE_RENDER_MODEL_PROPERTIES_FB };
	properties.next = &capabilities;
	XrResult r = ask.getProperties(ask.session, ask.path[hand], &properties);
	if (XR_FAILED(r))
		return fail("unavailable (" + std::to_string((int)r) + ")", true);
	if (r == XR_RENDER_MODEL_UNAVAILABLE_FB || properties.modelKey == XR_NULL_RENDER_MODEL_KEY_FB)
		return fail("unavailable", true);
	XrRenderModelLoadInfoFB loadInfo { XR_TYPE_RENDER_MODEL_LOAD_INFO_FB };
	loadInfo.modelKey = properties.modelKey;
	XrRenderModelBufferFB buffer { XR_TYPE_RENDER_MODEL_BUFFER_FB };
	r = ask.loadModel(ask.session, &loadInfo, &buffer);
	if (XR_FAILED(r) || buffer.bufferCountOutput == 0 || buffer.bufferCountOutput > 64u * 1024 * 1024)
		return fail("not loaded (" + std::to_string((int)r) + ")", true);
	std::vector<u8> glb(buffer.bufferCountOutput);
	buffer.bufferCapacityInput = (u32)glb.size();
	buffer.buffer = glb.data();
	r = ask.loadModel(ask.session, &loadInfo, &buffer);
	if (XR_FAILED(r))
		return fail("not loaded (" + std::to_string((int)r) + ")", true);
	GlbMesh mesh;
	std::string error;
	const bool read = loadGlb(glb.data(), std::min<size_t>(glb.size(), buffer.bufferCountOutput), mesh, error);
	const std::string texture = !read ? "unreadable (" + error + ")"
			: !mesh.images.empty() ? "texture " + std::to_string(mesh.images[0].width) + "x" + std::to_string(mesh.images[0].height)
			: "untextured (" + mesh.untextured + ")";
	NOTICE_LOG(RENDERER, "XR: render model %s: \"%s\" version %u, %u bytes, %s", ModelPath[hand], properties.modelName,
			properties.modelVersion, buffer.bufferCountOutput, texture.c_str());
	if (!read)
		return fail("unreadable: " + error, false);		// (the same model again would be too)
	fromGlb(runtime[hand], std::move(mesh));
	ask.loaded[hand] = true;
	ask.transient[hand] = false;
	ask.why[hand].clear();
	return true;
}

// Why the hands not from the runtime are built in ("": none is).
std::string builtInWhy()
{
	if (ask.getProperties == nullptr)
		return ask.why[Left];		// the runtime's lack: the same for both
	if (!ask.loaded[Left] && !ask.loaded[Right] && ask.why[Left] == ask.why[Right])
		return "left and right models " + ask.why[Left];
	std::string why;
	for (int hand : { Left, Right })
		if (!ask.loaded[hand])
			why += (why.empty() ? "" : ", ") + std::string(HandName[hand]) + " model " + ask.why[hand];
	return why;
}

// Asks the runtime again for each hand still built in whose last failure may pass; logs the
// models that arrive and a change in why hands are built in.
void askRuntime()
{
	bool arrived = false;
	if (ask.getProperties != nullptr)
	{
		// the paths the runtime lists, while a wanted one isn't (yet)
		bool unlisted = false;
		for (int hand : { Left, Right })
			unlisted = unlisted || (!ask.loaded[hand] && ask.transient[hand] && !ask.listed[hand]);
		if (unlisted)
		{
			u32 count = 0;
			std::vector<XrRenderModelPathInfoFB> paths;
			if (XR_SUCCEEDED(ask.enumeratePaths(ask.session, 0, &count, nullptr)) && count > 0 && count < 1024)
			{
				paths.resize(count, { XR_TYPE_RENDER_MODEL_PATH_INFO_FB });
				if (XR_FAILED(ask.enumeratePaths(ask.session, count, &count, paths.data())))
					paths.clear();
				paths.resize(std::min<size_t>(paths.size(), count));
			}
			for (const XrRenderModelPathInfoFB& p : paths)
				for (int hand : { Left, Right })
					ask.listed[hand] = ask.listed[hand] || p.path == ask.path[hand];
		}
		for (int hand : { Left, Right })
		{
			if (ask.loaded[hand] || !ask.transient[hand])
				continue;
			if (!ask.listed[hand])
				ask.why[hand] = std::string("not listed (") + ModelPath[hand] + ")";
			else if (loadRuntimeModel(hand))
				arrived = true;
		}
	}
	if (arrived)
	{
		// the brief's form, "triangles" after the first count; "built in" for a hand with none
		std::string left = ask.loaded[Left] ? std::to_string(runtime[Left].triangles) : "built in";
		std::string right = ask.loaded[Right] ? std::to_string(runtime[Right].triangles) : "built in";
		(ask.loaded[Left] ? left : right) += " triangles";
		NOTICE_LOG(RENDERER, "XR: controller models from the runtime (left %s, right %s)", left.c_str(), right.c_str());
	}
	const std::string why = builtInWhy();
	if (!why.empty() && why != ask.loggedWhy)
		NOTICE_LOG(RENDERER, "XR: controller models built in (%s)", why.c_str());
	ask.loggedWhy = why;
}

// The player's own models (xr_own.h) read again when their folder changed, only a hand whose files
// changed: each hand's GLB, at most 200,000 triangles and 8 textures. Logs "XR: controller model from
// controllers/left.glb (<n> triangles, <p> primitives in <k> parts)", or why it was skipped. (On the
// render thread: an old one leaves the GPU.)
void refreshOwn()
{
	if (own::controllersGeneration() == ownGeneration)
		return;
	ownGeneration = own::controllersGeneration();
	for (int hand : { Left, Right })
	{
		const own::ControllerFile& file = own::controller(hand);
		const std::string key = file.present ? file.path + "|" + file.stamp : std::string();
		if (key == ownKey[hand])
			continue;
		ownKey[hand] = key;
		release(own[hand]);
		ownLoaded[hand] = false;
		std::fill(std::begin(ownHasButton[hand]), std::end(ownHasButton[hand]), false);
		ownSidecarShown[hand].clear();
		if (!file.present)
			continue;
		std::string why = file.why;
		GlbMesh mesh;
		if (file.usable)
		{
			std::vector<u8> data;
			std::string error;
			if (!own::readFile(file.path, data, own::MaxFileBytes))
				why = "unreadable, or over 256 MB";
			else if (!loadGlb(data.data(), data.size(), mesh, error, own::MaxTextures, own::MaxTextureSide))
				why = error;
			else if (mesh.indices.size() / 3 > own::MaxTriangles)
				why = "over " + std::to_string(own::MaxTriangles) + " triangles (" + std::to_string(mesh.indices.size() / 3) + ")";
		}
		if (!why.empty())
		{
			WARN_LOG(RENDERER, "XR: controller model from %s skipped: %s", file.shown.c_str(), why.c_str());
			continue;
		}
		// the labels' points: each near the model (a quarter of its size from its bounds at most: else in other units?)
		glm::vec3 lo(FLT_MAX), hi(-FLT_MAX);
		for (size_t i = 0; i + 2 < mesh.positions.size(); i += 3)
		{
			const glm::vec3 p(mesh.positions[i], mesh.positions[i + 1], mesh.positions[i + 2]);
			lo = glm::min(lo, p);
			hi = glm::max(hi, p);
		}
		const float size = std::max({ hi.x - lo.x, hi.y - lo.y, hi.z - lo.z });
		std::vector<std::string> faults = file.buttonFaults;
		for (const auto& point : file.buttons)
		{
			const float off = glm::length(glm::max(glm::max(lo - point.second, point.second - hi), glm::vec3(0.f)));
			if (!(off <= 0.25f * size))
			{
				char why[160];
				snprintf(why, sizeof(why), "\"%s\" is %.3g from the model, over a quarter of its size (%.3g): in other units?", point.first.c_str(), off, size);
				faults.push_back(why);
				continue;
			}
			for (int b = 0; b < Buttons; b++)
				if (point.first == own::ButtonNames[b])
				{
					ownButton[hand][b] = point.second;
					ownHasButton[hand][b] = true;
				}
		}
		fromGlb(own[hand], std::move(mesh), true);
		ownToAim[hand] = file.modelToAim;
		ownLoaded[hand] = true;
		ownSidecarShown[hand] = file.shown.substr(0, file.shown.size() - 4) + ".json";
		NOTICE_LOG(RENDERER, "XR: controller model from %s (%u triangles, %zu primitive%s in %zu part%s)", file.shown.c_str(), own[hand].triangles,
				own[hand].primitives, own[hand].primitives == 1 ? "" : "s", own[hand].parts.size(), own[hand].parts.size() == 1 ? "" : "s");
		for (const std::string& why : faults)
			WARN_LOG(RENDERER, "XR: controller model from %s: label point left out (the built-in place): %s", file.shown.c_str(), why.c_str());
	}
}

}	// namespace

void init(XrInstance instance, XrSession session)
{
	for (Model& m : runtime)
		release(m);
	ask = Ask();
	ask.session = session;
	own::scan();
	refreshOwn();
	if (!builtIn.uploaded && builtIn.indices.empty())
		makeBuiltIn(builtIn);

	PFN_xrGetInstanceProcAddr getProc = sim::active() ? (PFN_xrGetInstanceProcAddr)sim::proc("xrGetInstanceProcAddr")
			: ::xrGetInstanceProcAddr;
	auto find = [&](const char *name, auto& function) {
		function = nullptr;
		return getProc != nullptr && XR_SUCCEEDED(getProc(instance, name, (PFN_xrVoidFunction *)&function)) && function != nullptr;
	};
	PFN_xrStringToPath stringToPath = nullptr;
	PFN_xrEnumerateRenderModelPathsFB enumeratePaths = nullptr;
	PFN_xrGetRenderModelPropertiesFB getProperties = nullptr;
	PFN_xrLoadRenderModelFB loadModel = nullptr;
	std::string lack;
	if (!find("xrEnumerateRenderModelPathsFB", enumeratePaths))
		lack = "the runtime has no XR_FB_render_model";
	else if (!find("xrGetRenderModelPropertiesFB", getProperties) || !find("xrLoadRenderModelFB", loadModel)
			|| !find("xrStringToPath", stringToPath))
		lack = "the runtime has no XR_FB_render_model functions";
	if (!lack.empty())
		// for good: built in, both
		ask.why[Left] = ask.why[Right] = lack;
	else
	{
		ask.enumeratePaths = enumeratePaths;
		ask.getProperties = getProperties;
		ask.loadModel = loadModel;
		for (int hand : { Left, Right })
		{
			if (XR_SUCCEEDED(stringToPath(instance, ModelPath[hand], &ask.path[hand])))
				ask.transient[hand] = true;		// (to be asked)
			else
				ask.why[hand] = std::string("has no path ") + ModelPath[hand];
		}
	}
	askRuntime();
}

bool retryWanted()
{
	for (int hand : { Left, Right })
		if (!ask.loaded[hand] && ask.transient[hand])
			return true;
	return false;
}

void retry()
{
	if (ask.session != XR_NULL_HANDLE && retryWanted())
		askRuntime();
}

bool fromRuntime(int hand)
{
	return (hand == Left || hand == Right) && ask.loaded[hand] && !runtime[hand].failed;
}

void draw(const glm::mat4& viewProj, const glm::vec3& eyePos, int hand, const glm::mat4& grip, const glm::mat4& aim)
{
	if (hand != Left && hand != Right)
		return;
	if (!builtIn.uploaded && builtIn.indices.empty() && !builtIn.failed)
		makeBuiltIn(builtIn);
	if (!initProgram())
		return;
	refreshOwn();
	Model *model = &builtIn;
	glm::mat4 pose = grip;
	if (ownLoaded[hand] && upload(own[hand]))
	{
		model = &own[hand];
		pose = aim * ownToAim[hand];
	}
	else if (fromRuntime(hand) && upload(runtime[hand]))
	{
		model = &runtime[hand];
		pose = grip * kRuntimeModelOffset;
	}
	else
	{
		if (!upload(builtIn))
			return;
		if (hand == Left)
			pose = glm::scale(grip, glm::vec3(-1.f, 1.f, 1.f));		// the right controller, mirrored
	}

	glcache.Disable(GL_SCISSOR_TEST);
	glcache.Disable(GL_STENCIL_TEST);
	glcache.Disable(GL_CULL_FACE);
	glcache.Disable(GL_BLEND);
	glcache.Enable(GL_DEPTH_TEST);
	glcache.DepthMask(GL_TRUE);
	glcache.DepthFunc(GL_LESS);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

	glcache.UseProgram(program);
	const glm::mat4 mvp = viewProj * pose;
	glUniformMatrix4fv(uMvp, 1, GL_FALSE, &mvp[0][0]);
	glUniformMatrix4fv(uModel, 1, GL_FALSE, &pose[0][0]);
	glUniform3f(uEye, eyePos.x, eyePos.y, eyePos.z);
	glUniform1i(uTex, 0);
	glActiveTexture(GL_TEXTURE0);
	bindVertexArray(model->vao);
	glBindBuffer(GL_ARRAY_BUFFER, model->vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model->ibo);
	glVertexAttribPointer(VERTEX_POS_ARRAY, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void *)0);
	glVertexAttribPointer(VERTEX_NORM_ARRAY, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void *)(3 * sizeof(float)));
	glVertexAttribPointer(VERTEX_UV_ARRAY, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (const void *)(6 * sizeof(float)));
	glEnableVertexAttribArray(VERTEX_POS_ARRAY);
	glEnableVertexAttribArray(VERTEX_NORM_ARRAY);
	glEnableVertexAttribArray(VERTEX_UV_ARRAY);
	const size_t indexSize = model->indexType == GL_UNSIGNED_INT ? 4 : 2;
	for (const Part& part : model->parts)
	{
		const Material& m = part.material;
		const bool textured = part.texture >= 0 && part.texture < (int)model->textures.size();
		glUniform1f(uTextured, textured ? 1.f : 0.f);
		glcache.BindTexture(GL_TEXTURE_2D, textured ? model->textures[part.texture] : 0);
		glUniform3f(uColor, m.color.r, m.color.g, m.color.b);
		glUniform1f(uSpecular, m.specular);
		glUniform1f(uShininess, m.shininess);
		glDrawElements(GL_TRIANGLES, (GLsizei)part.count, model->indexType, (const void *)(part.first * indexSize));
	}
	bindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glcache.BindTexture(GL_TEXTURE_2D, 0);
}

glm::vec3 anchor(int hand, Button button)
{
	static const char *const PartOf[Buttons] { "a", "b", "a", "b", "menu", "stick", "trigger", "grip" };
	if (button < 0 || button >= Buttons)
		return glm::vec3(0.f);
	for (const controllermodel::Part& p : controllermodel::Parts)
		if (!strcmp(p.name, PartOf[button]))
			return glm::vec3(hand == Left ? -p.center[0] : p.center[0], p.center[1], p.center[2]);
	return glm::vec3(0.f);
}

LabelPoint labelPoint(int hand, Button button, const glm::mat4& grip, const glm::mat4& aim)
{
	// (a and x, b and y: the same place, either name)
	static const Button Twin[Buttons] { X, Y, A, B, Menu, Stick, Trigger, Grip };
	LabelPoint p;
	if ((hand == Left || hand == Right) && button >= 0 && button < Buttons && ownLoaded[hand] && own[hand].uploaded)
	{
		// the model draw() drew: room <- aim * ownToAim <- GLB
		p.ownModel = true;
		const int b = ownHasButton[hand][button] ? button : ownHasButton[hand][Twin[button]] ? Twin[button] : -1;
		if (b >= 0)
		{
			p.fromFile = true;
			p.local = glm::vec3(ownToAim[hand] * glm::vec4(ownButton[hand][b], 1.f));
			p.room = glm::vec3(aim * glm::vec4(p.local, 1.f));
			return p;
		}
	}
	p.local = anchor(hand, button);
	p.room = glm::vec3(grip * glm::vec4(p.local, 1.f));
	return p;
}

std::string ownSidecar(int hand)
{
	return hand == Left || hand == Right ? ownSidecarShown[hand] : std::string();
}

void term()
{
	release(builtIn);
	for (Model& m : runtime)
		release(m);
	for (Model& m : own)
		release(m);
	ownLoaded[Left] = ownLoaded[Right] = false;
	for (int hand : { Left, Right })
	{
		std::fill(std::begin(ownHasButton[hand]), std::end(ownHasButton[hand]), false);
		ownSidecarShown[hand].clear();
	}
	ownGeneration = 0;
	ownKey[Left].clear();
	ownKey[Right].clear();
	ask = Ask();
	if (program != 0)
		glcache.DeleteProgram(program);
	program = 0;
	uMvp = uModel = uEye = uColor = uSpecular = uShininess = uTextured = uTex = -1;
}

}
