#include "common.h"
#include "FileHook.h"
#include "Log.h"

#include <tlhelp32.h>

namespace
{
	const FileMap *g_map = nullptr;

	typedef HANDLE(WINAPI *CreateFileA_t)(LPCSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
	typedef HANDLE(WINAPI *CreateFileW_t)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);

	CreateFileA_t RealCreateFileA = nullptr;
	CreateFileW_t RealCreateFileW = nullptr;

	HANDLE WINAPI MyCreateFileA(LPCSTR lpFileName, DWORD access, DWORD share,
		LPSECURITY_ATTRIBUTES sa, DWORD disposition, DWORD flags, HANDLE tmpl)
	{
		std::string replacement;
		if (g_map && lpFileName && g_map->TryReplace(lpFileName, replacement))
		{
			Log::Write("[Redirect] %s -> %s", lpFileName, replacement.c_str());
			return RealCreateFileA(replacement.c_str(), access, share, sa, disposition, flags, tmpl);
		}
		return RealCreateFileA(lpFileName, access, share, sa, disposition, flags, tmpl);
	}

	std::string WideToAnsi(LPCWSTR wide)
	{
		int len = WideCharToMultiByte(CP_ACP, 0, wide, -1, nullptr, 0, nullptr, nullptr);
		if (len <= 0)
			return "";
		std::string out(len - 1, '\0');
		WideCharToMultiByte(CP_ACP, 0, wide, -1, out.data(), len, nullptr, nullptr);
		return out;
	}

	HANDLE WINAPI MyCreateFileW(LPCWSTR lpFileName, DWORD access, DWORD share,
		LPSECURITY_ATTRIBUTES sa, DWORD disposition, DWORD flags, HANDLE tmpl)
	{
		std::string replacement;
		if (g_map && lpFileName && g_map->TryReplace(WideToAnsi(lpFileName), replacement))
		{
			Log::Write("[Redirect] %s -> %s", WideToAnsi(lpFileName).c_str(), replacement.c_str());

			int wlen = MultiByteToWideChar(CP_ACP, 0, replacement.c_str(), -1, nullptr, 0);
			std::vector<wchar_t> wbuf(wlen);
			MultiByteToWideChar(CP_ACP, 0, replacement.c_str(), -1, wbuf.data(), wlen);
			return RealCreateFileW(wbuf.data(), access, share, sa, disposition, flags, tmpl);
		}
		return RealCreateFileW(lpFileName, access, share, sa, disposition, flags, tmpl);
	}

	// KERNEL32.dll!CreateFileA/W patch
	void PatchModule(HMODULE hModule)
	{
		if (!hModule)
			return;

		auto base = (BYTE *)hModule;
		auto dos = (PIMAGE_DOS_HEADER)base;
		if (dos->e_magic != IMAGE_DOS_SIGNATURE)
			return;

		auto nt = (PIMAGE_NT_HEADERS)(base + dos->e_lfanew);
		if (nt->Signature != IMAGE_NT_SIGNATURE)
			return;

		DWORD impRva = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
		if (!impRva)
			return;

		auto importDesc = (PIMAGE_IMPORT_DESCRIPTOR)(base + impRva);

		for (; importDesc->Name; ++importDesc)
		{
			auto moduleName = (const char *)(base + importDesc->Name);
			if (_stricmp(moduleName, "KERNEL32.dll") != 0)
				continue;

			auto thunk = (PIMAGE_THUNK_DATA)(base + importDesc->FirstThunk);
			for (; thunk->u1.Function; ++thunk)
			{
				FARPROC *slot = (FARPROC *)&thunk->u1.Function;

				FARPROC target = nullptr;
				if (*slot == (FARPROC)RealCreateFileA)
					target = (FARPROC)MyCreateFileA;
				else if (*slot == (FARPROC)RealCreateFileW)
					target = (FARPROC)MyCreateFileW;

				if (!target)
					continue;

				DWORD oldProtect;
				if (VirtualProtect(slot, sizeof(*slot), PAGE_READWRITE, &oldProtect))
				{
					*slot = target;
					VirtualProtect(slot, sizeof(*slot), oldProtect, &oldProtect);
				}
			}
		}
	}
}

namespace FileHook
{
	void InstallHooks(const FileMap *map)
	{
		g_map = map;

		HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
		RealCreateFileA = (CreateFileA_t)GetProcAddress(hKernel32, "CreateFileA");
		RealCreateFileW = (CreateFileW_t)GetProcAddress(hKernel32, "CreateFileW");

		HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, GetCurrentProcessId());
		if (snap == INVALID_HANDLE_VALUE)
		{
			Log::Write("[FileHook] Failed to snapshot modules, GetLastError=%lu", GetLastError());
			return;
		}

		MODULEENTRY32 me;
		me.dwSize = sizeof(me);

		int patched = 0;
		if (Module32First(snap, &me))
		{
			do
			{
				PatchModule(me.hModule);
				++patched;
			} while (Module32Next(snap, &me));
		}

		CloseHandle(snap);
		Log::Write("[FileHook] Scanned %d modules for CreateFileA/W imports", patched);
	}
}
