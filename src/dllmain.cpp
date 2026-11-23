#include "common.h"
#include <d3d9.h>

#include "Log.h"
#include "ModManager.h"
#include "FileHook.h"

namespace
{
	typedef IDirect3D9 *(WINAPI *Direct3DCreate9_t)(UINT);

	HMODULE g_hRealD3D9 = nullptr;
	Direct3DCreate9_t g_realDirect3DCreate9 = nullptr;
	bool g_initialized = false;

	// this runs after the game loads all of its dlls
	void LazyInit()
	{
		if (g_initialized)
			return;
		g_initialized = true;

		char exePath[MAX_PATH];
		GetModuleFileNameA(nullptr, exePath, MAX_PATH);
		std::string path = exePath;
		size_t slash = path.find_last_of("\\/");
		std::string gameRoot = (slash == std::string::npos) ? "." : path.substr(0, slash);

		Log::Init(gameRoot);
		Log::Write("UnleashedLoader starting up (game root: %s)", gameRoot.c_str());

		ModManager::Init();
		FileHook::InstallHooks(&ModManager::GetFileMap());

		Log::Write("Initialization complete");
	}
}

extern "C" __declspec(dllexport) IDirect3D9 *WINAPI Direct3DCreate9(UINT SDKVersion)
{
	LazyInit();

	if (!g_realDirect3DCreate9)
		return nullptr;

	return g_realDirect3DCreate9(SDKVersion);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
	(void)hModule;
	(void)reserved;

	if (reason == DLL_PROCESS_ATTACH)
	{
		char sysDir[MAX_PATH];
		GetSystemDirectoryA(sysDir, MAX_PATH);
		std::string realPath = std::string(sysDir) + "\\d3d9.dll";

		g_hRealD3D9 = LoadLibraryA(realPath.c_str());
		if (g_hRealD3D9)
			g_realDirect3DCreate9 = (Direct3DCreate9_t)GetProcAddress(g_hRealD3D9, "Direct3DCreate9");
	}

	return TRUE;
}
