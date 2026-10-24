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

// Where the folders may be: next to flycast.exe (the PC), next to the game's zip.
std::vector<std::string> roots()
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
	return r;
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
	std::string gunsDir, controllersDir;
	for (const std::string& root : roots())
	{
		if (gunsDir.empty() && isDir(root + "/guns"))
			gunsDir = root + "/guns";
		if (controllersDir.empty() && isDir(root + "/controllers"))
			controllersDir = root + "/controllers";
	}
	// the guns in file-name order (at most 32)
	std::vector<std::string> names;
	if (!gunsDir.empty())
		if (DIR *dir = flycast::opendir(gunsDir.c_str()))
		{
			while (dirent *e = flycast::readdir(dir))
			{
				const std::string name = e->d_name;
				if (name.size() > 4 && lower(name.substr(name.size() - 4)) == ".glb")
					names.push_back(name);
			}
			flycast::closedir(dir);
		}
	std::sort(names.begin(), names.end(), [](const std::string& a, const std::string& b) { return lower(a) < lower(b); });
	if (names.size() > 32)
		names.resize(32);
	// each folder on its own: a change in one doesn't read the other again
	std::string gunsSig = gunsDir, controllersSig = controllersDir;
	std::vector<std::string> gunStamps;
	for (const std::string& name : names)
	{
		const std::string stem = name.substr(0, name.size() - 4);
		gunStamps.push_back(stamp(gunsDir + "/" + name) + ":" + stamp(gunsDir + "/" + stem + ".json"));
		gunsSig += "|" + name + ":" + gunStamps.back();
	}
	for (const char *hand : { "left", "right" })
		if (!controllersDir.empty())
			controllersSig += std::string("|") + hand + ":" + stamp(controllersDir + "/" + hand + ".glb") + ":" + stamp(controllersDir + "/" + hand + ".json");
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
		for (size_t i = 0; i < names.size(); i++)
		{
			GunFile f;
			f.path = gunsDir + "/" + names[i];
			f.stamp = gunStamps[i];
			// the same files as last time: not looked at again
			const auto same = std::find_if(gunFiles.begin(), gunFiles.end(),
					[&](const GunFile& g) { return g.path == f.path && g.stamp == f.stamp; });
			if (same != gunFiles.end())
			{
				next.push_back(*same);
				continue;
			}
			f.shown = "guns/" + names[i];
			f.stem = names[i].substr(0, names[i].size() - 4);
			if (readGun(f, gunsDir + "/" + f.stem + ".json", "guns/" + f.stem + ".json"))
				lookAtGlb(f);
			next.push_back(f);
		}
		gunFiles = std::move(next);
		// the budget, in file-name order: a gun that would take the textures over it is left out
		size_t total = 0;
		for (GunFile& f : gunFiles)
		{
			f.usable = f.fault.empty() && f.textureBytes <= GunTextureBudget - total;
			f.why = !f.fault.empty() ? f.fault : f.usable ? std::string()
					: "its textures (" + megabytes(f.textureBytes) + ") would take the folder's guns over "
						+ megabytes(GunTextureBudget) + " (make-gun.py makes them smaller)";
			if (f.usable)
				total += f.textureBytes;
		}
	}
	if (controllersChanged)
	{
		controllersSignature = controllersSig;
		controllerScans++;
		for (int hand : { 0, 1 })
		{
			ControllerFile& f = controllerFiles[hand];
			f = ControllerFile();
			if (controllersDir.empty())
				continue;
			const std::string base = hand == 0 ? "left" : "right";
			f.path = controllersDir + "/" + base + ".glb";
			f.shown = "controllers/" + base + ".glb";
			f.stamp = stamp(f.path) + ":" + stamp(controllersDir + "/" + base + ".json");
			f.present = stamp(f.path) != "-";
			if (f.present)
				readController(f, controllersDir + "/" + base + ".json", "controllers/" + base + ".json");
		}
	}
	if (!gunsDir.empty() || !controllersDir.empty())
		NOTICE_LOG(RENDERER, "XR: own models from %s%s%s (%zu guns, controllers%s%s)", gunsDir.c_str(),
				!gunsDir.empty() && !controllersDir.empty() ? " and " : "", controllersDir.c_str(), gunFiles.size(),
				controllerFiles[0].present ? " left" : "", controllerFiles[1].present ? " right" : (controllerFiles[0].present ? "" : " none"));
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

const ControllerFile& controller(int hand)
{
	return controllerFiles[hand == 0 ? 0 : 1];
}

}
