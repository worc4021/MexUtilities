cmake_minimum_required(VERSION 3.25)
project(MexUtilities
        VERSION 1.0
        DESCRIPTION "Utilities for Matlab mex files"
        LANGUAGES CXX)

include(GNUInstallDirs)
option(BUILD_TESTS "Build tests" ON)
option(USE_EIGEN "Provide eigen3 interfaces" ON)
option(NO_MATLAB "Supress use of mexes etc" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT NO_MATLAB)
find_package(Matlab REQUIRED MAT_LIBRARY MAIN_PROGRAM MEX_LIBRARY)
string(REGEX REPLACE "([0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)" "\\1" MATLAB_YEAR "${Matlab_VERSION}")
    
endif()

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


set(MexUtilitiesLibraries fmt::fmt)
if(NOT NO_MATLAB)
list(APPEND MexUtiliesLibraries Matlab::mex)
endif(NOT NO_MATLAB)
if (USE_EIGEN)
    FetchContent_Declare(
        eigen3
        GIT_REPOSITORY https://gitlab.com/libeigen/eigen
        GIT_TAG 3.4
    )
    FetchContent_MakeAvailable(eigen3)
    list(APPEND MexUtilitiesLibraries eigen3)
    target_compile_definitions(MexUtilities INTERFACE USE_EIGEN)
endif(USE_EIGEN)

target_link_libraries(MexUtilities INTERFACE ${MexUtilitiesLibraries})

target_compile_features(MexUtilities INTERFACE cxx_std_20)

if (MATLAB_YEAR LESS 25)
target_compile_definitions(MexUtilities INTERFACE REDUCED_TYPES)
endif()

if(BUILD_TESTS)
include(CTest)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

if(NOT NO_MATLAB)
find_package(Matlab REQUIRED COMPONENTS MAIN_PROGRAM)
endif(NOT NO_MATLAB)
enable_testing()
add_subdirectory(test)
endif(BUILD_TESTS)

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