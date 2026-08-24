# Create an OBJECT library for the shared core files to avoid recompiling them in the tests
add_library(CubeForgeLoader_Common OBJECT
    crc.cpp
    mutex.cpp
    Logger.cpp
    CrashHandler.cpp
)
add_library(CubeForge::Common ALIAS CubeForgeLoader_Common)

# Link to common compiler options, standard, and CWSDK
target_link_libraries(CubeForgeLoader_Common PUBLIC CWSDK CubeForge::CompilerFlags)

# Local include directories
target_include_directories(CubeForgeLoader_Common PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/callbacks>
)

# External/Third-party include directories (marked as SYSTEM to suppress warnings)
target_include_directories(CubeForgeLoader_Common SYSTEM PUBLIC 
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
    ${cubeforge_sdk_SOURCE_DIR}
)

if(MSVC)
    set_property(TARGET CubeForgeLoader_Common PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(CubeForgeLoader_Common PRIVATE _ITERATOR_DEBUG_LEVEL=0 MODLOADER=1 CWSDK_NO_GLOBAL_NEW_DELETE=1)
endif()

# Enable Precompiled Headers to speed up compilation
target_precompile_headers(CubeForgeLoader_Common PUBLIC
    <vector>
    <string>
    <unordered_map>
    <memory>
    <windows.h>
    <imgui.h>
)

if(BUILD_LOADER)
    if(MSVC)
        # On MSVC, use native MASM compiler support for trampolines.asm
        set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/trampolines.asm" PROPERTIES LANGUAGE ASM_MASM)
        add_library(CubeForgeLoader SHARED 
            $<TARGET_OBJECTS:CubeForgeLoader_Common>
            HookManager.cpp
            DLL.cpp
            main.cpp
            ModWidget.cpp
            trampolines.asm
        )
    else()
        # Non-MSVC platforms (like macOS during development/testing)
        add_library(CubeForgeLoader SHARED 
            $<TARGET_OBJECTS:CubeForgeLoader_Common>
            HookManager.cpp
            DLL.cpp
            main.cpp
            ModWidget.cpp
        )
    endif()
    add_library(CubeForge::Loader ALIAS CubeForgeLoader)

    # Local include directories
    target_include_directories(CubeForgeLoader PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/callbacks
    )

    # External/Third-party include directories (marked as SYSTEM to suppress warnings)
    target_include_directories(CubeForgeLoader SYSTEM PRIVATE
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
        ${cubeforge_sdk_SOURCE_DIR}
    )

    target_link_libraries(CubeForgeLoader PRIVATE 
        CWSDK 
        dbghelp 
        imgui::imgui 
        CubeForge::CompilerFlags
    )
    
    target_compile_definitions(CubeForgeLoader PRIVATE MODLOADER=1 CWSDK_NO_GLOBAL_NEW_DELETE=1 _ITERATOR_DEBUG_LEVEL=0)
    target_compile_definitions(CWSDK PRIVATE MODLOADER)

    # Reuse Precompiled Headers from CubeForgeLoader_Common to speed up build further
    target_precompile_headers(CubeForgeLoader REUSE_FROM CubeForgeLoader_Common)

    if(MSVC)
        set_property(TARGET CubeForgeLoader PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    add_custom_command(TARGET CubeForgeLoader POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CubeForgeLoader>
        $<TARGET_FILE_DIR:CubeForgeLoader>/CubeForgeLoader.fip
        COMMENT "Generating CubeForgeLoader.fip from dynamic library"
    )
endif()
