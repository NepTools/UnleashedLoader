#include "common.h"
#include "ModManager.h"
#include "IniFile.h"
#include "Log.h"

namespace
{
	std::string g_gameRoot;
	FileMap g_fileMap;

	const char *kLoaderIniTemplate =
		"; UnleashedLoader configuration.\r\n"
		"; List mods to enable under [Mods], one per line, in load order.\r\n"
		"; Mods later in the list win when two mods replace the same file.\r\n"
		";\r\n"
		"; [Mods]\r\n"
		"; Mod1=some-mod-folder-name\r\n"
		"; Mod2=another-mod-folder-name\r\n"
		"\r\n"
		"[Mods]\r\n";

	bool DirectoryExists(const std::string &path)
	{
		DWORD attr = GetFileAttributesA(path.c_str());
		return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
	}

	bool FileExists(const std::string &path)
	{
		DWORD attr = GetFileAttributesA(path.c_str());
		return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
	}

	bool IsReservedModFile(const std::string &fileNameLower)
	{
		return fileNameLower == "mod.ini" ||
			fileNameLower == "mod.ico" ||
			fileNameLower == "config.ini" ||
			fileNameLower == "readme.txt" ||
			fileNameLower == "readme.md" ||
			fileNameLower == "preview.png";
	}

	void ScanModFolder(const std::string &modRoot, const std::string &dir, int depth)
	{
		WIN32_FIND_DATAA fd;
		std::string pattern = dir + "\\*";
		HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
		if (hFind == INVALID_HANDLE_VALUE)
			return;

		do
		{
			std::string name = fd.cFileName;
			if (name == "." || name == "..")
				continue;

			std::string full = dir + "\\" + name;

			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				ScanModFolder(modRoot, full, depth + 1);
				continue;
			}

			if (depth == 0)
			{
				std::string lower = name;
				for (char &c : lower)
					c = (char)tolower((unsigned char)c);
				if (IsReservedModFile(lower))
					continue;
			}

			std::string relKey = full.substr(modRoot.size() + 1);
			g_fileMap.AddFile(relKey, full);
		} while (FindNextFileA(hFind, &fd));

		FindClose(hFind);
	}

	void LoadMod(const std::string &modsDir, const std::string &folderName)
	{
		std::string modRoot = modsDir + "\\" + folderName;
		if (!DirectoryExists(modRoot))
		{
			Log::Write("[ModManager] Skipping '%s': folder not found", folderName.c_str());
			return;
		}

		std::string name = folderName, author, version;
		IniFile modIni;
		if (modIni.Load(modRoot + "\\mod.ini"))
		{
			for (auto &kv : modIni.GetSection("Mod"))
			{
				if (_stricmp(kv.first.c_str(), "Name") == 0) name = kv.second;
				else if (_stricmp(kv.first.c_str(), "Author") == 0) author = kv.second;
				else if (_stricmp(kv.first.c_str(), "Version") == 0) version = kv.second;
			}
		}

		size_t before = g_fileMap.Count();
		ScanModFolder(modRoot, modRoot, 0);
		size_t added = g_fileMap.Count() - before;

		Log::Write("[ModManager] Loaded '%s' (%s%s%s) - %zu file override(s)",
			name.c_str(),
			author.empty() ? "unknown author" : author.c_str(),
			version.empty() ? "" : ", v",
			version.c_str(),
			added);
	}
}

namespace ModManager
{
	void Init()
	{
		char exePath[MAX_PATH];
		GetModuleFileNameA(nullptr, exePath, MAX_PATH);

		std::string path = exePath;
		size_t slash = path.find_last_of("\\/");
		g_gameRoot = (slash == std::string::npos) ? "." : path.substr(0, slash);

		g_fileMap.SetGameRoot(g_gameRoot);

		std::string modsDir = g_gameRoot + "\\mods";
		if (!DirectoryExists(modsDir))
			CreateDirectoryA(modsDir.c_str(), nullptr);

		std::string loaderIniPath = g_gameRoot + "\\loader.ini";
		if (!FileExists(loaderIniPath))
		{
			FILE *f = fopen(loaderIniPath.c_str(), "w");
			if (f)
			{
				fputs(kLoaderIniTemplate, f);
				fclose(f);
			}
		}

		IniFile loaderIni;
		if (!loaderIni.Load(loaderIniPath))
		{
			Log::Write("[ModManager] No loader.ini found, no mods will be loaded");
			return;
		}

		auto mods = loaderIni.GetSection("Mods");
		Log::Write("[ModManager] %zu mod(s) listed in loader.ini", mods.size());

		for (auto &kv : mods)
			LoadMod(modsDir, kv.second);

		Log::Write("[ModManager] %zu total file override(s) active", g_fileMap.Count());
	}

	const FileMap &GetFileMap()
	{
		return g_fileMap;
	}

	const std::string &GetGameRoot()
	{
		return g_gameRoot;
	}
}
