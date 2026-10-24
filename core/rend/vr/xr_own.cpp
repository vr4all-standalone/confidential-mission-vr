/*
	The player's own models (hotd2-vr, Task 14b). See xr_own.h.

	Copyright 2026 mikermak. This file is part of Flycast and is distributed under the GNU GPL v2 or later.
*/
#include "xr_own.h"
#include "oslib/directory.h"
#include "log/Log.h"
#include "stdclass.h"
#include "json.hpp"

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cctype>
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
std::string signature;		// what the last scan saw
unsigned scans;

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

bool vec3(const json& doc, const char *key, glm::vec3& v, std::string& why)
{
	const auto it = doc.find(key);
	if (it == doc.end() || !it->is_array() || it->size() != 3)
	{
		why = std::string("\"") + key + "\" is not [x, y, z]";
		return false;
	}
	for (int k = 0; k < 3; k++)
	{
		if (!(*it)[k].is_number() || !std::isfinite((*it)[k].get<double>()))
		{
			why = std::string("\"") + key + "\" is not 3 finite numbers";
			return false;
		}
		v[k] = (*it)[k].get<float>();
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

void readGun(GunFile& f, const std::string& sidecar, const std::string& shownSidecar)
{
	f.name = f.stem.substr(0, 24);
	json doc;
	if (!readJson(sidecar, shownSidecar, doc, f.why))
		return;
	const auto name = doc.find("name");
	if (name != doc.end() && name->is_string() && !name->get<std::string>().empty() && name->get<std::string>().size() <= 24)
		f.name = name->get<std::string>();
	if (!vec3(doc, "grip", f.grip, f.why) || !vec3(doc, "muzzle", f.muzzle, f.why))
		return;
	if (doc.contains("up") && !vec3(doc, "up", f.up, f.why))
		return;
	const auto length = doc.find("length");
	if (length == doc.end() || !length->is_number())
	{
		f.why = "\"length\" is not a number (metres)";
		return;
	}
	f.length = length->get<float>();
	if (!std::isfinite(f.length) || f.length < 0.05f || f.length > 1.5f)
	{
		f.why = "\"length\" " + std::to_string(f.length) + " is outside 0.05-1.5 m";
		return;
	}
	if (glm::length(f.up) < 1e-6f)
	{
		f.why = "\"up\" is [0, 0, 0]";
		return;
	}
	const glm::vec3 up = glm::normalize(f.up);
	const glm::vec3 barrel = f.muzzle - f.grip;
	if (glm::length(barrel - glm::dot(barrel, up) * up) < 1e-6f * std::max(1.f, glm::length(barrel)))
	{
		f.why = "\"muzzle\" is straight along \"up\" from \"grip\"";
		return;
	}
	f.usable = true;
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
	{
		ok = (*rotation)[k].is_number() && std::isfinite((*rotation)[k].get<double>());
		if (ok)
			q[k] = (*rotation)[k].get<float>();
	}
	if (!ok || std::sqrt(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]) < 1e-6f)
	{
		f.why = "\"rotation\" is not [qx, qy, qz, qw]";
		return;
	}
	const auto scale = doc.find("scale");
	const float s = scale != doc.end() && scale->is_number() ? scale->get<float>() : -1.f;
	if (!std::isfinite(s) || s < 0.0001f || s > 100.f)
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
	std::string sig = gunsDir + "|" + controllersDir;
	for (const std::string& name : names)
	{
		const std::string stem = name.substr(0, name.size() - 4);
		sig += "|" + name + ":" + stamp(gunsDir + "/" + name) + ":" + stamp(gunsDir + "/" + stem + ".json");
	}
	for (const char *hand : { "left", "right" })
		if (!controllersDir.empty())
			sig += std::string("|") + hand + ":" + stamp(controllersDir + "/" + hand + ".glb") + ":" + stamp(controllersDir + "/" + hand + ".json");
	if (scans != 0 && sig == signature)
		return;
	signature = sig;
	scans++;

	gunFiles.clear();
	for (const std::string& name : names)
	{
		GunFile f;
		f.path = gunsDir + "/" + name;
		f.shown = "guns/" + name;
		f.stem = name.substr(0, name.size() - 4);
		readGun(f, gunsDir + "/" + f.stem + ".json", "guns/" + f.stem + ".json");
		gunFiles.push_back(f);
	}
	for (int hand : { 0, 1 })
	{
		ControllerFile& f = controllerFiles[hand];
		f = ControllerFile();
		if (controllersDir.empty())
			continue;
		const std::string base = hand == 0 ? "left" : "right";
		f.path = controllersDir + "/" + base + ".glb";
		f.shown = "controllers/" + base + ".glb";
		f.present = stamp(f.path) != "-";
		if (f.present)
			readController(f, controllersDir + "/" + base + ".json", "controllers/" + base + ".json");
	}
	if (!gunsDir.empty() || !controllersDir.empty())
		NOTICE_LOG(RENDERER, "XR: own models from %s%s%s (%zu guns, controllers%s%s)", gunsDir.c_str(),
				!gunsDir.empty() && !controllersDir.empty() ? " and " : "", controllersDir.c_str(), gunFiles.size(),
				controllerFiles[0].present ? " left" : "", controllerFiles[1].present ? " right" : (controllerFiles[0].present ? "" : " none"));
}

unsigned generation()
{
	return scans;
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
