#include "main.h"
#include "HookManager.h"
#include <iostream>
#include <windows.h>
#include <vector>
#define MODLOADER 1
#include "cwsdk.h"
#include "DLL.h"
#include "ModWidget.h"
#include "crc.h"
#include "mutex.h"
#include "macros.h"
#include "Logger.h"
#include "CrashHandler.h"

using namespace std;

void* base = nullptr; // Module base
std::vector<DLL*> modDLLs; // Enabled mods loaded
std::vector<DLL*> allDlls; // All available mods
std::vector<DLL*> legacyDLLs; // cwmods
GLOBAL HMODULE hSelf; // A handle to ourself, to prevent being unloaded
GLOBAL void** initterm_eReference; // A pointer-pointer to a function which is run extremely soon after starting, or after being unpacked
GETTER_VAR(void*, initterm_e); // A pointer to that function

extern "C" void CleanupImGui();

// Handles injecting callbacks and the mods
bool already_loaded_mods = false;
::mutex already_loaded_mods_mtx;
extern "C" void StartMods() {
    char msg[256] = {0};

    {
        ScopedLock lock(already_loaded_mods_mtx);
        if (already_loaded_mods) {
            return;
        }
        already_loaded_mods = true;
    }

    CW_LOG_INFO("StartMods() invoked. Beginning mod discovery and setup...");

    ModPreInitialize();
    mod::ModWidget::Init();
    cw::HookManager::SetupHandlers();

    //Find mods
    HANDLE hFind;
    WIN32_FIND_DATA data;

    CreateDirectory("Mods", NULL);
    hFind = FindFirstFile("Mods\\*.dll", &data);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            // We should be loaded into the application's address space, so we can just LoadLibraryA
            DLL* dll = new DLL(string("Mods\\") + data.cFileName);
            dll->Load();
            CW_LOG_INFO("Discovered and loaded mod DLL: %s", dll->fileName.c_str());
            modDLLs.push_back(dll);
        } while (FindNextFile(hFind, &data));
        FindClose(hFind);
    }
    
    // Find all the functions the mods may export
    for (DLL* dll: modDLLs) {
        MUST_IMPORT(dll, MakeMod);

        // Versioning and pre-init functions are optional for backward compatibility with older mods
        dll->ModMajorVersion = GetProcAddress(dll->handle, "ModMajorVersion");
        dll->ModMinorVersion = GetProcAddress(dll->handle, "ModMinorVersion");
        dll->ModPreInitialize = GetProcAddress(dll->handle, "ModPreInitialize");
    }

    // Ensure version compatibility
	for (DLL* dll : modDLLs) {
		int majorVersion = dll->ModMajorVersion ? ((int(*)())dll->ModMajorVersion)() : MOD_MAJOR_VERSION;
		int minorVersion = dll->ModMinorVersion ? ((int(*)())dll->ModMinorVersion)() : MOD_MINOR_VERSION;

		if (majorVersion > MOD_MAJOR_VERSION) {
			snprintf(msg, sizeof(msg), "%s has major version %d but requires %d. You should update your mod loader.\n", dll->fileName.c_str(), majorVersion, MOD_MAJOR_VERSION);
			CW_LOG_ERROR("Mod compatibility error: %s", msg);
			Popup("Error", msg);
			exit(1);
		}

		if (majorVersion < MOD_MAJOR_VERSION) {
			snprintf(msg, sizeof(msg), "%s has major version %d but requires %d. The mod author needs to update this mod to CWSDK %d.X\n", dll->fileName.c_str(), majorVersion, MOD_MAJOR_VERSION, MOD_MAJOR_VERSION);
			CW_LOG_ERROR("Mod compatibility error: %s", msg);
			Popup("Error", msg);
			exit(1);
		}

		if (minorVersion > MOD_MINOR_VERSION) {
			snprintf(msg, sizeof(msg), "%s has minor version %d but requires %d or lower. You should update your mod loader.\n", dll->fileName.c_str(), minorVersion, MOD_MINOR_VERSION);
			CW_LOG_ERROR("Mod compatibility error: %s", msg);
			Popup("Error", msg);
			exit(1);
		}
	}

    mod::ModWidget::LoadSave(&modDLLs);
    allDlls = std::vector<DLL*>(modDLLs.begin(), modDLLs.end());
    for (size_t i = 0; i < modDLLs.size();)
    {
        if (!modDLLs.at(i)->enabled)
        {
            CW_LOG_INFO("Mod disabled by configuration: %s", modDLLs.at(i)->fileName.c_str());
            modDLLs.erase(modDLLs.begin() + i);
        }
        else
        {
            ++i;
        }
    }

    CW_LOG_INFO("Initializing %zu active mod(s)...", modDLLs.size());

    // Run Initialization routines on all mods
    for (DLL* dll: modDLLs) {
        if (dll->ModPreInitialize) {
            CW_LOG_DEBUG("Calling ModPreInitialize on %s", dll->fileName.c_str());
            ((void(*)())dll->ModPreInitialize)();
        }
		dll->mod = ((GenericMod*(*)())dll->MakeMod)();
    }

    for (DLL* dll: modDLLs) {
        CW_LOG_INFO("Calling Initialize on mod: %s", dll->fileName.c_str());
		dll->mod->Initialize();
    }

	// Load legacy cwmods. Don't use this.
	hFind = FindFirstFile("Mods\\*.cwmod", &data);
	if (hFind != INVALID_HANDLE_VALUE) {
		do {
			// We should be loaded into the application's address space, so we can just LoadLibraryA
			DLL* dll = new DLL(string("Mods\\") + data.cFileName);
			dll->Load();
			CW_LOG_WARN("Loaded legacy mod: %s (Legacy .cwmod format is deprecated)", dll->fileName.c_str());
			legacyDLLs.push_back(dll);
		} while (FindNextFile(hFind, &data));
		FindClose(hFind);
	}

    CW_LOG_INFO("StartMods() completed successfully. Total enabled mods: %zu, legacy: %zu", 
                modDLLs.size(), legacyDLLs.size());

    for (DLL* dll : modDLLs) {
        CW_LOG_INFO("  Active Mod: %s", dll->fileName.c_str());
    }
    return;
}


