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
	std::string g_realD3D9Path;
	std::string g_realD3D9Method;

	void ResolveRealD3D9()
	{
		char sysDir[MAX_PATH];
		GetSystemDirectoryA(sysDir, MAX_PATH);
		g_realD3D9Path = std::string(sysDir) + "\\d3d9.dll";

		g_hRealD3D9 = LoadLibraryA(g_realD3D9Path.c_str());
		g_realD3D9Method = "GetSystemDirectory path";

		if (!g_hRealD3D9)
		{
			g_hRealD3D9 = LoadLibraryExA("d3d9.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
			g_realD3D9Method = "LOAD_LIBRARY_SEARCH_SYSTEM32";
		}

		if (g_hRealD3D9)
			g_realDirect3DCreate9 = (Direct3DCreate9_t)GetProcAddress(g_hRealD3D9, "Direct3DCreate9");
	}

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

		if (g_realDirect3DCreate9)
		{
			Log::Write("Real d3d9.dll resolved via %s: %s (handle=%p, Direct3DCreate9=%p)",
				g_realD3D9Method.c_str(), g_realD3D9Path.c_str(), (void *)g_hRealD3D9, (void *)g_realDirect3DCreate9);
		}
		else
		{
			Log::Write("Ooooh...we failed to get the actual d3d9.dll aha... GetLastError=%lu",
				g_realD3D9Path.c_str(), GetLastError());
		}

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

	IDirect3D9 *result = g_realDirect3DCreate9(SDKVersion);
	Log::Write("Direct3DCreate9(%u) -> %p", SDKVersion, (void *)result);
	return result;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
	(void)hModule;
	(void)reserved;

	if (reason == DLL_PROCESS_ATTACH)
		ResolveRealD3D9();

	return TRUE;
}
