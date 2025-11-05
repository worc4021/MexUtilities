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
    
    # Only find Matlab if not already found
    if(NOT DEFINED Matlab_FOUND OR NOT Matlab_FOUND)
        find_package(Matlab REQUIRED)
    endif()
    
    # Ensure we have the required Matlab variables
    if(Matlab_FOUND AND Matlab_VERSION)
        matlab_get_release_name_from_version("${Matlab_VERSION}" Matlab_RELEASE_NAME)
        string(REGEX REPLACE "R([0-9]+)[ab]" "\\1" MATLAB_YEAR "${Matlab_RELEASE_NAME}")
        message(STATUS "Found Matlab version ${Matlab_RELEASE_NAME}, year ${MATLAB_YEAR}")
    else()
        message(WARNING "Matlab found but version information not available")
        set(MATLAB_YEAR 2025)  # Default to current year to avoid issues
    endif()
endif()

include(FetchContent)

# Handle fmt dependency with version compatibility
set(MEXUTILITIES_MIN_FMT_VERSION "10.0" CACHE STRING "Minimum required fmt version")

# First, check if fmt target already exists (from parent project or previous FetchContent)
if(TARGET fmt::fmt)
    message(STATUS "fmt::fmt target already exists, using existing version")
    
    # Try to get version information if available
    get_target_property(FMT_VERSION fmt::fmt VERSION)
    if(FMT_VERSION)
        message(STATUS "Using existing fmt::fmt target version ${FMT_VERSION}")
        if(FMT_VERSION VERSION_LESS ${MEXUTILITIES_MIN_FMT_VERSION})
            message(WARNING "Existing fmt version ${FMT_VERSION} is less than required minimum ${MEXUTILITIES_MIN_FMT_VERSION}")
        endif()
    else()
        message(STATUS "Using existing fmt::fmt target (version unknown)")
    endif()
    
    set(fmt_FOUND TRUE)
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
            message(STATUS "fmt fetched and made available")
        else()
            message(STATUS "fmtlib already populated by another project")
        endif()
    else()
        message(STATUS "Found system fmt ${fmt_VERSION}")
    endif()
endif()

# Ensure we have the fmt::fmt target available
if(NOT TARGET fmt::fmt)
    message(FATAL_ERROR "fmt::fmt target not available after dependency resolution")
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
if(USE_EIGEN)
    # First check if Eigen3::Eigen target already exists
    if(TARGET Eigen3::Eigen)
        message(STATUS "Eigen3::Eigen target already exists, using existing version")
        set(Eigen3_FOUND TRUE)
    else()
        find_package(Eigen3 QUIET)
        if(NOT Eigen3_FOUND)
            message(STATUS "Eigen3 not found, fetching from source")
            FetchContent_Declare(
                eigen3
                GIT_REPOSITORY https://gitlab.com/libeigen/eigen
                GIT_TAG 3.4
            )
            FetchContent_MakeAvailable(eigen3)
            # After FetchContent, the target should be available
            if(TARGET Eigen3::Eigen)
                set(Eigen3_FOUND TRUE)
            else()
                message(FATAL_ERROR "Failed to make Eigen3::Eigen target available")
            endif()
        else()
            message(STATUS "Found system Eigen3")
        endif()
    endif()
    
    list(APPEND MexUtilitiesLibraries Eigen3::Eigen)
    target_compile_definitions(MexUtilities INTERFACE USE_EIGEN)
endif()

target_link_libraries(MexUtilities INTERFACE ${MexUtilitiesLibraries})

target_compile_features(MexUtilities INTERFACE cxx_std_20)

if(NOT NO_MATLAB AND DEFINED MATLAB_YEAR AND MATLAB_YEAR LESS 2025)
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

# Only install when MexUtilities is the top-level project or installation is explicitly requested
if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR OR MEXUTILITIES_INSTALL_DEPS)
    message(STATUS "MexUtilities: Installing targets and exports")
else()
    message(STATUS "MexUtilities: Skipping install targets (used via FetchContent)")
    
    # Install fmt if we fetched it
    if(TARGET fmt::fmt)
        # Check if fmt was created by us (FetchContent) vs found externally
        get_target_property(FMT_SOURCE_DIR fmt::fmt SOURCE_DIR)
        get_target_property(FMT_BINARY_DIR fmt::fmt BINARY_DIR)
        
        # Only install if fmt was fetched (has SOURCE_DIR and BINARY_DIR from FetchContent)
        if(FMT_SOURCE_DIR AND FMT_BINARY_DIR AND EXISTS "${FMT_SOURCE_DIR}/CMakeLists.txt")
            # Check if fmt target is installable (not an imported target)
            get_target_property(FMT_IMPORTED fmt::fmt IMPORTED)
            if(NOT FMT_IMPORTED)
                install(TARGETS fmt::fmt
                    EXPORT MexUtilitiesTargets
                    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                )
            endif()
        endif()
    endif()

    # Install Eigen3 if we fetched it
    if(USE_EIGEN AND TARGET Eigen3::Eigen)
        # Check if Eigen3 was fetched by us
        get_target_property(EIGEN_SOURCE_DIR Eigen3::Eigen SOURCE_DIR)
        get_target_property(EIGEN_IMPORTED Eigen3::Eigen IMPORTED)
        
        if(EIGEN_SOURCE_DIR AND NOT EIGEN_IMPORTED)
            # Only install eigen if it was fetched and is installable
            if(TARGET eigen)
                install(TARGETS eigen
                    EXPORT MexUtilitiesTargets
                    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
                    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
                    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
                )
            endif()
        endif()
    endif()

    # Install MexUtilities
    install(TARGETS MexUtilities
        EXPORT MexUtilitiesTargets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        FILE_SET headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/utilities
    )

    # Install the export set
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
    
endif()