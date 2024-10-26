# Existing code
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
cmake_minimum_required(VERSION 3.21)
project(MexUtilties
        VERSION 1.0
        DESCRIPTION "Utilities for Matlab mex files"
        LANGUAGES CXX)

include(GNUInstallDirs)
option(BUILD_TESTS "Build tests" ON)
option(USE_EIGEN "Provide eigen3 interfaces" ON)

find_package(Matlab REQUIRED)
find_package(fmt CONFIG REQUIRED)


add_library(MexUtilities INTERFACE)
add_library(MexUtilities::MexUtilities ALIAS MexUtilities)

target_include_directories(MexUtilities INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utilities>
    $<INSTALL_INTERFACE:include/utilities>
)

set(MexUtililtiesLibraries Matlab::mex fmt::fmt-header-only)
if (USE_EIGEN)
    find_package(Eigen3 CONFIG REQUIRED)
    list(APPEND MexUtililtiesLibraries Eigen3::Eigen)
    target_compile_definitions(MexUtilities INTERFACE USE_EIGEN)
endif()

target_link_libraries(MexUtilities INTERFACE ${MexUtililtiesLibraries})

target_compile_features(MexUtilities INTERFACE cxx_std_20)


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
)
install(DIRECTORY utilities/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/utilities)
install(EXPORT MexUtilitiesTargets
      FILE MexUtilities.cmake
      NAMESPACE MexUtilities::
      DESTINATION cmake)