extern "C" void ASMStartMods();


// FreeImage plugin contract exports
extern "C" __declspec(dllexport) void Init(void* plugin, int format_id) {
    CW_LOG_INFO("FreeImage Init plugin callback invoked (format_id: %d).", format_id);
}

extern "C" __declspec(dllexport) void FreeImage_Init(void* plugin, int format_id) {
    CW_LOG_INFO("FreeImage_Init plugin callback invoked (format_id: %d).", format_id);
}

void PatchFreeImage(){
    // FreeImage on 64-bit Windows has a known bug in Plugin.cpp where the file search
    // handle is truncated to 32-bit (long), causing FindNextFileW/FindClose to crash (0xC0000005).
    // In FreeImage_Initialise, right after loading and registering CubeModLoader.fip (offset 0x1E8C4E),
    // we patch 0x1E8C4E with a direct JMP to offset 0x1E8C85 (via E9 32 00 00 00).
    // This cleanly bypasses _wfindnext (0x1E8C62) and _findclose (0x1E8C73) while allowing
    // FreeImage_Initialise to complete all remaining format/subsystem initialization normally.
    HMODULE hFreeImage = GetModuleHandleA("FreeImage.dll");
    if (!hFreeImage) {
        CW_LOG_DEBUG("FreeImage.dll handle not found for patching.");
        return;
    }

    DWORD oldProtect;
    void* patchaddr = Offset(hFreeImage, 0x1E8C4E);
    // Unprotect 64 bytes covering the broken find loop up to 0x1E8C85
    if (VirtualProtect(patchaddr, 64, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        // E9 32 00 00 00 = JMP 0x1E8C85 (end of find loop)
        unsigned char jumpExit[5] = { 0xE9, 0x32, 0x00, 0x00, 0x00 };
        memcpy(patchaddr, jumpExit, 5);
        memset(Offset(patchaddr, 5), 0x90, 50); // NOP out remaining 50 bytes until 0x1E8C85
        VirtualProtect(patchaddr, 64, oldProtect, &oldProtect);
        CW_LOG_INFO("Patched FreeImage at 0x1E8C4E to cleanly jump to end of search loop (0x1E8C85).");
    } else {
        CW_LOG_ERROR("Failed to VirtualProtect FreeImage patch address.");
    }
}

void PatchInitterm_ePtr() {
    if (!base) {
        base = GetModuleHandle(NULL);
    }
    if (!base) {
        CW_LOG_ERROR("Cannot patch initterm_e: module base is NULL.");
        return;
    }

    // Get ** to initterm_e
    initterm_eReference = (void**)(Offset(base, 0x42CBD8));
    if (!initterm_eReference) {
        CW_LOG_ERROR("initterm_eReference is NULL.");
        return;
    }

    DWORD oldProtect;
    if (VirtualProtect((LPVOID)initterm_eReference, 8, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        initterm_e = *initterm_eReference;
        *initterm_eReference = (void*)&ASMStartMods;
        VirtualProtect((LPVOID)initterm_eReference, 8, oldProtect, &oldProtect);
        CW_LOG_INFO("Patched initterm_e successfully.");
    } else {
        CW_LOG_ERROR("VirtualProtect failed on initterm_eReference.");
    }
}

void Popup(const char* title, const char* msg ) {
    MessageBoxA(0, msg, title, MB_OK | MB_ICONINFORMATION);
}

void PrintLoadedMods() {
    std::string mods("Mods Loaded:\n");
    for (DLL* dll : modDLLs) {
        mods += dll->fileName;
        mods += "\n";
    }
	if (modDLLs.size() == 0) {
		mods += "<No mods>\n";
	}
	if (legacyDLLs.size() != 0) {
		mods += "\nLegacy mods loaded:\n";
		for (DLL* dll : legacyDLLs) {
			mods += dll->fileName;
			mods += "\n";
		}
	}
    Popup("Loaded Mods", mods.c_str());
}


void* Offset(void* x1, uint64_t x2) {
	return (void*)((char*)x1 + x2);
}

bool already_initialized = false;
::mutex already_initialized_mtx;
extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        {
            ScopedLock lock(already_initialized_mtx);
            if (already_initialized) {
                return TRUE;
            }
            already_initialized = true;
        }

        // Initialize Logging and Crash Handling subsystems
        cw::Logger::Instance().Init("cube-world-logs", "modloader", true);
        cw::CrashHandler::Install("cube-world-logs");

        CW_LOG_INFO("=================================================");
        CW_LOG_INFO("  CubeModLoader Initializing (v%d.%d)", MOD_MAJOR_VERSION, MOD_MINOR_VERSION);
        CW_LOG_INFO("=================================================");

        // Pin ourselves in memory safely without re-invoking LoadLibrary during DllMain
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_PIN | GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, 
                           (LPCSTR)hinstDLL, &hSelf);

        base = GetModuleHandle(NULL);
        if (!base) {
            CW_LOG_ERROR("Failed to get main module handle.");
            return TRUE;
        }

        // Figure out where the executable is and verify its checksum
        char cubePath[_MAX_PATH + 1] = {0};
        GetModuleFileNameA(NULL, cubePath, _MAX_PATH);

        uint32_t checksum = crc32_file(cubePath);
        CW_LOG_INFO("Target executable: %s", cubePath);
        CW_LOG_INFO("Target checksum: 0x%08X (Expected packed: 0x%08X, unpacked: 0x%08X)", 
                    checksum, CUBE_PACKED_CRC, CUBE_UNPACKED_CRC);

        if (checksum == CUBE_PACKED_CRC || checksum == CUBE_UNPACKED_CRC) {
            CW_LOG_INFO("Binary checksum verified. Patching initterm_e...");
            PatchInitterm_ePtr();
        } else {
#ifndef USE_CHECKSUM
			CW_LOG_WARN("Checksum validation bypassed via preprocessor definition.");
			PatchInitterm_ePtr();
#else
            char msg[256] = {0};
            sprintf_s(msg, sizeof(msg), "%s does not seem to be version %s. CRC %08X", cubePath, CUBE_VERSION, checksum);
            CW_LOG_ERROR("%s", msg);
            PatchFreeImage();
            return TRUE;
#endif
        }

        PatchFreeImage();
        CW_LOG_INFO("CubeModLoader DLL_PROCESS_ATTACH complete.");
        break;
    }

    case DLL_PROCESS_DETACH: {
        CW_LOG_INFO("CubeModLoader detaching from process.");
        CleanupImGui();
        cw::CrashHandler::Uninstall();
        cw::Logger::Instance().Shutdown();
        break;
    }
    }
    return TRUE;
}

