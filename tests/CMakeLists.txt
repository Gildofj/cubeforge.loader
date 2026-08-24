set(TEST_SOURCES
    test_main.cpp
    unit/test_crc.cpp
    unit/test_priority_callbacks.cpp
    unit/test_version_compat.cpp
    unit/test_modwidget_storage.cpp
    unit/test_modwidget_pagination.cpp
    unit/test_mutex.cpp
    unit/test_trampoline.cpp
    unit/test_logger.cpp
    unit/test_crash_handler.cpp
    integration/test_mod_lifecycle.cpp
    integration/test_fault_injection.cpp
)

# In modern CMake (3.12+), linking against an OBJECT library transitively adds its object files
# and propagates compile options, definitions, and include directories automatically.
add_executable(test_runner 
    ${TEST_SOURCES}
)

# Link against the Common object library and compiler options
target_link_libraries(test_runner PRIVATE 
    CubeForge::Common 
    CubeForge::CompilerFlags
    CWSDK
    imgui::imgui
)

# Test-specific private include directories (e.g. for framework files)
target_include_directories(test_runner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/framework
)

target_compile_definitions(test_runner PRIVATE _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_link_libraries(test_runner PRIVATE dbghelp)
    target_compile_definitions(test_runner PRIVATE _ITERATOR_DEBUG_LEVEL=0 MODLOADER=1 CWSDK_NO_GLOBAL_NEW_DELETE=1)
    set_property(TARGET test_runner PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Reuse precompiled headers from CubeForgeLoader_Common
target_precompile_headers(test_runner REUSE_FROM CubeForgeLoader_Common)

include(CTest)
enable_testing()
add_test(NAME AllTests COMMAND test_runner)
