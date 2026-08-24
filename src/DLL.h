#ifndef DLL_H
#define DLL_H

#include <string>
#include <windows.h>
#define MODLOADER 1
#include "cwsdk.h"

class DLL
{
public:
    std::string fileName;
    HMODULE handle{nullptr};

    FARPROC ModPreInitialize{nullptr};
    FARPROC ModMajorVersion{nullptr};
    FARPROC ModMinorVersion{nullptr};
    FARPROC MakeMod{nullptr};

    GenericMod* mod{nullptr};
    bool enabled{true};
    bool isLegacy{false};

    explicit DLL(std::string fileName);
    virtual ~DLL();

    // Disable copy to prevent double free of resources
    DLL(const DLL&) = delete;
    DLL& operator=(const DLL&) = delete;

    // Enable move semantics
    DLL(DLL&& other) noexcept;
    DLL& operator=(DLL&& other) noexcept;

    HMODULE Load();
    [[nodiscard]] bool IsLoaded() const noexcept;
    [[nodiscard]] bool IsLegacy() const noexcept { return isLegacy; }
};

#endif // DLL_H
