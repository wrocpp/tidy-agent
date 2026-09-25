cmake_minimum_required(VERSION 3.20)
# C is enabled only because LLVMConfig.cmake probes C headers (FindLibEdit).
project(wrocpp_tidy_plugin LANGUAGES C CXX)

# Build against the LLVM whose clang-tidy will load the plugin (ADR-0002):
#   cmake -S checks/plugin -B build -DLLVM_DIR=$(llvm-config --cmakedir)
find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG HINTS "${LLVM_DIR}/../clang")
message(STATUS "Building the wrocpp clang-tidy plugin for LLVM ${LLVM_PACKAGE_VERSION}")

if(LLVM_VERSION_MAJOR LESS 22)
  message(FATAL_ERROR "tidy-agent needs LLVM 22 or later, found ${LLVM_PACKAGE_VERSION}")
endif()

add_library(WrocppTidyModule MODULE
  src/WrocppModule.cpp
  src/DefaultThenAssignCheck.cpp
  src/EscapingRefCaptureCheck.cpp
  src/NeedlessSharedPtrCheck.cpp
  src/HeaderCommentPolicyCheck.cpp
)
target_compile_features(WrocppTidyModule PRIVATE cxx_std_17)
target_include_directories(WrocppTidyModule SYSTEM PRIVATE ${LLVM_INCLUDE_DIRS} ${CLANG_INCLUDE_DIRS})
target_compile_definitions(WrocppTidyModule PRIVATE ${LLVM_DEFINITIONS})
if(NOT LLVM_ENABLE_RTTI)
  target_compile_options(WrocppTidyModule PRIVATE -fno-rtti)
endif()

# The plugin resolves clang and clang-tidy symbols from the clang-tidy
# executable that loads it, so it links against nothing.
if(APPLE)
  target_link_options(WrocppTidyModule PRIVATE -undefined dynamic_lookup)
endif()
set_target_properties(WrocppTidyModule PROPERTIES PREFIX "lib" OUTPUT_NAME "wrocpp-tidy")
