cmake_minimum_required(VERSION 3.25)
project(sonicgenerations LANGUAGES C CXX)

# Windows build of the recompiled game.
#
# Repo layout: <root>/windows (this file), <root>/port (recompiled game),
# <root>/windows/sdk/rexglue-sdk (fetched by ../windows_sdk.sh).
# The generated C++ is committed to the repo, so - unlike the manifest-first
# flow in port/generated/rexglue.cmake - this project never re-runs codegen
# (which would need the original game files).
#
# Toolchain: Clang 18+ is required by the SDK. On a GitHub runner that means
# clang-cl from LLVM (see ../.github/workflows/windows.yml); MSVC proper is
# rejected by the SDK's compiler check.
set(REX_PORT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../port" CACHE PATH "recompiled game dir")
set(REXSDK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/sdk/rexglue-sdk" CACHE PATH "ReXGlue SDK tree")

if(NOT EXISTS "${REXSDK_DIR}/CMakeLists.txt")
    message(FATAL_ERROR "ReXGlue SDK not found at ${REXSDK_DIR} - run windows_sdk.sh first")
endif()
if(NOT EXISTS "${REX_PORT_DIR}/generated/default/sources.cmake")
    message(FATAL_ERROR "no generated code at ${REX_PORT_DIR}/generated/default")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Vulkan backend: the same path the Android port uses and tests. D3D12 stays
# off (it needs extra Windows SDK components for marginal benefit here).
set(REXGLUE_ENABLE_TRACY OFF CACHE BOOL "" FORCE)
set(REXGLUE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(REXGLUE_USE_VULKAN ON CACHE BOOL "" FORCE)
set(REXGLUE_USE_D3D12 OFF CACHE BOOL "" FORCE)

add_subdirectory("${REXSDK_DIR}" rexglue-sdk)
# REXGLUE_SHARE_DIR is published by the SDK for subdirectory consumers.
include("${REXSDK_DIR}/cmake/rexglue_helpers.cmake")

# --- recompiled guest code (pre-generated, committed) -----------------------
set(REX_GEN_DIR "${REX_PORT_DIR}/generated/default")
include("${REX_GEN_DIR}/sources.cmake")

add_library(sonicgenerations_recomp OBJECT ${GENERATED_SOURCES})
target_include_directories(sonicgenerations_recomp PRIVATE
    "${REX_PORT_DIR}" "${REX_PORT_DIR}/src" "${REX_GEN_DIR}")
target_link_libraries(sonicgenerations_recomp PRIVATE rex::runtime)
rexglue_apply_target_settings(sonicgenerations_recomp)
# Guest SEH scopes emit __try/__except, which traps hardware faults only under
# asynchronous EH; the PCH must agree with every TU including it.
if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(REX_RECOMP_EH /EHa)
else()
    set(REX_RECOMP_EH -fasync-exceptions)
endif()
target_precompile_headers(sonicgenerations_recomp PRIVATE
    "${REX_GEN_DIR}/sonicgenerations_pch.h")
target_compile_options(sonicgenerations_recomp PRIVATE ${REX_RECOMP_EH})

# --- host executable ---------------------------------------------------------
add_executable(sonicgenerations "${REX_PORT_DIR}/src/main.cpp")
target_include_directories(sonicgenerations PRIVATE
    "${REX_PORT_DIR}" "${REX_PORT_DIR}/src" "${REX_GEN_DIR}")
target_link_libraries(sonicgenerations PRIVATE sonicgenerations_recomp rex::runtime)
# Adds SDL main + ReXApp sources, stages runtime DLLs and the GPU plugin.
rexglue_configure_target(sonicgenerations GPU_PLUGINS xenos)

# --- install layout (the shipped .zip) ---------------------------------------
install(TARGETS sonicgenerations RUNTIME DESTINATION .)
install(FILES $<TARGET_FILE:rexruntime> $<TARGET_FILE:rexgpu-xenos> DESTINATION .)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/sonicgenerations.toml" DESTINATION .)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/Play.bat" DESTINATION .)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/../BUILD_WINDOWS.md" DESTINATION . RENAME README.md)
