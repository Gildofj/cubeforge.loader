#ifndef MAIN_H
#define MAIN_H

#include "DLL.h"
#include <vector>
#include <cstdint>

#define MOD_MAJOR_VERSION 7
#define MOD_MINOR_VERSION 3

#define CUBE_VERSION "1.0.0-1"
#define CUBE_PACKED_CRC 0xC7682619
#define CUBE_UNPACKED_CRC 0xBA092543

#define MODLOADER_NAME "CubeForgeLoader"
#define USE_CHECKSUM

void WriteFarJMP(void* source, void* destination);
void Popup(const char* title, const char* msg);
void PrintLoadedMods();

#define MUST_IMPORT(dllname, name)\
dllname->name = GetProcAddress(dllname->handle, #name);\
            if (!dllname->name) {\
                char ERROR_MESSAGE_POPUP[512] = {0};\
                snprintf(ERROR_MESSAGE_POPUP, sizeof(ERROR_MESSAGE_POPUP), "%s does not export " #name ".\n", dllname->fileName.c_str());\
                Popup("Error", ERROR_MESSAGE_POPUP);\
                exit(1);\
            }

#define IMPORT(dllname, name)\
dllname->name = GetProcAddress(dllname->handle, #name);

#define GETTER_VAR(vartype, varname)\
	extern "C" vartype varname = 0;\
	extern "C" vartype Get_##varname(){ return varname; }

#define GLOBAL static

// Shared global variables (previously static/GLOBAL inside main.cpp)
extern void* base;
extern std::vector<DLL*> modDLLs;
extern std::vector<DLL*> allDlls;
extern std::vector<DLL*> legacyDLLs;

void* Offset(void* x1, uint64_t x2);

#endif // MAIN_H

