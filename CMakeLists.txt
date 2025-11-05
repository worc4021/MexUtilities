cmake_minimum_required(VERSION 3.25)
project(MexUtilities
        VERSION 1.0
        DESCRIPTION "Utilities for Matlab mex files"
        LANGUAGES CXX)

include(GNUInstallDirs)
option(BUILD_TESTS "Build tests" ON)
option(USE_EIGEN "Provide eigen3 interfaces" ON)
option(NO_MATLAB "Supress use of mexes etc" OFF)
option(MEXUTILITIES_INSTALL_DEPS "Install fetched dependencies (fmt, eigen3)" OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT NO_MATLAB)
set(MATLAB_ADDITIONAL_VERSION 
    "R2024b=24.2"
    "R2025a=25.1"
    "R2025b=25.2"
)
    if(NOT DEFINED Matlab_FOUND)
    find_package(Matlab REQUIRED)
    endif()
matlab_get_release_name_from_version("${Matlab_VERSION}" Matlab_RELEASE_NAME)
string(REGEX REPLACE "R([0-9]+)[ab]" "\\1" MATLAB_YEAR "${Matlab_RELEASE_NAME}")
message(STATUS "Found Matlab version ${Matlab_RELEASE_NAME}, year ${MATLAB_YEAR}")
endif()

include(FetchContent)

# Handle fmt dependency with version compatibility
set(MEXUTILITIES_MIN_FMT_VERSION "10.0" CACHE STRING "Minimum required fmt version")

# First, check if fmt target already exists (from parent project or previous FetchContent)
if(TARGET fmt::fmt)
    message(STATUS "fmt target already exists, using existing version")
    get_target_property(FMT_INCLUDE_DIRS fmt::fmt INTERFACE_INCLUDE_DIRECTORIES)
    if(FMT_INCLUDE_DIRS)
        # Try to determine version from existing target
        message(STATUS "Using existing fmt::fmt target")
    endif()
else()
    # Try to find fmt via find_package first
    find_package(fmt ${MEXUTILITIES_MIN_FMT_VERSION} QUIET)
    
    if(NOT fmt_FOUND)
        message(STATUS "fmt not found or version < ${MEXUTILITIES_MIN_FMT_VERSION}, fetching compatible version")
        
        # Check if someone else already declared fmtlib in FetchContent
        FetchContent_GetProperties(fmtlib)
        if(NOT fmtlib_POPULATED)
            FetchContent_Declare(
                fmtlib
                GIT_REPOSITORY https://github.com/fmtlib/fmt
                GIT_TAG        12.1.0
            )
            
            # Only set FMT_INSTALL if we're the top-level project or explicitly requested
            if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR OR MEXUTILITIES_INSTALL_DEPS)
                set(FMT_INSTALL ON CACHE BOOL "" FORCE)
            else()
                set(FMT_INSTALL OFF CACHE BOOL "" FORCE)
            endif()
            
            FetchContent_MakeAvailable(fmtlib)
        else()
            message(STATUS "fmtlib already populated by another project")
        endif()
    else()
        message(STATUS "fmt found ${fmt_VERSION}")
    endif()
endif()

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
        utilities/details/blockdata.hpp
        utilities/eigen/conversions.hpp
        utilities/eigen/sparse.hpp
)


set(MexUtilitiesLibraries fmt::fmt)
if(NOT NO_MATLAB)
list(APPEND MexUtilitiesLibraries Matlab::mex)
endif(NOT NO_MATLAB)
if (USE_EIGEN)
    find_package(Eigen3 QUIET)
    if(NOT Eigen3_FOUND)
        FetchContent_Declare(
            eigen3
            GIT_REPOSITORY https://gitlab.com/libeigen/eigen
            GIT_TAG 3.4
        )
        FetchContent_MakeAvailable(eigen3)
        find_package(Eigen3 REQUIRED)
    endif()
    list(APPEND MexUtilitiesLibraries Eigen3::Eigen)
    target_compile_definitions(MexUtilities INTERFACE USE_EIGEN)
endif(USE_EIGEN)

target_link_libraries(MexUtilities INTERFACE ${MexUtilitiesLibraries})

target_compile_features(MexUtilities INTERFACE cxx_std_20)

if (MATLAB_YEAR LESS 2025)
target_compile_definitions(MexUtilities INTERFACE REDUCED_TYPES)
message(STATUS "Using REDUCED_TYPES for Matlab versions before 2025")
endif()

if(BUILD_TESTS)
include(CTest)
find_package(GTest QUIET)
if (NOT GTest_FOUND)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG v1.17.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
endif()

if(NOT NO_MATLAB)
find_package(Matlab REQUIRED COMPONENTS MAIN_PROGRAM)
endif(NOT NO_MATLAB)
enable_testing()
add_subdirectory(test)
endif(BUILD_TESTS)

# Install MexUtilities and its dependencies
# Only install fmt if we fetched it AND we're either top-level or explicitly requested
if(TARGET fmt AND CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR OR MEXUTILITIES_INSTALL_DEPS)
    # Check if fmt was created by us (FetchContent) vs found externally
    get_target_property(FMT_SOURCE_DIR fmt::fmt SOURCE_DIR)
    if(FMT_SOURCE_DIR)
        # fmt was fetched, so we can install it
        install(TARGETS fmt
            EXPORT MexUtilitiesTargets
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()

if(USE_EIGEN AND NOT Eigen3_FOUND)
    # If we fetched Eigen3, install it too
    install(TARGETS eigen
        EXPORT MexUtilitiesTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

install(TARGETS MexUtilities
    EXPORT MexUtilitiesTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    FILE_SET headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/utilities
)

install(EXPORT MexUtilitiesTargets
      FILE MexUtilitiesTargets.cmake
      NAMESPACE MexUtilities::
      DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MexUtilities)

# Create and install package configuration files
include(CMakePackageConfigHelpers)

# Create MexUtilitiesConfig.cmake
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/MexUtilitiesConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/MexUtilitiesConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MexUtilities
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR
)

# Create MexUtilitiesConfigVersion.cmake
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/MexUtilitiesConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the config files
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/MexUtilitiesConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/MexUtilitiesConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/MexUtilities
)