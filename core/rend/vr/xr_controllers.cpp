/*
	The player's controllers drawn in the hands (hotd2-vr). See xr_controllers.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_controllers.h"
#include "xr_gun.h"
#include "xr_sim.h"
#include "glb_mesh.h"
#include "controller_model.h"
#include "rend/gles/gles.h"
#include "rend/gles/glcache.h"
#include "log/Log.h"

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
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
// the runtime's model: its own texture times white; untextured, the body's grey
const Material Textured { { 1.f, 1.f, 1.f }, 0.25f, 30.f };
const Material Untextured { { 0.55f, 0.56f, 0.58f }, 0.30f, 30.f };

// A model: interleaved position, normal, uv (8 floats a vertex), 16- or 32-bit indices, parts
// (ranges of the indices, each with its material), an optional base colour texture. Kept on
// the CPU until the first draw puts it on the GPU.
struct Part { unsigned first, count; Material material; };
struct Model
{
	std::vector<float> verts;
	std::vector<u32> indices;
	std::vector<Part> parts;
	int texWidth = 0, texHeight = 0;
	std::vector<u8> rgba;
	unsigned triangles = 0;

	GLuint vao = 0, vbo = 0, ibo = 0, texture = 0;
	GLenum indexType = GL_UNSIGNED_SHORT;
	bool uploaded = false;
	bool failed = false;
};

Model builtIn;
Model runtime[2];
bool runtimeModels;		// both hands' models from the runtime

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

void fromGlb(Model& m, GlbMesh&& mesh)
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
	m.texWidth = mesh.texWidth;
	m.texHeight = mesh.texHeight;
	m.rgba = std::move(mesh.rgba);
	m.parts.push_back({ 0, (unsigned)m.indices.size(), m.texWidth != 0 ? Textured : Untextured });
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
	if (m.texWidth != 0)
	{
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &m.texture);
		glcache.BindTexture(GL_TEXTURE_2D, m.texture);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m.texWidth, m.texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m.rgba.data());
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glcache.BindTexture(GL_TEXTURE_2D, 0);
	}
	// on the GPU now
	m.verts = std::vector<float>();
	m.rgba = std::vector<u8>();
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
	if (m.texture != 0)
		glcache.DeleteTextures(1, &m.texture);
	m = Model();
}

// The runtime's model of one hand: its GLB, or why not (empty: loaded).
std::string loadRuntimeModel(XrInstance instance, XrSession session, int hand, PFN_xrGetInstanceProcAddr getProc)
{
	PFN_xrStringToPath stringToPath = nullptr;
	PFN_xrGetRenderModelPropertiesFB getProperties = nullptr;
	PFN_xrLoadRenderModelFB loadModel = nullptr;
	if (XR_FAILED(getProc(instance, "xrStringToPath", (PFN_xrVoidFunction *)&stringToPath))
			|| XR_FAILED(getProc(instance, "xrGetRenderModelPropertiesFB", (PFN_xrVoidFunction *)&getProperties))
			|| XR_FAILED(getProc(instance, "xrLoadRenderModelFB", (PFN_xrVoidFunction *)&loadModel))
			|| stringToPath == nullptr || getProperties == nullptr || loadModel == nullptr)
		return "the runtime has no XR_FB_render_model functions";
	XrPath path = XR_NULL_PATH;
	if (XR_FAILED(stringToPath(instance, ModelPath[hand], &path)))
		return std::string("no path ") + ModelPath[hand];
	// glTF with PNG or JPEG images, not KTX2
	XrRenderModelCapabilitiesRequestFB capabilities { XR_TYPE_RENDER_MODEL_CAPABILITIES_REQUEST_FB };
	capabilities.flags = XR_RENDER_MODEL_SUPPORTS_GLTF_2_0_SUBSET_1_BIT_FB;
	XrRenderModelPropertiesFB properties { XR_TYPE_RENDER_MODEL_PROPERTIES_FB };
	properties.next = &capabilities;
	XrResult r = getProperties(session, path, &properties);
	if (XR_FAILED(r) || r == XR_RENDER_MODEL_UNAVAILABLE_FB || properties.modelKey == XR_NULL_RENDER_MODEL_KEY_FB)
		return std::string(HandName[hand]) + " model unavailable (" + std::to_string((int)r) + ")";
	XrRenderModelLoadInfoFB loadInfo { XR_TYPE_RENDER_MODEL_LOAD_INFO_FB };
	loadInfo.modelKey = properties.modelKey;
	XrRenderModelBufferFB buffer { XR_TYPE_RENDER_MODEL_BUFFER_FB };
	r = loadModel(session, &loadInfo, &buffer);
	if (XR_FAILED(r) || buffer.bufferCountOutput == 0 || buffer.bufferCountOutput > 64u * 1024 * 1024)
		return std::string(HandName[hand]) + " model not loaded (" + std::to_string((int)r) + ")";
	std::vector<u8> glb(buffer.bufferCountOutput);
	buffer.bufferCapacityInput = (u32)glb.size();
	buffer.buffer = glb.data();
	r = loadModel(session, &loadInfo, &buffer);
	if (XR_FAILED(r))
		return std::string(HandName[hand]) + " model not loaded (" + std::to_string((int)r) + ")";
	NOTICE_LOG(RENDERER, "XR: render model %s: \"%s\" version %u, %u bytes", ModelPath[hand], properties.modelName,
			properties.modelVersion, buffer.bufferCountOutput);
	GlbMesh mesh;
	std::string error;
	if (!loadGlb(glb.data(), std::min<size_t>(glb.size(), buffer.bufferCountOutput), mesh, error))
		return std::string(HandName[hand]) + " model unreadable: " + error;
	fromGlb(runtime[hand], std::move(mesh));
	return "";
}

}	// namespace

void init(XrInstance instance, XrSession session)
{
	for (Model& m : runtime)
		release(m);
	runtimeModels = false;
	if (!builtIn.uploaded && builtIn.indices.empty())
		makeBuiltIn(builtIn);

	PFN_xrGetInstanceProcAddr getProc = sim::active() ? (PFN_xrGetInstanceProcAddr)sim::proc("xrGetInstanceProcAddr")
			: ::xrGetInstanceProcAddr;
	std::string why;
	PFN_xrEnumerateRenderModelPathsFB enumeratePaths = nullptr;
	if (getProc == nullptr || XR_FAILED(getProc(instance, "xrEnumerateRenderModelPathsFB", (PFN_xrVoidFunction *)&enumeratePaths))
			|| enumeratePaths == nullptr)
		why = "the runtime has no XR_FB_render_model";
	else
	{
		// the paths the runtime has models for: both controllers' wanted
		u32 count = 0;
		std::vector<XrRenderModelPathInfoFB> paths;
		if (XR_SUCCEEDED(enumeratePaths(session, 0, &count, nullptr)) && count > 0 && count < 1024)
		{
			paths.resize(count, { XR_TYPE_RENDER_MODEL_PATH_INFO_FB });
			if (XR_FAILED(enumeratePaths(session, count, &count, paths.data())))
				paths.clear();
			paths.resize(std::min<size_t>(paths.size(), count));
		}
		PFN_xrStringToPath stringToPath = nullptr;
		getProc(instance, "xrStringToPath", (PFN_xrVoidFunction *)&stringToPath);
		for (int hand : { Left, Right })
		{
			XrPath want = XR_NULL_PATH;
			bool listed = false;
			if (stringToPath != nullptr && XR_SUCCEEDED(stringToPath(instance, ModelPath[hand], &want)))
				for (const XrRenderModelPathInfoFB& p : paths)
					listed = listed || p.path == want;
			if (!listed)
			{
				why = std::string("the runtime lists no ") + ModelPath[hand];
				break;
			}
			why = loadRuntimeModel(instance, session, hand, getProc);
			if (!why.empty())
				break;
		}
	}
	if (why.empty())
	{
		runtimeModels = true;
		NOTICE_LOG(RENDERER, "XR: controller models from the runtime (left %u triangles, right %u)",
				runtime[Left].triangles, runtime[Right].triangles);
	}
	else
	{
		for (Model& m : runtime)
			release(m);
		NOTICE_LOG(RENDERER, "XR: controller models built in (%s)", why.c_str());
	}
}

bool fromRuntime(int hand)
{
	return runtimeModels && (hand == Left || hand == Right) && !runtime[hand].failed;
}

void draw(const glm::mat4& viewProj, const glm::vec3& eyePos, int hand, const glm::mat4& grip)
{
	if (hand != Left && hand != Right)
		return;
	if (!builtIn.uploaded && builtIn.indices.empty() && !builtIn.failed)
		makeBuiltIn(builtIn);
	if (!initProgram())
		return;
	Model *model = &builtIn;
	glm::mat4 pose = grip;
	if (fromRuntime(hand) && upload(runtime[hand]))
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
	glUniform1f(uTextured, model->texture != 0 ? 1.f : 0.f);
	glActiveTexture(GL_TEXTURE0);
	glcache.BindTexture(GL_TEXTURE_2D, model->texture);
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

void term()
{
	release(builtIn);
	for (Model& m : runtime)
		release(m);
	runtimeModels = false;
	if (program != 0)
		glcache.DeleteProgram(program);
	program = 0;
	uMvp = uModel = uEye = uColor = uSpecular = uShininess = uTextured = uTex = -1;
}

}
