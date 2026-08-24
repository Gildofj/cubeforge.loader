#include "DLL.h"
#include <iostream>
#include <utility>

DLL::DLL(std::string fileName)
    : fileName(std::move(fileName))
    , handle(nullptr)
    , ModPreInitialize(nullptr)
    , ModMajorVersion(nullptr)
    , ModMinorVersion(nullptr)
    , MakeMod(nullptr)
    , mod(nullptr)
    , enabled(true)
    , isLegacy(false)
{
}

DLL::DLL(DLL&& other) noexcept
    : fileName(std::move(other.fileName))
    , handle(other.handle)
    , ModPreInitialize(other.ModPreInitialize)
    , ModMajorVersion(other.ModMajorVersion)
    , ModMinorVersion(other.ModMinorVersion)
    , MakeMod(other.MakeMod)
    , mod(other.mod)
    , enabled(other.enabled)
    , isLegacy(other.isLegacy)
{
    other.handle = nullptr;
    other.mod = nullptr;
}

DLL& DLL::operator=(DLL&& other) noexcept
{
    if (this != &other) {
        if (handle != nullptr) {
            FreeLibrary(handle);
        }
        fileName = std::move(other.fileName);
        handle = other.handle;
        ModPreInitialize = other.ModPreInitialize;
        ModMajorVersion = other.ModMajorVersion;
        ModMinorVersion = other.ModMinorVersion;
        MakeMod = other.MakeMod;
        mod = other.mod;
        enabled = other.enabled;
        isLegacy = other.isLegacy;

        other.handle = nullptr;
        other.mod = nullptr;
    }
    return *this;
}

HMODULE DLL::Load()
{
    if (handle != nullptr) {
        return handle;
    }

    handle = LoadLibraryA(fileName.c_str());
    if (!handle) {
        std::cerr << "[ModLoader Error] Could not load " << fileName << ": Win32 Error " << GetLastError() << "\n";
    }
    return handle;
}

bool DLL::IsLoaded() const noexcept
{
    return handle != nullptr;
}

DLL::~DLL()
{
    // Mod DLLs stay loaded in address space unless explicit cleanup
}
