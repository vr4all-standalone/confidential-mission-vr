/*
	A standalone test of core/rend/vr/hands_build.cpp's merged guns (buildGun with GunTarget::merge),
	on synthetic captures: no game data. accept.ps1's cm-merge-unit builds and runs it
	(re\hands_build_test.cmd), with the PC's MSVC; it is no part of the app.

	The gun: a barrel (a box 3 x 0.3 x 0.24 game units along -z, its top's texture mirrored about
	the middle, its two sides sharing one part of the texture), a grip under its back (open at the
	top, inside the barrel; its sides shared too) and a front sight in a second texture; every face
	a grid of quads, each a 4-corner strip as the game sends them. Captures are made through
	made-up cameras, with the polygons each one shows picked by hand, a little noise where the
	case needs it (a capture's own corners all moved alike, as a frame's projection would; or each
	polygon's own copy of a corner moved apart, as separate matrices would).

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "hands_build.h"

#include <glm/glm.hpp>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>
#include <set>
#include <string>
#include <vector>

using namespace vr::hands;
using glm::dvec3;

namespace
{

constexpr uint32_t TexA = 0x100000, TexB = 0x200000;	// the body, the sight
constexpr double Length = 3.0;		// the barrel, game units

struct Corner { dvec3 p; double u, v; };
struct Quad
{
	uint32_t tex;
	std::string part, face;		// "barrel", "+x"
	Corner c[4];				// strip order
	dvec3 mid() const { return (c[0].p + c[1].p + c[2].p + c[3].p) / 4.0; }
};
using Model = std::vector<Quad>;

// A box's faces as quads: n divisions along x, y, z; faces in skip left out. Its +x and -x faces
// share one tile of the texture (mirrored twins); with mirrorTop, so do the two halves of its top.
void box(Model& out, const char *part, uint32_t tex, dvec3 lo, dvec3 hi, const int (&n)[3], int firstTile,
		const std::set<std::string>& skip, bool mirrorTop)
{
	const char *names[3][2] { { "-x", "+x" }, { "-y", "+y" }, { "-z", "+z" } };
	for (int a = 0; a < 3; a++)
		for (int s = 0; s < 2; s++)
		{
			const std::string face = names[a][s];
			if (skip.count(face))
				continue;
			const int b = a == 0 ? 1 : a == 1 ? 2 : 0, c = a == 0 ? 2 : a == 1 ? 0 : 1;
			const int tile = firstTile + (a == 0 ? 0 : a * 2 - 1 + s);
			const double tu = 0.25 * (tile % 4) + 0.02, tv = 0.25 * (tile / 4) + 0.02;
			auto at = [&](int i, int j) {
				Corner k;
				k.p[a] = s ? hi[a] : lo[a];
				k.p[b] = lo[b] + (hi[b] - lo[b]) * i / n[b];
				k.p[c] = lo[c] + (hi[c] - lo[c]) * j / n[c];
				k.u = tu + 0.2 * i / n[b];
				k.v = tv + 0.2 * j / n[c];
				if (mirrorTop && face == "+y")
					k.v = tv + 0.2 * std::abs(k.p.x - (lo.x + hi.x) / 2) / ((hi.x - lo.x) / 2);
				return k;
			};
			for (int i = 0; i < n[b]; i++)
				for (int j = 0; j < n[c]; j++)
					out.push_back({ tex, part, face, { at(i, j), at(i + 1, j), at(i, j + 1), at(i + 1, j + 1) } });
		}
}

Model gunModel()
{
	Model m;
	box(m, "barrel", TexA, dvec3(-0.12, 0.0, -2.4), dvec3(0.12, 0.3, 0.6), { 4, 2, 4 }, 0, {}, true);
	box(m, "grip", TexA, dvec3(-0.1, -1.0, 0.0), dvec3(0.1, 0.15, 0.4), { 2, 3, 2 }, 5, { "+y" }, false);
	box(m, "sight", TexB, dvec3(-0.03, 0.3, -2.3), dvec3(0.03, 0.38, -2.2), { 1, 1, 1 }, 0, { "-y" }, false);
	return m;
}

// The grip's open top: its rim's segments are the body's open edges when all of it is there.
constexpr uint32_t WholeOpenEdges = 2 * (2 + 2);

struct Camera
{
	dvec3 eye, right, up, back;
	dvec3 toEye(const dvec3& p) const { const dvec3 d = p - eye; return dvec3(glm::dot(d, right), glm::dot(d, up), glm::dot(d, back)); }
};
Camera lookAt(dvec3 eye, dvec3 target)
{
	Camera cam;
	cam.eye = eye;
	cam.back = glm::normalize(eye - target);
	cam.right = glm::normalize(glm::cross(dvec3(0, 1, 0), cam.back));
	cam.up = glm::cross(cam.back, cam.right);
	return cam;
}

// a fixed pseudo-random vector in [-1, 1]^3 for a point (and a seed)
dvec3 hashed(const dvec3& p, uint32_t seed)
{
	uint64_t h = 1469598103934665603ULL ^ seed;
	for (int k = 0; k < 3; k++)
	{
		h ^= (uint64_t)std::llround(p[k] * 100000.0);
		h *= 1099511628211ULL;
		h ^= h >> 29;
	}
	dvec3 out;
	for (int k = 0; k < 3; k++)
	{
		h *= 6364136223846793005ULL;
		h += 1442695040888963407ULL;
		out[k] = (double)(h >> 11) / (double)(1ULL << 53) * 2.0 - 1.0;
	}
	return out;
}

struct Shot
{
	Camera cam;
	std::function<bool(const Quad&)> shows;
	double jitter = 0.0;	// of the length: each corner of the gun moved alike in every polygon of the capture
	double perPoly = 0.0;	// of the length: each polygon's copy of a corner moved on its own
};

Rip capture(const Model& model, const Shot& shot)
{
	Rip rip;
	rip.fbWidth = 640;
	rip.fbHeight = 480;
	rip.focalX = rip.focalY = 300.f;
	for (size_t q = 0; q < model.size(); q++)
	{
		const Quad& quad = model[q];
		if (!shot.shows(quad))
			continue;
		RipPoly p;
		p.isp = 0;
		p.tsp = 0;
		p.tcw = (quad.tex >> 3) & 0x1FFFFF;
		p.pcw = 1u << 3;
		for (const Corner& c : quad.c)
		{
			dvec3 e = shot.cam.toEye(c.p);
			if (shot.jitter != 0.0)
				e += hashed(c.p, 7) * (shot.jitter * Length);
			if (shot.perPoly != 0.0)
				e += hashed(c.p, 1000 + (uint32_t)q) * (shot.perPoly * Length);
			const double w = -e.z;
			RipVertex v;
			v.x = (float)(e.x / w * rip.focalX + rip.fbWidth * 0.5);
			v.y = (float)(-e.y / w * rip.focalY + rip.fbHeight * 0.5);
			v.z = (float)(1.0 / w);
			v.u = (float)c.u;
			v.v = (float)c.v;
			std::fill(std::begin(v.col), std::end(v.col), (uint8_t)255);
			p.v.push_back(v);
		}
		rip.polys.push_back(std::move(p));
	}
	return rip;
}

GunTarget target(const Model& model)
{
	uint16_t a = 0, b = 0;
	for (const Quad& q : model)
		(q.tex == TexA ? a : b)++;
	return { "test gun", "test.bin", { TexA, TexB, 0, 0 }, (uint16_t)(a + b), 10, 0.30f, 0.f, 0.f, true,
		{ a, b, 0, 0 }, WholeOpenEdges, { 0.f, 0.f, 0.f }, 0.f };
}

// The model's file: its flags word and the gun mesh's corners.
struct Read
{
	bool ok = false;
	uint32_t flags = 0, sourcePolys = 0;
	std::vector<dvec3> corners;
};
Read readModel(const std::vector<uint8_t>& b)
{
	Read r;
	size_t off = 0;
	auto u32 = [&](uint32_t& x) { if (off + 4 > b.size()) return false; memcpy(&x, &b[off], 4); off += 4; return true; };
	if (b.size() < 8 || memcmp(b.data(), "HND1", 4) != 0)
		return r;
	off = 8 + 23 * 4;
	uint32_t textures = 0, meshes = 0;
	if (!u32(r.flags) || !u32(r.sourcePolys) || !u32(textures))
		return r;
	for (uint32_t t = 0; t < textures; t++)
	{
		uint32_t w, h, f;
		if (!u32(w) || !u32(h) || !u32(f))
			return r;
		off += (size_t)w * h * 4;
	}
	if (!u32(meshes) || meshes == 0)
		return r;
	uint32_t groups;
	if (!u32(groups))
		return r;
	for (uint32_t g = 0; g < groups; g++)
	{
		uint32_t tex, n;
		if (!u32(tex) || !u32(n) || off + (size_t)n * 36 > b.size())
			return r;
		for (uint32_t v = 0; v < n; v++)
		{
			float f[3];
			memcpy(f, &b[off + (size_t)v * 36], 12);
			r.corners.emplace_back(f[0], f[1], f[2]);
		}
		off += (size_t)n * 36;
	}
	r.ok = true;
	return r;
}

// Corners written close together but not at the same place (in the model, metres): a crack in
// a seam. Each pair once.
int cracks(const std::vector<dvec3>& corners, double within)
{
	std::vector<dvec3> unique;
	for (const dvec3& p : corners)
		if (std::none_of(unique.begin(), unique.end(), [&](const dvec3& q) { return q == p; }))
			unique.push_back(p);
	int n = 0;
	for (size_t i = 0; i < unique.size(); i++)
		for (size_t j = i + 1; j < unique.size(); j++)
			n += glm::length(unique[i] - unique[j]) < within;
	return n;
}

struct Result
{
	bool built = false, complete = false;
	std::string error;
	GunReport report;
	Read model;
};
Result run(const std::vector<Rip>& captures, const GunTarget& t)
{
	const TextureSource textures = [](uint32_t, uint32_t, Image& image) {
		image.width = image.height = 2;
		image.rgba.assign(16, 200);
		return true;
	};
	Result r;
	std::vector<uint8_t> out;
	r.built = buildGun(captures, t, textures, out, r.error, &r.complete, &r.report);
	if (r.built)
		r.model = readModel(out);
	return r;
}

std::string summary(const Result& r)
{
	char s[400];
	snprintf(s, sizeof(s), "built %d%s%s, complete %d, %u captures, %u merged onto no. %u, %u dropped, rms %.4f %%, "
			"%u polys %u/%u, open edges %u, extents 1 : %.3f : %.3f, incomplete \"%s\", flags %u, source polys %u",
			r.built, r.built ? "" : ": ", r.error.c_str(), r.complete, r.report.captures, r.report.merged, r.report.reference,
			r.report.dropped, r.report.rms * 100.0, r.report.polys, r.report.texturePolys[0], r.report.texturePolys[1],
			r.report.openEdges, r.report.extents[1], r.report.extents[2], r.report.incomplete.c_str(), r.model.flags,
			r.model.sourcePolys);
	return s;
}

int failed = 0, passed = 0;
struct Case
{
	const char *name;
	std::vector<std::string> why;
	explicit Case(const char *n) : name(n) {}
	void check(bool ok, const std::string& what) { if (!ok) why.push_back(what); }
	void done(const Result& r)
	{
		printf("  %s: %s\n", name, summary(r).c_str());
		if (why.empty())
		{
			printf("PASS %s\n", name);
			passed++;
			return;
		}
		printf("FAIL %s:", name);
		for (const std::string& w : why)
			printf(" %s;", w.c_str());
		printf("\n");
		failed++;
	}
};

const Model model = gunModel();
bool barrel(const Quad& q, const char *face) { return q.part == "barrel" && q.face == face; }
bool grip(const Quad& q, const char *face) { return q.part == "grip" && q.face == face; }

// The reference (the most of the body): the right side, the top's right half, the bottom, the
// front, the grip's front, back and bottom, the sight. Each polygon's copies of its corners a
// little apart (1e-5 of the length: float noise, well under the welding of 1e-4).
Shot shotA()
{
	return { lookAt(dvec3(7, 3, 2), dvec3(0, 0, -0.9)), [](const Quad& q) {
		return barrel(q, "+x") || (barrel(q, "+y") && q.mid().x > 0) || barrel(q, "-y") || barrel(q, "-z")
				|| grip(q, "+x") || grip(q, "-z") || grip(q, "+z") || grip(q, "-y") || q.part == "sight"; }, 0.0, 1e-5 };
}
// The left side, all of the top, the front and the back (the barrel's back only here): its
// corners moved by up to 0.35 % of the length (the fit's rms about 0.2 %).
Shot shotB()
{
	return { lookAt(dvec3(-7, 2, 5), dvec3(0, 0, -0.9)), [](const Quad& q) {
		return barrel(q, "-x") || barrel(q, "+y") || barrel(q, "-z") || barrel(q, "+z")
				|| grip(q, "-x") || grip(q, "-z") || grip(q, "+z"); }, 0.002, 0.0 };
}
// From above: both sides, the top and the sight (so it shows each mirrored corner in two places).
Shot shotC()
{
	return { lookAt(dvec3(0.5, 8, 0), dvec3(0, 0, -0.9)), [](const Quad& q) {
		return q.face == "+x" || q.face == "-x" || barrel(q, "+y") || q.part == "sight"; }, 0.0, 0.0 };
}
// Two polygons of the bottom (6 corners: too few to fit).
Shot shotFew()
{
	return { lookAt(dvec3(1, -6, -1), dvec3(0, 0, -0.9)), [](const Quad& q) {
		return barrel(q, "-y") && q.mid().z < -1.65 && q.mid().x > 0 && q.mid().x < 0.1; }, 0.0, 0.0 };
}

Rip withNaN(Rip rip, size_t polys)
{
	// a NaN x in the first corner of the first polys (all: polys = size)
	for (size_t i = 0; i < rip.polys.size() && i < polys; i++)
		rip.polys[i].v[0].x = std::numeric_limits<float>::quiet_NaN();
	return rip;
}

std::vector<Rip> partial() { return { capture(model, shotA()), capture(model, shotB()), capture(model, shotC()) }; }

bool finite(const std::vector<dvec3>& pts)
{
	return std::all_of(pts.begin(), pts.end(), [](const dvec3& p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); });
}

std::string str(uint32_t n) { return std::to_string(n); }

}	// namespace

int main()
{
	const GunTarget whole = target(model);
	const uint32_t n = whole.wholePolys;
	printf("hands_build_test: the gun has %u polygons (%u/%u), %u open edges when whole\n", n,
			(uint32_t)whole.texturePolys[0], (uint32_t)whole.texturePolys[1], WholeOpenEdges);

	// 1. Partial captures that make all of it together; the seams between polygons of different
	// captures closed (the corners written at the welded places).
	{
		Case c("merge-complete");
		const Result r = run(partial(), whole);
		c.check(r.built, "not built: " + r.error);
		c.check(r.report.merged == 3 && r.report.dropped == 0, "merged " + str(r.report.merged) + ", dropped " + str(r.report.dropped) + ", want 3 and 0");
		c.check(r.report.polys == n, str(r.report.polys) + " polygons, want " + str(n));
		c.check(r.report.openEdges == WholeOpenEdges, str(r.report.openEdges) + " open edges, want " + str(WholeOpenEdges));
		c.check(r.complete && r.report.incomplete.empty(), "not complete: " + r.report.incomplete);
		c.check(r.model.ok && r.model.flags == (ModelGunOnly | ModelComplete), "flags " + str(r.model.flags) + ", want 3");
		c.check(r.model.sourcePolys == n, "source polys " + str(r.model.sourcePolys));
		const int open = r.model.ok ? cracks(r.model.corners, 0.003) : -1;
		c.check(open == 0, str(open) + " pairs of corners within 3 mm but apart (cracks in the seams)");
		c.done(r);
	}
	// 2. One polygon in no capture (a quad of the barrel's back, which only B shows): incomplete,
	// saved anyway, flagged gun only.
	{
		Case c("merge-missing-one");
		std::vector<Rip> caps = partial();
		const Quad *gone = nullptr;
		for (const Quad& q : model)
			if (barrel(q, "+z"))
				gone = &q;
		Model less;
		for (const Quad& q : model)
			if (&q != gone)
				less.push_back(q);
		caps = { capture(less, shotA()), capture(less, shotB()), capture(less, shotC()) };
		const Result r = run(caps, whole);
		char want[160];
		snprintf(want, sizeof(want), "%u of its %u polygons, %u/%u/0/0 by texture, want %u/%u/0/0", n - 1, n,
				(uint32_t)whole.texturePolys[0] - 1, (uint32_t)whole.texturePolys[1], (uint32_t)whole.texturePolys[0],
				(uint32_t)whole.texturePolys[1]);
		c.check(r.built, "not built: " + r.error);
		c.check(!r.complete, "flagged complete");
		c.check(r.report.incomplete == want, "incomplete \"" + r.report.incomplete + "\", want \"" + want + "\"");
		c.check(r.model.ok && r.model.flags == ModelGunOnly, "flags " + str(r.model.flags) + ", want 1");
		c.done(r);
	}
	// 3. The second and the third check, in that order.
	{
		Case c("merge-open-edges");
		GunTarget t = whole;
		t.maxOpenEdges = WholeOpenEdges - 1;
		t.extents[0] = 1.f;
		t.extents[1] = 0.9f;
		t.extents[2] = 0.8f;
		t.extentsOff = 0.1f;
		const Result r = run(partial(), t);
		const std::string want = "the body has " + str(WholeOpenEdges) + " open edges, at most " + str(WholeOpenEdges - 1);
		c.check(r.built && !r.complete, "built " + str(r.built) + ", complete " + str(r.complete));
		c.check(r.report.incomplete == want, "incomplete \"" + r.report.incomplete + "\", want \"" + want + "\"");
		c.check(r.model.ok && r.model.flags == ModelGunOnly, "flags " + str(r.model.flags) + ", want 1");
		c.done(r);
	}
	{
		Case c("merge-extents");
		GunTarget t = whole;
		t.extents[0] = 1.f;
		t.extents[1] = 0.9f;
		t.extents[2] = 0.8f;
		t.extentsOff = 0.1f;
		const Result r = run(partial(), t);
		char want[160];
		snprintf(want, sizeof(want), "the body's extents are 1 : %.3f : %.3f, want 1 : 0.900 : 0.800 within 10 %%",
				r.report.extents[1], r.report.extents[2]);
		c.check(r.built && !r.complete, "built " + str(r.built) + ", complete " + str(r.complete));
		c.check(r.report.extents[1] < 0.6 && r.report.extents[2] < 0.2, "extents 1 : " + std::to_string(r.report.extents[1])
				+ " : " + std::to_string(r.report.extents[2]) + ", want about 1 : 0.45 : 0.08");
		c.check(r.report.incomplete == want, "incomplete \"" + r.report.incomplete + "\", want \"" + want + "\"");
		c.check(r.model.ok && r.model.flags == ModelGunOnly, "flags " + str(r.model.flags) + ", want 1");
		c.done(r);
	}
	// 4. Captures dropped and counted: one with too few corners shared, one with a NaN x in every
	// polygon; and one with a NaN x in one polygon only, which is left out (the rest fits).
	{
		Case c("merge-dropped");
		std::vector<Rip> caps = partial();
		caps.push_back(capture(model, shotFew()));
		caps.push_back(withNaN(capture(model, shotC()), 1000));
		Rip oneNaN = capture(model, shotC());
		// its first polygon a sight's, so the NaN corner is the first of its key (the one a fit would take)
		std::stable_partition(oneNaN.polys.begin(), oneNaN.polys.end(), [](const RipPoly& p) { return p.texture() == TexB; });
		caps.push_back(withNaN(oneNaN, 1));
		const Result r = run(caps, whole);
		c.check(r.report.dropped == 2 && r.report.merged == 4, "merged " + str(r.report.merged) + ", dropped "
				+ str(r.report.dropped) + ", want 4 and 2");
		c.check(r.complete && r.model.ok && r.model.flags == 3, "not complete: " + r.report.incomplete);
		c.check(r.model.ok && finite(r.model.corners), "a corner not finite in the model");
		c.done(r);
	}
	// 5. Every other capture dropped: the reference alone decides (whole here).
	{
		Case c("merge-reference-alone");
		const Shot all { lookAt(dvec3(6, 4, 4), dvec3(0, 0, -0.9)), [](const Quad&) { return true; }, 0.0, 0.0 };
		std::vector<Rip> caps { capture(model, shotFew()), capture(model, all), withNaN(capture(model, shotC()), 1000) };
		const Result r = run(caps, whole);
		c.check(r.report.reference == 1 && r.report.merged == 1 && r.report.dropped == 2, "reference " + str(r.report.reference)
				+ ", merged " + str(r.report.merged) + ", dropped " + str(r.report.dropped) + ", want 1, 1 and 2");
		c.check(r.complete && r.model.ok && r.model.flags == 3, "not complete: " + r.report.incomplete);
		c.done(r);
	}
	// 6. Polygons merged from another capture that lie behind the reference's camera can't go into
	// the model: the checks count what does (not complete, and the polygons it has).
	{
		Case c("merge-behind-camera");
		Shot nearShot { lookAt(dvec3(0, 1.5, 0.5), dvec3(0, 1.5, -10)), [](const Quad&) { return true; }, 0.0, 0.0 };
		const Shot rear { lookAt(dvec3(2, 2, 8), dvec3(0, 0, -0.9)), [](const Quad& q) {
			return barrel(q, "+z") || (q.part == "barrel" && q.mid().z > 0) || q.part == "grip"; }, 0.0, 0.0 };
		uint32_t inFront = 0;
		for (const Quad& q : model)
			inFront += std::all_of(std::begin(q.c), std::end(q.c), [&](const Corner& k) { return nearShot.cam.toEye(k.p).z < -1e-6; });
		const Result r = run({ capture(model, nearShot), capture(model, rear) }, whole);
		const std::string want = str(inFront) + " of its " + str(n) + " polygons";
		c.check(r.built, "not built: " + r.error);
		c.check(r.report.merged == 2, "merged " + str(r.report.merged) + ", want 2");
		c.check(!r.complete && r.model.ok && r.model.flags == ModelGunOnly, "flags " + str(r.model.flags) + ", want 1");
		c.check(r.report.polys == inFront && r.report.incomplete.rfind(want, 0) == 0,
				str(r.report.polys) + " polygons, incomplete \"" + r.report.incomplete + "\", want it to start \"" + want + "\"");
		c.check(r.model.sourcePolys == inFront, "source polys " + str(r.model.sourcePolys) + ", want " + str(inFront));
		c.done(r);
	}

	printf("hands_build_test: %d passed, %d failed\n", passed, failed);
	return failed ? 1 : 0;
}
