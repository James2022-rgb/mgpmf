cmake_minimum_required(VERSION 3.22)

# --------------------------------------------------------------------------------
# Internal variables
#

set(TARGET_NAME "mgpmf")
set(SRC_DIR "src")

# --------------------------------------------------------------------------------
# Project
#

project(${TARGET_NAME}
  VERSION 0.1.0.0
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=thread-safety")
endif()

if(NOT TARGET mbase)
  message(FATAL_ERROR "mgpmf: required target 'mbase' not defined. Add it before mgpmf.")
endif()

# --------------------------------------------------------------------------------
# jgpmf_capi (Rust crate from James2022-rgb/gpmf_tools, built via cargo)
#
# Mirrors the vidsynt integration in mnexus: FetchContent the upstream Rust
# workspace, drive `cargo build -p gpmf_capi --release` from a custom
# command so the static library lands at a predictable path, then expose
# it as an IMPORTED static target with the cbindgen-generated header.
#

if(NOT TARGET jgpmf_capi)
  find_program(CARGO_EXECUTABLE cargo REQUIRED)

  include(FetchContent)

  # SHA-pinned for reproducibility. Bump when you intentionally
  # want a newer gpmf_tools.
  FetchContent_Declare(gpmf_tools
    GIT_REPOSITORY https://github.com/James2022-rgb/gpmf_tools.git
    GIT_TAG        2b6e57237ceb069603f96cb34ebacec1df3d6ab8
  )
  FetchContent_GetProperties(gpmf_tools)
  if(NOT gpmf_tools_POPULATED)
    FetchContent_Populate(gpmf_tools)
  endif()

  set(_gpmf_tools_dir "${gpmf_tools_SOURCE_DIR}")
  set(_jgpmf_capi_lib "${_gpmf_tools_dir}/target/release/jgpmf_capi.lib")

  # Re-run cargo whenever any of gpmf_tools's Cargo.toml / Cargo.lock /
  # .rs sources change. cargo's own incremental compile keeps the cost
  # of a no-op rebuild small.
  file(GLOB_RECURSE _gpmf_tools_sources CONFIGURE_DEPENDS
    "${_gpmf_tools_dir}/Cargo.toml"
    "${_gpmf_tools_dir}/Cargo.lock"
    "${_gpmf_tools_dir}/crates/*/Cargo.toml"
    "${_gpmf_tools_dir}/crates/*/src/*.rs"
    "${_gpmf_tools_dir}/crates/*/build.rs"
    "${_gpmf_tools_dir}/crates/*/cbindgen.toml"
  )

  add_custom_command(
    OUTPUT  ${_jgpmf_capi_lib}
    COMMAND ${CARGO_EXECUTABLE} build -p gpmf_capi --release
    WORKING_DIRECTORY ${_gpmf_tools_dir}
    DEPENDS ${_gpmf_tools_sources}
    COMMENT "Building gpmf_tools (cargo build -p gpmf_capi --release)"
    VERBATIM
  )
  add_custom_target(jgpmf_capi_build DEPENDS ${_jgpmf_capi_lib})
  set_target_properties(jgpmf_capi_build PROPERTIES FOLDER "external/gpmf_tools")

  add_library(jgpmf_capi STATIC IMPORTED GLOBAL)
  set_target_properties(jgpmf_capi PROPERTIES
    IMPORTED_LOCATION             "${_jgpmf_capi_lib}"
    INTERFACE_INCLUDE_DIRECTORIES "${_gpmf_tools_dir}/crates/gpmf_capi/include"
    # System libraries the Rust standard library / runtime depend on
    # on Windows. Match the link line documented in the upstream repo.
    INTERFACE_LINK_LIBRARIES      "ws2_32;userenv;ntdll;bcrypt;advapi32"
  )
  add_dependencies(jgpmf_capi jgpmf_capi_build)
endif()

# --------------------------------------------------------------------------------
# project target
#

add_library(${TARGET_NAME} STATIC)
target_compile_features(${TARGET_NAME} PRIVATE cxx_std_23)

set(SOURCES_PRIVATE_ROOT_DIR "${SRC_DIR}/${TARGET_NAME}/private")
set(SOURCES_PUBLIC_ROOT_DIR  "${SRC_DIR}/${TARGET_NAME}/public")

set(SOURCES_MGPMF_PUBLIC
  ${SOURCES_PUBLIC_ROOT_DIR}/gpmf_sample.h
)
source_group("Public" FILES ${SOURCES_MGPMF_PUBLIC})

set(SOURCES_MGPMF_PRIVATE
  ${SOURCES_PRIVATE_ROOT_DIR}/gpmf_sample.cpp
)
source_group("Private" FILES ${SOURCES_MGPMF_PRIVATE})

target_sources(${TARGET_NAME} PRIVATE
  ${SOURCES_MGPMF_PUBLIC}
  ${SOURCES_MGPMF_PRIVATE}
)

target_include_directories(${TARGET_NAME} PUBLIC
  ${SRC_DIR}
)

target_include_directories(${TARGET_NAME} PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${SOURCES_PRIVATE_ROOT_DIR}
)

target_link_libraries(${TARGET_NAME} PUBLIC
  mbase
)
target_link_libraries(${TARGET_NAME} PRIVATE
  jgpmf_capi
)
