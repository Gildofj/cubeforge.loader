cmake_minimum_required(VERSION 3.25)

if(CMAKE_HOST_WIN32 OR CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    project(CubeForgeLoader LANGUAGES C CXX ASM_MASM)
else()
    project(CubeForgeLoader LANGUAGES C CXX)
endif()

# Enforce standard-compliant and modern behavior
cmake_policy(SET CMP0091 NEW) # Use MSVC_RUNTIME_LIBRARY property

if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Enable compiler cache if available
find_program(CCACHE_PROGRAM NAMES ccache sccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    message(STATUS "Found compiler cache: ${CCACHE_PROGRAM}")
endif()

# Enable Clang-Tidy integration if requested
option(ENABLE_CLANG_TIDY "Enable static analysis with Clang-Tidy" OFF)
if(ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_PROGRAM NAMES clang-tidy)
    if(CLANG_TIDY_PROGRAM)
        set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_PROGRAM}")
        message(STATUS "Found Clang-Tidy: ${CLANG_TIDY_PROGRAM}")
    endif()
endif()

# Enable Link-Time Optimization (LTO) for Release/RelWithDebInfo builds if supported
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported LANGUAGES CXX)
if(ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL TRUE)
endif()

# Create compiler interface target to enforce standards and common options
add_library(Loader_CompilerFlags INTERFACE)
add_library(CubeForge::CompilerFlags ALIAS Loader_CompilerFlags)
target_compile_features(Loader_CompilerFlags INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(Loader_CompilerFlags INTERFACE 
        $<$<COMPILE_LANGUAGE:CXX>:
            /W4          # Enforce stricter warnings (Level 4)
            /EHsc        # Standard C++ exception handling
            /utf-8       # Set Source and Executable character sets to UTF-8
            /permissive- # Strict standards conformance
            /Zc:__cplusplus # Correct __cplusplus macro
            /wd4828      # Disable warning on characters that cannot be represented in the current code page
            /wd4100      # Suppress unreferenced formal parameter (common in callbacks/hooks)
            /MP          # Multi-process compilation
            /Zc:inline   # Remove unreferenced functions or data
        >
    )
    target_compile_definitions(Loader_CompilerFlags INTERFACE
        _ITERATOR_DEBUG_LEVEL=0
        _CRT_SECURE_NO_WARNINGS
        DIRECTINPUT_VERSION=0x0800
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
else()
    target_compile_options(Loader_CompilerFlags INTERFACE 
        $<$<COMPILE_LANGUAGE:CXX>:
            -Wall 
            -Wextra 
            -Wpedantic 
            -Wno-unused-parameter
        >
    )
endif()

# Temporarily disable Clang-Tidy for third-party/external targets to avoid warnings/errors and speed up compilation
set(OLD_CMAKE_CXX_CLANG_TIDY ${CMAKE_CXX_CLANG_TIDY})
set(CMAKE_CXX_CLANG_TIDY "")

# Fetch and configure Dear ImGui
include(FetchContent)

# Mark ImGui as SYSTEM to suppress compiler warnings from its headers
FetchContent_Declare(
    imgui
    SYSTEM
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.91.8
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(imgui)
if(NOT TARGET imgui)
    add_library(imgui STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_demo.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )
    # Using SYSTEM on include directories as well to guarantee header isolation
    target_include_directories(imgui SYSTEM PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    if(MSVC)
        target_compile_definitions(imgui PUBLIC _ITERATOR_DEBUG_LEVEL=0)
        set_property(TARGET imgui PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
    target_link_libraries(imgui PUBLIC d3d11 dxgi)
    target_link_libraries(imgui PRIVATE Loader_CompilerFlags)
    set_target_properties(imgui PROPERTIES INTERPROCEDURAL_OPTIMIZATION FALSE)
endif()
add_library(imgui::imgui ALIAS imgui)

# Fetch and configure cubeforge_sdk
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../cubeforge.sdk/CMakeLists.txt")
    FetchContent_Declare(
        cubeforge_sdk
        SYSTEM
        SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../cubeforge.sdk"
    )
else()
    FetchContent_Declare(
        cubeforge_sdk
        SYSTEM
        GIT_REPOSITORY https://github.com/Gildofj/cubeforge.sdk.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
    )
endif()
FetchContent_MakeAvailable(cubeforge_sdk)

# Restore Clang-Tidy for our project targets
set(CMAKE_CXX_CLANG_TIDY ${OLD_CMAKE_CXX_CLANG_TIDY})



option(BUILD_LOADER "Build CubeForgeLoader dynamic library and .fip" ON)
option(BUILD_TESTS "Build automated unit and integration tests" OFF)

# Always add src subdirectory because both loader and test runner depend on its objects
add_subdirectory(src)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
