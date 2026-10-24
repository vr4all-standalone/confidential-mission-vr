/*
	The player's own models (hotd2-vr, Task 14b). See xr_own.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_own.h"
#include "glb_mesh.h"
#include "oslib/directory.h"
#include "log/Log.h"
#include "stdclass.h"
#include "json.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cctype>
#include <cfloat>
#include <cmath>
#include <cstdio>
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace vr::xr::own
{
namespace
{

using nlohmann::json;

std::vector<GunFile> gunFiles;
ControllerFile controllerFiles[2];
std::string gunsSignature, controllersSignature;	// what the last scan saw in each folder
unsigned gunScans, controllerScans;
bool scanned;

std::string folderOf(const std::string& path)
{
	const size_t slash = path.find_last_of("/\\");
	return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

// A place the folders may be, and what the log puts before guns/..., controllers/... from it
struct Root
{
	std::string dir;
	std::string shown;
	bool bundled = false;	// <data>/models
};

// Where the folders may be: the player's, next to flycast.exe (the PC) and next to the game's zip;
// then the bundled set, <data>/models (the release builds': last, so a player's file goes first).
std::vector<Root> roots()
{
	std::vector<std::string> r;
#ifdef _WIN32
	wchar_t exe[MAX_PATH];
	const DWORD n = GetModuleFileNameW(nullptr, exe, MAX_PATH);
	if (n > 0 && n < MAX_PATH)
	{
		const std::wstring wide(exe, n);
		std::string path;
		const int size = WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), nullptr, 0, nullptr, nullptr);
		path.resize(size > 0 ? size : 0);
		if (size > 0)
			WideCharToMultiByte(CP_UTF8, 0, wide.c_str(), (int)wide.size(), &path[0], size, nullptr, nullptr);
		if (!folderOf(path).empty())
			r.push_back(folderOf(path));
	}
#endif
	const std::string content = folderOf(settings.content.path);
	if (!content.empty() && std::find(r.begin(), r.end(), content) == r.end())
		r.push_back(content);
	std::vector<Root> all;
	for (const std::string& dir : r)
		all.push_back({ dir, "", false });
	std::string bundled = get_writable_data_path("models");
	while (!bundled.empty() && (bundled.back() == '/' || bundled.back() == '\\'))
		bundled.pop_back();
	if (!bundled.empty() && std::find(r.begin(), r.end(), bundled) == r.end())
		all.push_back({ bundled, "bundled models/", true });
	return all;
}

bool isDir(const std::string& path)
{
	DIR *dir = flycast::opendir(path.c_str());
	if (dir == nullptr)
		return false;
	flycast::closedir(dir);
	return true;
}

// size and time, or "-" when missing
std::string stamp(const std::string& path)
{
	struct stat st {};
	if (flycast::stat(path.c_str(), &st) != 0)
		return "-";
	return std::to_string((long long)st.st_size) + "@" + std::to_string((long long)st.st_mtime);
}

std::string lower(std::string s)
{
	for (char& c : s)
		c = (char)std::tolower((unsigned char)c);
	return s;
}

// s, at most max bytes of it, not cut inside a UTF-8 character
std::string cutUtf8(const std::string& s, size_t max)
{
	if (s.size() <= max)
		return s;
	size_t n = max;
	while (n > 0 && ((unsigned char)s[n] & 0xc0) == 0x80)
		n--;
	return s.substr(0, n);
}

// A sidecar's number as the float it becomes: finite as that, and within +-MaxCoordinate
bool number(const json& v, float& out)
{
	if (!v.is_number())
		return false;
	const float f = (float)v.get<double>();
	if (!std::isfinite(f) || std::fabs(f) > MaxCoordinate)
		return false;
	out = f;
	return true;
}

bool vec3(const json& doc, const char *key, glm::vec3& v, std::string& why)
{
	const auto it = doc.find(key);
	if (it == doc.end() || !it->is_array() || it->size() != 3)
	{
		why = std::string("\"") + key + "\" is not [x, y, z]";
		return false;
	}
	for (int k = 0; k < 3; k++)
		if (!number((*it)[k], v[k]))
		{
			why = std::string("\"") + key + "\" is not 3 numbers from -1000000 to 1000000";
			return false;
		}
	return true;
}

bool readJson(const std::string& path, const std::string& shown, json& doc, std::string& why)
{
	std::vector<u8> data;
	if (!readFile(path, data, 64 * 1024))
	{
		why = "no sidecar " + shown;
		return false;
	}
	// from chars: the NDK's libc++ has no std::char_traits<unsigned char> for u8 iterators
	doc = json::parse(std::string(data.begin(), data.end()), nullptr, false);
	if (doc.is_discarded() || !doc.is_object())
	{
		why = shown + " is not a JSON object";
		return false;
	}
	return true;
}

// The sidecar into f; false with what's wrong in f.fault.
bool readGun(GunFile& f, const std::string& sidecar, const std::string& shownSidecar)
{
	f.name = cutUtf8(f.stem, 24);
	json doc;
	if (!readJson(sidecar, shownSidecar, doc, f.fault))
		return false;
	const auto name = doc.find("name");
	if (name != doc.end() && name->is_string() && !name->get<std::string>().empty() && name->get<std::string>().size() <= 24)
		f.name = name->get<std::string>();
	if (!vec3(doc, "grip", f.grip, f.fault) || !vec3(doc, "muzzle", f.muzzle, f.fault))
		return false;
	if (doc.contains("up") && !vec3(doc, "up", f.up, f.fault))
		return false;
	const auto length = doc.find("length");
	if (length == doc.end() || !number(*length, f.length))
	{
		f.fault = "\"length\" is not a number (metres)";
		return false;
	}
	if (f.length < 0.05f || f.length > 1.5f)
	{
		f.fault = "\"length\" " + std::to_string(f.length) + " is outside 0.05-1.5 m";
		return false;
	}
	if (glm::length(f.up) < 1e-6f)
	{
		f.fault = "\"up\" is [0, 0, 0]";
		return false;
	}
	const glm::vec3 up = glm::normalize(f.up);
	const glm::vec3 barrel = f.muzzle - f.grip;
	if (glm::length(barrel - glm::dot(barrel, up) * up) < 1e-6f * std::max(1.f, glm::length(barrel)))
	{
		f.fault = "\"muzzle\" is straight along \"up\" from \"grip\"";
		return false;
	}
	return true;
}

// The gun's GLB looked at, not read (no image decoded): its triangles, its textures' size as they'd
// be read, its bounds, and the sidecar's grip and muzzle near them; what's wrong in f.fault.
void lookAtGlb(GunFile& f)
{
	std::vector<u8> data;
	if (!readFile(f.path, data, MaxFileBytes))
	{
		f.fault = "unreadable, or over 256 MB";
		return;
	}
	GlbMesh mesh;
	if (!loadGlb(data.data(), data.size(), mesh, f.fault, MaxTextures, MaxTextureSide, false))
		return;
	f.triangles = mesh.indices.size() / 3;
	if (f.triangles > MaxTriangles)
	{
		f.fault = "over " + std::to_string(MaxTriangles) + " triangles (" + std::to_string(f.triangles) + ")";
		return;
	}
	f.textures = (int)mesh.images.size();
	f.textureBytes = 0;
	for (const GlbImage& image : mesh.images)
		f.textureBytes += (size_t)image.width * image.height * 4;
	glm::vec3 lo(FLT_MAX), hi(-FLT_MAX);
	for (size_t i = 0; i + 2 < mesh.positions.size(); i += 3)
	{
		const glm::vec3 p(mesh.positions[i], mesh.positions[i + 1], mesh.positions[i + 2]);
		lo = glm::min(lo, p);
		hi = glm::max(hi, p);
	}
	// a units mix-up (a sidecar in metres for a model in cm) puts the hand or the shots metres off
	const float size = std::max({ hi.x - lo.x, hi.y - lo.y, hi.z - lo.z });
	const std::pair<const char *, glm::vec3> points[] { { "grip", f.grip }, { "muzzle", f.muzzle } };
	for (const auto& point : points)
	{
		const float off = glm::length(glm::max(glm::max(lo - point.second, point.second - hi), glm::vec3(0.f)));
		if (!(off <= 2.f * size))
		{
			char why[160];
			snprintf(why, sizeof(why), "\"%s\" is %.1f from the model, over twice its size (%g): in other units?", point.first, off, size);
			f.fault = why;
			return;
		}
	}
}

std::string megabytes(size_t bytes)
{
	char s[32];
	snprintf(s, sizeof(s), "%.1f MB", bytes / 1048576.0);
	return s;
}

void readController(ControllerFile& f, const std::string& sidecar, const std::string& shownSidecar)
{
	json doc;
	if (!readJson(sidecar, shownSidecar, doc, f.why))
		return;
	const auto aim = doc.find("aim");
	if (aim == doc.end() || !aim->is_object())
	{
		f.why = "no \"aim\" { \"position\", \"rotation\" }";
		return;
	}
	glm::vec3 position;
	if (!vec3(*aim, "position", position, f.why))
		return;
	const auto rotation = aim->find("rotation");
	float q[4] {};
	bool ok = rotation != aim->end() && rotation->is_array() && rotation->size() == 4;
	for (int k = 0; ok && k < 4; k++)
		ok = number((*rotation)[k], q[k]);
	if (!ok || std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]) < 1e-6f)
	{
		f.why = "\"rotation\" is not [qx, qy, qz, qw]";
		return;
	}
	const auto scale = doc.find("scale");
	float s = -1.f;
	if (scale == doc.end() || !number(*scale, s) || s < 0.0001f || s > 100.f)
	{
		f.why = "\"scale\" is not a number from 0.0001 to 100 (metres per unit)";
		return;
	}
	const glm::mat4 node = glm::translate(glm::mat4(1.f), position) * glm::mat4_cast(glm::normalize(glm::quat(q[3], q[0], q[1], q[2])));
	f.modelToAim = glm::scale(glm::mat4(1.f), glm::vec3(s)) * glm::inverse(node);
	f.usable = true;
	// the labels' points (optional): one that isn't right is left out, the rest kept
	const auto buttons = doc.find("buttons");
	if (buttons == doc.end())
		return;
	if (!buttons->is_object())
	{
		f.buttonFaults.push_back("\"buttons\" is not { \"<button>\": [x, y, z], ... }");
		return;
	}
	for (auto it = buttons->begin(); it != buttons->end(); ++it)
	{
		glm::vec3 p;
		std::string why;
		if (std::find_if(std::begin(ButtonNames), std::end(ButtonNames), [&](const char *name) { return it.key() == name; }) == std::end(ButtonNames))
			f.buttonFaults.push_back("\"" + cutUtf8(it.key(), 24) + "\" is not a button (a, b, x, y, menu, stick, trigger, grip)");
		else if (vec3(*buttons, it.key().c_str(), p, why))
			f.buttons.emplace_back(it.key(), p);
		else
			f.buttonFaults.push_back(why);
	}
}

}	// namespace

bool readFile(const std::string& path, std::vector<u8>& data, size_t maxBytes)
{
	FILE *f = nowide::fopen(path.c_str(), "rb");
	if (f == nullptr)
		return false;
	bool ok = fseek(f, 0, SEEK_END) == 0;
	const long size = ok ? ftell(f) : -1;
	ok = ok && size >= 0 && (size_t)size <= maxBytes && fseek(f, 0, SEEK_SET) == 0;
	if (ok)
	{
		data.resize((size_t)size);
		ok = size == 0 || fread(data.data(), 1, data.size(), f) == data.size();
	}
	fclose(f);
	return ok;
}

void scan()
{
	// Every root's guns/*.glb, a file name (case aside) from the first root that has it; the bundled
	// names first (a player's file of a bundled one's name too, in its place), then the others, each in
	// file-name order (at most 32); each hand's controller from the first root with its GLB. The
	// folders there, for the log.
	struct Found
	{
		std::string dir, name, shown;	// its folder, its file name, the log's prefix (bundled models/)
		bool bundledSlot = false;	// the bundled root has a file of its name (this one, or one it goes instead of)
	};
	std::vector<Found> found;
	Found hands[2];
	std::vector<std::string> folders;
	std::vector<std::string> earlier;	// the stems of the roots before, lower case
	for (const Root& root : roots())
	{
		const std::string gunsDir = root.dir + "/guns", controllersDir = root.dir + "/controllers";
		std::vector<std::string> stems;
		if (isDir(gunsDir))
		{
			folders.push_back(gunsDir);
			if (DIR *dir = flycast::opendir(gunsDir.c_str()))
			{
				while (dirent *e = flycast::readdir(dir))
				{
					const std::string name = e->d_name;
					if (name.size() <= 4 || lower(name.substr(name.size() - 4)) != ".glb")
						continue;
					const std::string stem = lower(name.substr(0, name.size() - 4));
					if (std::find(earlier.begin(), earlier.end(), stem) == earlier.end())
					{
						found.push_back({ gunsDir, name, root.shown, root.bundled });
						stems.push_back(stem);
					}
					else if (root.bundled)
					{
						// a player's file goes instead of this one, in its place
						for (Found& f : found)
							if (lower(f.name.substr(0, f.name.size() - 4)) == stem)
								f.bundledSlot = true;
					}
				}
				flycast::closedir(dir);
			}
		}
		earlier.insert(earlier.end(), stems.begin(), stems.end());
		if (isDir(controllersDir))
		{
			folders.push_back(controllersDir);
			for (int hand : { 0, 1 })
				if (hands[hand].dir.empty() && stamp(controllersDir + (hand == 0 ? "/left.glb" : "/right.glb")) != "-")
					hands[hand] = { controllersDir, hand == 0 ? "left" : "right", root.shown };
		}
	}
	std::sort(found.begin(), found.end(), [](const Found& a, const Found& b) {
		return a.bundledSlot != b.bundledSlot ? a.bundledSlot : lower(a.name) < lower(b.name);
	});
	if (found.size() > 32)
		found.resize(32);
	// each kind on its own: a change in the guns doesn't read the controllers again, nor the other way
	std::string gunsSig, controllersSig;
	std::vector<std::string> gunStamps;
	for (const Found& f : found)
	{
		const std::string stem = f.name.substr(0, f.name.size() - 4);
		gunStamps.push_back(stamp(f.dir + "/" + f.name) + ":" + stamp(f.dir + "/" + stem + ".json"));
		gunsSig += "|" + f.dir + "/" + f.name + ":" + gunStamps.back();
	}
	for (const Found& h : hands)
		if (!h.dir.empty())
			controllersSig += "|" + h.dir + "/" + h.name + ":" + stamp(h.dir + "/" + h.name + ".glb") + ":" + stamp(h.dir + "/" + h.name + ".json");
	const bool gunsChanged = !scanned || gunsSig != gunsSignature;
	const bool controllersChanged = !scanned || controllersSig != controllersSignature;
	scanned = true;
	if (!gunsChanged && !controllersChanged)
		return;

	if (gunsChanged)
	{
		gunsSignature = gunsSig;
		gunScans++;
		std::vector<GunFile> next;
		for (size_t i = 0; i < found.size(); i++)
		{
			const Found& at = found[i];
			GunFile f;
			f.path = at.dir + "/" + at.name;
			f.stamp = gunStamps[i];
			// the same files as last time: not looked at again
			const auto same = std::find_if(gunFiles.begin(), gunFiles.end(),
					[&](const GunFile& g) { return g.path == f.path && g.stamp == f.stamp; });
			if (same != gunFiles.end())
			{
				next.push_back(*same);
				continue;
			}
			f.shown = at.shown + "guns/" + at.name;
			f.stem = at.name.substr(0, at.name.size() - 4);
			if (readGun(f, at.dir + "/" + f.stem + ".json", at.shown + "guns/" + f.stem + ".json"))
				lookAtGlb(f);
			// (no total across the guns: only the chosen one is read, each capped on its own)
			f.usable = f.fault.empty();
			f.why = f.fault;
			next.push_back(f);
		}
		gunFiles = std::move(next);
	}
	if (controllersChanged)
	{
		controllersSignature = controllersSig;
		controllerScans++;
		for (int hand : { 0, 1 })
		{
			ControllerFile& f = controllerFiles[hand];
			f = ControllerFile();
			const Found& at = hands[hand];
			if (at.dir.empty())
				continue;
			f.path = at.dir + "/" + at.name + ".glb";
			f.shown = at.shown + "controllers/" + at.name + ".glb";
			f.stamp = stamp(f.path) + ":" + stamp(at.dir + "/" + at.name + ".json");
			f.present = stamp(f.path) != "-";
			if (f.present)
				readController(f, at.dir + "/" + at.name + ".json", at.shown + "controllers/" + at.name + ".json");
		}
	}
	if (!folders.empty())
	{
		std::string where;
		for (const std::string& folder : folders)
			where += (where.empty() ? "" : ", ") + folder;
		NOTICE_LOG(RENDERER, "XR: own models from %s (%zu guns, controllers%s%s)", where.c_str(), gunFiles.size(),
				controllerFiles[0].present ? " left" : "", controllerFiles[1].present ? " right" : (controllerFiles[0].present ? "" : " none"));
	}
	// each gun whose state changed
	for (GunFile& f : gunFiles)
	{
		const std::string line = !f.usable ? "skipped: " + f.why : "offered: \"" + f.name + "\" (" + std::to_string(f.triangles)
				+ " triangles, " + std::to_string(f.textures) + (f.textures == 1 ? " texture, " : " textures, ") + megabytes(f.textureBytes) + ")";
		if (line != f.logged)
			NOTICE_LOG(RENDERER, "XR: own gun %s %s", f.shown.c_str(), line.c_str());
		f.logged = line;
	}
}

unsigned gunsGeneration()
{
	return gunScans;
}

unsigned controllersGeneration()
{
	return controllerScans;
}

const std::vector<GunFile>& guns()
{
	return gunFiles;
}

bool sameStem(const std::string& a, const std::string& b)
{
	return lower(a) == lower(b);
}

const ControllerFile& controller(int hand)
{
	return controllerFiles[hand == 0 ? 0 : 1];
}

}
