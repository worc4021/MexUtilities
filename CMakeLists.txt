# Existing code
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
cmake_minimum_required(VERSION 3.25)
project(MexUtilties
        VERSION 1.0
        DESCRIPTION "Utilities for Matlab mex files"
        LANGUAGES CXX)

include(GNUInstallDirs)
option(BUILD_TESTS "Build tests" ON)
option(USE_EIGEN "Provide eigen3 interfaces" ON)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

find_package(Matlab REQUIRED MAT_LIBRARY MAIN_PROGRAM MEX_LIBRARY)

include(FetchContent)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt
  GIT_TAG        11.2.0)
FetchContent_MakeAvailable(fmt)


add_library(MexUtilities INTERFACE)
add_library(MexUtilities::MexUtilities ALIAS MexUtilities)

target_sources(MexUtilities PUBLIC 
    FILE_SET headers
    TYPE HEADERS
    BASE_DIRS 
        utilities
    FILES
        utilities/utilities.hpp
        utilities/sparse.hpp
        utilities/details/BlockData.hpp
        utilities/eigen/conversions.hpp
        utilities/eigen/sparse.hpp
)


set(MexUtilitiesLibraries Matlab::mex fmt::fmt)
if (USE_EIGEN)
    find_package(Eigen3 CONFIG REQUIRED)
    list(APPEND MexUtilitiesLibraries Eigen3::Eigen)
    target_compile_definitions(MexUtilities INTERFACE USE_EIGEN)
endif()

target_link_libraries(MexUtilities INTERFACE ${MexUtilitiesLibraries})

target_compile_features(MexUtilities INTERFACE cxx_std_20)

target_compile_definitions(MexUtilities INTERFACE "MEX_OUTPUT_NAME=$<TARGET_PROPERTY:OUTPUT_NAME>")

if(BUILD_TESTS)
include(CTest)
find_package(GTest CONFIG REQUIRED)
find_package(Matlab REQUIRED COMPONENTS MAIN_PROGRAM)
enable_testing()
add_subdirectory(test)
endif()

install(TARGETS MexUtilities 
    EXPORT MexUtilitiesTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/utilities
)

install(EXPORT MexUtilitiesTargets
      FILE MexUtilities.cmake
      NAMESPACE MexUtilities::
      DESTINATION cmake)