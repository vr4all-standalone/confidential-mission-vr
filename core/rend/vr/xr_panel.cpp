/*
	Panels with text in the headset (hotd2-vr). See xr_panel.h.

	The text is drawn on the CPU with stb_truetype (the copy that comes with Dear ImGui) in
	Flycast's own Roboto, into a picture that goes on a quad.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_panel.h"
#include "rend/gles/gles.h"
#include "rend/gles/glcache.h"
#include "oslib/resources.h"
#include "log/Log.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "imgui/imstb_truetype.h"
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>
#include <memory>

namespace vr::xr
{
namespace
{

// The lines panel's picture, and where it is
constexpr int Width = 1024, Height = 448;
constexpr float WidthMetres = 1.3f, Distance = 1.6f;
// The rows panel's picture: 1024 px across 0.9 m (about as sharp as a Quest 3 shows it at 1.2 m),
// a band for the title, the rows, a band for the footer.
constexpr int RowsWidth = 1024, TitleBand = 100, RowHeight = 72, FooterBand = 76;
constexpr float RowsWidthMetres = 0.9f;

std::unique_ptr<u8[]> fontData;
stbtt_fontinfo font;
bool fontTried, fontLoaded;

GLuint program;
GLuint quadVao, quadVbo;
GLint uMvp = -1;
// A panel's picture on the GPU, and what it says now (made again only when that changes)
struct Picture
{
	GLuint texture = 0;
	std::string shown;
};
Picture linesPicture, rowsPicture;
// the picture being painted
int canvasWidth, canvasHeight;
std::vector<u8> pixels;

void logGlErrors()
{
	// (hotd2-vr: the first few seconds only, at most 10 lines: checked after every draw it
	// costs a GPU round trip and filled the log at headset rate)
	static int checks, logged;
	if (checks >= 1000 || logged >= 10)
		return;
	checks++;
	for (GLenum error; (error = glGetError()) != GL_NO_ERROR && logged < 10; logged++)
		ERROR_LOG(RENDERER, "VR panel OpenGL error 0x%04x", error);
}

bool loadFont()
{
	if (fontTried)
		return fontLoaded;
	fontTried = true;
	size_t size = 0;
	fontData = resource::load("fonts/Roboto-Bold.ttf", size);
	fontLoaded = fontData != nullptr && stbtt_InitFont(&font, fontData.get(), stbtt_GetFontOffsetForIndex(fontData.get(), 0)) != 0;
	if (!fontLoaded)
		WARN_LOG(RENDERER, "XR: no font for the panel");
	return fontLoaded;
}

void blend(int x, int y, const glm::vec3& colour, float alpha)
{
	if (x < 0 || y < 0 || x >= canvasWidth || y >= canvasHeight || alpha <= 0.f)
		return;
	u8 *p = &pixels[((size_t)y * canvasWidth + x) * 4];
	for (int c = 0; c < 3; c++)
		p[c] = (u8)std::lround(p[c] * (1.f - alpha) + colour[c] * 255.f * alpha);
}

void fill(int x0, int y0, int x1, int y1, const glm::vec3& colour, float alpha = 1.f)
{
	for (int y = std::max(y0, 0); y < std::min(y1, canvasHeight); y++)
		for (int x = std::max(x0, 0); x < std::min(x1, canvasWidth); x++)
			blend(x, y, colour, alpha);
}

// A new picture: the dark background and a thin red frame, like the game's own.
void background(int width, int height)
{
	canvasWidth = width;
	canvasHeight = height;
	pixels.assign((size_t)width * height * 4, 0);
	for (size_t i = 0; i < pixels.size(); i += 4)
	{
		pixels[i] = 16;
		pixels[i + 1] = 15;
		pixels[i + 2] = 17;
		pixels[i + 3] = 235;
	}
	const glm::vec3 red(0.62f, 0.06f, 0.06f);
	fill(0, 0, width, 5, red);
	fill(0, height - 5, width, height, red);
	fill(0, 0, 5, height, red);
	fill(width - 5, 0, width, height, red);
}

// One line, its baseline at y: its left end at x (align 0), its middle (0.5) or its right end (1).
void text(const std::string& s, float px, float x, int y, const glm::vec3& colour, float align = 0.5f)
{
	const float scale = stbtt_ScaleForPixelHeight(&font, px);
	float width = 0.f;
	for (size_t i = 0; i < s.size(); i++)
	{
		int advance, bearing;
		stbtt_GetCodepointHMetrics(&font, (u8)s[i], &advance, &bearing);
		width += advance * scale;
		if (i + 1 < s.size())
			width += scale * stbtt_GetCodepointKernAdvance(&font, (u8)s[i], (u8)s[i + 1]);
	}
	// (centred: worked out as the lines panel always did, to the bit)
	x = align == 0.5f ? (x * 2.f - width) * 0.5f : x - width * align;
	std::vector<u8> glyph;
	for (size_t i = 0; i < s.size(); i++)
	{
		const int c = (u8)s[i];
		int advance, bearing, x0, y0, x1, y1;
		stbtt_GetCodepointHMetrics(&font, c, &advance, &bearing);
		stbtt_GetCodepointBitmapBoxSubpixel(&font, c, scale, scale, x - std::floor(x), 0.f, &x0, &y0, &x1, &y1);
		const int w = x1 - x0, h = y1 - y0;
		if (w > 0 && h > 0)
		{
			glyph.assign((size_t)w * h, 0);
			stbtt_MakeCodepointBitmapSubpixel(&font, glyph.data(), w, h, w, scale, scale, x - std::floor(x), 0.f, c);
			for (int gy = 0; gy < h; gy++)
				for (int gx = 0; gx < w; gx++)
					blend((int)std::floor(x) + x0 + gx, y + y0 + gy, colour, glyph[(size_t)gy * w + gx] / 255.f);
		}
		x += advance * scale;
		if (i + 1 < s.size())
			x += scale * stbtt_GetCodepointKernAdvance(&font, c, (u8)s[i + 1]);
	}
}

void paint(const Panel& panel)
{
	background(Width, Height);
	float total = 0.f;
	for (const PanelLine& line : panel.lines)
		total += line.size * Height * 1.45f;
	const int bar = panel.progress >= 0.f ? 46 : 0;
	float y = (Height - total - bar) * 0.5f;
	for (const PanelLine& line : panel.lines)
	{
		const float px = line.size * Height;
		y += px * 1.45f;
		if (!line.text.empty())
			text(line.text, px, Width * 0.5f, (int)(y - px * 0.38f), line.colour);
	}
	if (panel.progress >= 0.f)
	{
		const int x0 = Width * 18 / 100, x1 = Width - x0, top = (int)y + 24, bottom = top + 14;
		fill(x0, top, x1, bottom, glm::vec3(0.24f, 0.24f, 0.26f));
		fill(x0, top, x0 + (int)((x1 - x0) * std::clamp(panel.progress, 0.f, 1.f)), bottom, glm::vec3(0.8f, 0.1f, 0.1f));
	}
	if (!panel.footer.empty())
		text(panel.footer, Height * 0.042f, Width * 0.5f, Height - 28, glm::vec3(0.55f));
}

int rowsHeight(const RowsPanel& panel)
{
	return TitleBand + RowHeight * (int)panel.rows.size() + FooterBand;
}

void paintRows(const RowsPanel& panel)
{
	const int w = RowsWidth, h = rowsHeight(panel);
	background(w, h);
	const glm::vec3 red(0.86f, 0.13f, 0.1f), white(0.92f), grey(0.62f), dim(0.42f), rule(0.3f, 0.09f, 0.08f);
	// the baseline that puts capitals this size in the middle of a band
	auto baseline = [](float middle, float px) { return (int)std::lround(middle + px * 0.3f); };
	text(panel.title, 50.f, w * 0.5f, baseline(TitleBand * 0.5f + 2.f, 50.f), red);
	fill(40, TitleBand - 3, w - 40, TitleBand - 1, rule);
	for (size_t i = 0; i < panel.rows.size(); i++)
	{
		const PanelRow& row = panel.rows[i];
		const int top = TitleBand + RowHeight * (int)i;
		const float middle = top + RowHeight * 0.5f;
		if (row.selected)
		{
			fill(12, top + 4, w - 12, top + RowHeight - 4, glm::vec3(0.36f, 0.09f, 0.08f), 0.85f);
			fill(12, top + 4, 20, top + RowHeight - 4, red);
		}
		const glm::vec3 labelColour = row.dim ? dim : white;
		const glm::vec3 valueColour = row.dim ? dim : row.selected ? white : grey;
		if (row.stepper)
		{
			const glm::vec3 sign = row.dim ? dim : row.selected ? red : grey;
			text("-", 48.f, 44.f, baseline(middle, 48.f), sign);
			text("+", 48.f, w - 44.f, baseline(middle, 48.f), sign);
		}
		text(row.label, 38.f, 76.f, baseline(middle, 38.f), labelColour, 0.f);
		if (!row.value.empty())
			text(row.value, 38.f, w - 76.f, baseline(middle, 38.f), valueColour, 1.f);
	}
	fill(40, h - FooterBand + 1, w - 40, h - FooterBand + 3, rule);
	if (!panel.footer.empty())
		text(panel.footer, 26.f, w * 0.5f, baseline(h - FooterBand * 0.5f - 1.f, 26.f), grey);
}

bool initGl()
{
	if (program != 0)
		return true;
	OpenGlSource vertex;
	vertex.addSource(VertexCompatShader).addSource(R"(
in highp vec4 in_pos;
in highp vec2 in_uv;
uniform highp mat4 mvp;
out highp vec2 vtx_uv;
void main()
{
	vtx_uv = in_uv;
	gl_Position = mvp * vec4(in_pos.xyz, 1.0);
}
)");
	OpenGlSource fragment;
	fragment.addSource(PixelCompatShader).addSource(R"(
uniform sampler2D tex;
in highp vec2 vtx_uv;
void main()
{
	gl_FragColor = texture(tex, vtx_uv);
}
)");
	program = gl_CompileAndLink(vertex.generate().c_str(), fragment.generate().c_str());
	if (program == 0)
		return false;
	uMvp = glGetUniformLocation(program, "mvp");
	glcache.UseProgram(program);
	glUniform1i(glGetUniformLocation(program, "tex"), 0);
	quadVao = createVertexArray();
	bindVertexArray(quadVao);
	glGenBuffers(1, &quadVbo);
	glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
	glBufferData(GL_ARRAY_BUFFER, 20 * sizeof(float), nullptr, GL_STREAM_DRAW);
	glVertexAttribPointer(VERTEX_POS_ARRAY, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)0);
	glVertexAttribPointer(VERTEX_UV_ARRAY, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (const void *)(3 * sizeof(float)));
	glEnableVertexAttribArray(VERTEX_POS_ARRAY);
	glEnableVertexAttribArray(VERTEX_UV_ARRAY);
	bindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return true;
}

// Binds the picture's texture, painted again (paintIt) when what it says changed.
template<typename Paint>
void bindPicture(Picture& picture, const std::string& says, Paint paintIt)
{
	glActiveTexture(GL_TEXTURE0);
	const bool made = picture.texture != 0;
	if (!made)
		glGenTextures(1, &picture.texture);
	glcache.BindTexture(GL_TEXTURE_2D, picture.texture);
	if (!made)
	{
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glcache.TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	if (!made || says != picture.shown)
	{
		picture.shown = says;
		paintIt();
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, canvasWidth, canvasHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	}
}

// The bound picture on a quad hx by hy metres each way from the middle of `model`, facing +z.
void drawQuad(const glm::mat4& viewProj, const glm::mat4& model, float hx, float hy)
{
	const float quad[] {
		-hx,  hy, 0.f,  0.f, 0.f,
		 hx,  hy, 0.f,  1.f, 0.f,
		-hx, -hy, 0.f,  0.f, 1.f,
		 hx, -hy, 0.f,  1.f, 1.f,
	};
	glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(quad), quad);
	glcache.UseProgram(program);
	const glm::mat4 mvp = viewProj * model;
	glUniformMatrix4fv(uMvp, 1, GL_FALSE, &mvp[0][0]);
	glcache.Disable(GL_SCISSOR_TEST);
	glcache.Disable(GL_STENCIL_TEST);
	glcache.Disable(GL_DEPTH_TEST);
	glcache.Disable(GL_CULL_FACE);
	glcache.Enable(GL_BLEND);
	glcache.BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);	// (through the cache: the game's next draw relies on it)
	bindVertexArray(quadVao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	bindVertexArray(0);
	glcache.Disable(GL_BLEND);
	glcache.BindTexture(GL_TEXTURE_2D, 0);
	logGlErrors();
}

}	// namespace

void drawPanel(const glm::mat4& viewProj, const Panel& panel)
{
	if (!loadFont() || !initGl())
		return;
	// what it says, the bar in steps of half a percent
	std::string says;
	for (const PanelLine& line : panel.lines)
		says += line.text + '\n';
	says += panel.footer + '\n' + std::to_string(panel.progress < 0.f ? -1 : (int)(panel.progress * 200.f));
	bindPicture(linesPicture, says, [&] { paint(panel); });
	const float hx = WidthMetres * 0.5f, hy = hx * Height / Width;
	drawQuad(viewProj, glm::translate(glm::mat4(1.f), glm::vec3(0.f, 0.f, -Distance)), hx, hy);
}

RowsLayout rowsLayout(const RowsPanel& panel)
{
	const float metres = RowsWidthMetres / RowsWidth;
	const int height = rowsHeight(panel);
	return { RowsWidthMetres, height * metres, RowHeight * metres, (height * 0.5f - (TitleBand + RowHeight * 0.5f)) * metres };
}

void drawRowsPanel(const glm::mat4& viewProj, const glm::mat4& model, const RowsPanel& panel)
{
	if (!loadFont() || !initGl())
		return;
	std::string says = panel.title + '\n';
	for (const PanelRow& row : panel.rows)
		says += row.label + '\t' + row.value + '\t' + (char)('0' + row.selected + row.dim * 2 + row.stepper * 4) + '\n';
	says += panel.footer;
	bindPicture(rowsPicture, says, [&] { paintRows(panel); });
	const RowsLayout layout = rowsLayout(panel);
	drawQuad(viewProj, model, layout.width * 0.5f, layout.height * 0.5f);
}

bool rowsPanelHit(const glm::mat4& model, const RowsPanel& panel, const glm::vec3& origin, const glm::vec3& dir,
		glm::vec3 *point, glm::vec2 *uv)
{
	const glm::mat4 toPanel = glm::inverse(model);
	const glm::vec3 o = glm::vec3(toPanel * glm::vec4(origin, 1.f));
	const glm::vec3 d = glm::vec3(toPanel * glm::vec4(dir, 0.f));
	if (d.z > -1e-6f)
		return false;	// along it, or from behind
	const float t = -o.z / d.z;
	if (t <= 0.f)
		return false;
	const glm::vec3 hit = o + d * t;
	const RowsLayout layout = rowsLayout(panel);
	const glm::vec2 at(hit.x / layout.width + 0.5f, 0.5f - hit.y / layout.height);
	if (at.x < 0.f || at.x > 1.f || at.y < 0.f || at.y > 1.f)
		return false;
	if (point != nullptr)
		*point = origin + dir * t;
	if (uv != nullptr)
		*uv = at;
	return true;
}

int rowAt(const glm::mat4& model, const RowsPanel& panel, const glm::vec3& origin, const glm::vec3& dir, float *u)
{
	glm::vec2 uv;
	if (!rowsPanelHit(model, panel, origin, dir, nullptr, &uv))
		return -1;
	if (u != nullptr)
		*u = uv.x;
	const float y = uv.y * rowsHeight(panel) - TitleBand;
	if (y < 0.f)
		return -1;
	const int row = (int)(y / RowHeight);
	return row < (int)panel.rows.size() ? row : -1;
}

void termPanel()
{
	if (program != 0)
		glcache.DeleteProgram(program);
	for (Picture *picture : { &linesPicture, &rowsPicture })
	{
		if (picture->texture != 0)
			glcache.DeleteTextures(1, &picture->texture);
		*picture = Picture();
	}
	if (quadVbo != 0) glDeleteBuffers(1, &quadVbo);
	deleteVertexArray(quadVao);
	program = 0;
	quadVao = quadVbo = 0;
	uMvp = -1;
}

}
