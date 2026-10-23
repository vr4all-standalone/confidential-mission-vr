/*
	A glTF 2.0 binary (GLB) read into one triangle mesh (hotd2-vr). See glb_mesh.h.

	The data comes from the OpenXR runtime, so it is checked as untrusted: cgltf_validate (every
	accessor within its buffer), indices within their primitive's vertices, and images decoded
	only from the GLB's own buffer.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "glb_mesh.h"

// the one translation unit with cgltf's implementation
#define CGLTF_IMPLEMENTATION
#include "cgltf/cgltf.h"
// (stb_image's implementation is in rend/CustomTexture.cpp, PNG and JPEG only)
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

namespace vr::xr
{

namespace
{

const char *resultName(cgltf_result r)
{
	switch (r)
	{
	case cgltf_result_success: return "success";
	case cgltf_result_data_too_short: return "data too short";
	case cgltf_result_unknown_format: return "unknown format";
	case cgltf_result_invalid_json: return "invalid JSON";
	case cgltf_result_invalid_gltf: return "invalid glTF";
	case cgltf_result_invalid_options: return "invalid options";
	case cgltf_result_file_not_found: return "file not found";
	case cgltf_result_io_error: return "I/O error";
	case cgltf_result_out_of_memory: return "out of memory";
	case cgltf_result_legacy_gltf: return "glTF 1.0";
	default: return "error";
	}
}

// The base colour image, decoded: from a buffer view of the GLB (or a data: URI cgltf loaded
// into a buffer), PNG or JPEG.
bool loadImage(const cgltf_image *image, GlbMesh& out)
{
	if (image == nullptr || image->buffer_view == nullptr || image->buffer_view->buffer->data == nullptr)
		return false;
	if (image->mime_type != nullptr && strcmp(image->mime_type, "image/png") && strcmp(image->mime_type, "image/jpeg"))
		return false;
	const cgltf_buffer_view *view = image->buffer_view;
	if (view->offset + view->size > view->buffer->size || view->size > 0x7fffffff)
		return false;
	int w = 0, h = 0, channels = 0;
	u8 *pixels = stbi_load_from_memory((const stbi_uc *)cgltf_buffer_view_data(view), (int)view->size, &w, &h, &channels, 4);
	if (pixels == nullptr)
		return false;
	if (w > 0 && h > 0 && w <= 4096 && h <= 4096)
	{
		out.texWidth = w;
		out.texHeight = h;
		out.rgba.assign(pixels, pixels + (size_t)w * h * 4);
	}
	stbi_image_free(pixels);
	return out.texWidth != 0;
}

const cgltf_accessor *attribute(const cgltf_primitive& prim, cgltf_attribute_type type, int index = 0)
{
	for (cgltf_size i = 0; i < prim.attributes_count; i++)
		if (prim.attributes[i].type == type && prim.attributes[i].index == index)
			return prim.attributes[i].data;
	return nullptr;
}

void addPrimitive(const cgltf_primitive& prim, const glm::mat4& world, GlbMesh& out, const cgltf_image *& textureImage)
{
	if (prim.type != cgltf_primitive_type_triangles && prim.type != cgltf_primitive_type_triangle_strip
			&& prim.type != cgltf_primitive_type_triangle_fan)
		return;
	const cgltf_accessor *position = attribute(prim, cgltf_attribute_type_position);
	if (position == nullptr || position->type != cgltf_type_vec3 || position->count == 0)
		return;
	const cgltf_accessor *normal = attribute(prim, cgltf_attribute_type_normal);
	if (normal != nullptr && (normal->type != cgltf_type_vec3 || normal->count != position->count))
		normal = nullptr;
	const cgltf_accessor *uv = attribute(prim, cgltf_attribute_type_texcoord);
	if (uv != nullptr && (uv->type != cgltf_type_vec2 || uv->count != position->count))
		uv = nullptr;
	// this primitive's base colour image: the mesh's texture if it is the first one
	const cgltf_image *image = nullptr;
	if (prim.material != nullptr && prim.material->has_pbr_metallic_roughness)
	{
		const cgltf_texture_view& view = prim.material->pbr_metallic_roughness.base_color_texture;
		if (view.texture != nullptr && view.texcoord == 0)
			image = view.texture->image;
	}
	if (textureImage == nullptr && image != nullptr && loadImage(image, out))
		textureImage = image;
	const bool textured = image != nullptr && image == textureImage;

	const size_t vertexCount = position->count;
	if (out.positions.size() / 3 + vertexCount > 0x7fffffff)
		return;
	const u32 base = (u32)(out.positions.size() / 3);
	const glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(world)));
	for (size_t i = 0; i < vertexCount; i++)
	{
		float p[3] = {}, n[3] = {}, t[2] = {};
		cgltf_accessor_read_float(position, i, p, 3);
		const glm::vec3 wp = glm::vec3(world * glm::vec4(p[0], p[1], p[2], 1.f));
		out.positions.insert(out.positions.end(), { wp.x, wp.y, wp.z });
		glm::vec3 wn(0.f);
		if (normal != nullptr && cgltf_accessor_read_float(normal, i, n, 3))
		{
			wn = normalMatrix * glm::vec3(n[0], n[1], n[2]);
			const float len = glm::length(wn);
			wn = len > 1e-12f ? wn / len : glm::vec3(0.f);
		}
		out.normals.insert(out.normals.end(), { wn.x, wn.y, wn.z });
		if (textured && uv != nullptr)
			cgltf_accessor_read_float(uv, i, t, 2);
		out.uvs.insert(out.uvs.end(), { t[0], t[1] });
	}

	// the primitive's vertex order (indexed or not), as triangles
	const size_t count = prim.indices != nullptr ? prim.indices->count : vertexCount;
	auto vertex = [&](size_t k) -> size_t {
		return prim.indices != nullptr ? cgltf_accessor_read_index(prim.indices, k) : k;
	};
	const size_t firstIndex = out.indices.size();
	auto triangle = [&](size_t a, size_t b, size_t c) {
		if (a >= vertexCount || b >= vertexCount || c >= vertexCount)
			return;		// (an index past the primitive's vertices: left out)
		out.indices.insert(out.indices.end(), { base + (u32)a, base + (u32)b, base + (u32)c });
	};
	if (prim.type == cgltf_primitive_type_triangles)
		for (size_t k = 0; k + 2 < count; k += 3)
			triangle(vertex(k), vertex(k + 1), vertex(k + 2));
	else if (prim.type == cgltf_primitive_type_triangle_strip)
		for (size_t k = 0; k + 2 < count; k++)
			k % 2 == 0 ? triangle(vertex(k), vertex(k + 1), vertex(k + 2)) : triangle(vertex(k + 1), vertex(k), vertex(k + 2));
	else
		for (size_t k = 1; k + 1 < count; k++)
			triangle(vertex(0), vertex(k), vertex(k + 1));

	if (normal == nullptr)
	{
		// face normals, summed at the vertices
		for (size_t k = firstIndex; k < out.indices.size(); k += 3)
		{
			const u32 *tri = &out.indices[k];
			const glm::vec3 a = glm::make_vec3(&out.positions[tri[0] * 3]);
			const glm::vec3 b = glm::make_vec3(&out.positions[tri[1] * 3]);
			const glm::vec3 c = glm::make_vec3(&out.positions[tri[2] * 3]);
			const glm::vec3 fn = glm::cross(b - a, c - a);
			for (int v = 0; v < 3; v++)
				for (int j = 0; j < 3; j++)
					out.normals[tri[v] * 3 + j] += fn[j];
		}
		for (size_t v = base; v < base + vertexCount; v++)
		{
			glm::vec3 n = glm::make_vec3(&out.normals[v * 3]);
			const float len = glm::length(n);
			n = len > 1e-20f ? n / len : glm::vec3(0.f, 1.f, 0.f);
			for (int j = 0; j < 3; j++)
				out.normals[v * 3 + j] = n[j];
		}
	}
}

void addNode(const cgltf_node *node, GlbMesh& out, const cgltf_image *& textureImage, int depth)
{
	if (depth > 64)
		return;		// (no cycles in a valid file; cgltf_validate checks)
	if (node->mesh != nullptr)
	{
		float world[16];
		cgltf_node_transform_world(node, world);
		const glm::mat4 m = glm::make_mat4(world);
		for (cgltf_size i = 0; i < node->mesh->primitives_count; i++)
			addPrimitive(node->mesh->primitives[i], m, out, textureImage);
	}
	for (cgltf_size i = 0; i < node->children_count; i++)
		addNode(node->children[i], out, textureImage, depth + 1);
}

}	// namespace

bool loadGlb(const u8 *data, size_t size, GlbMesh& out, std::string& error)
{
	out = GlbMesh();
	if (data == nullptr || size < 12 || memcmp(data, "glTF", 4) != 0)
	{
		error = "not a GLB file";
		return false;
	}
	cgltf_options options {};
	options.type = cgltf_file_type_glb;
	cgltf_data *gltf = nullptr;
	cgltf_result r = cgltf_parse(&options, data, size, &gltf);
	if (r == cgltf_result_success)
		// the GLB's own binary chunk and data: URIs only (no file path: no files are read)
		r = cgltf_load_buffers(&options, gltf, nullptr);
	if (r == cgltf_result_success)
		r = cgltf_validate(gltf);
	if (r != cgltf_result_success)
	{
		error = std::string("glTF: ") + resultName(r);
		cgltf_free(gltf);
		return false;
	}
	const cgltf_image *textureImage = nullptr;
	const cgltf_scene *scene = gltf->scene != nullptr ? gltf->scene : gltf->scenes_count > 0 ? &gltf->scenes[0] : nullptr;
	if (scene != nullptr)
	{
		for (cgltf_size i = 0; i < scene->nodes_count; i++)
			addNode(scene->nodes[i], out, textureImage, 0);
	}
	else
	{
		// no scene: every root node
		for (cgltf_size i = 0; i < gltf->nodes_count; i++)
			if (gltf->nodes[i].parent == nullptr)
				addNode(&gltf->nodes[i], out, textureImage, 0);
	}
	cgltf_free(gltf);
	if (out.indices.empty())
	{
		error = "no triangles";
		out = GlbMesh();
		return false;
	}
	return true;
}

}
