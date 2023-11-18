# Existing code
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)
cmake_minimum_required(VERSION 3.21)
project(MexUtilties
        VERSION 1.0
        DESCRIPTION "Utilities for Matlab mex files"
        LANGUAGES CXX)

find_package(Matlab REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(Eigen3 CONFIG REQUIRED)


add_library(MexUtilities INTERFACE)
target_include_directories(MexUtilities INTERFACE 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/utilities>
    $<INSTALL_INTERFACE:include/utilities>
)
target_link_libraries(MexUtilities INTERFACE Matlab::mex fmt::fmt-header-only)

target_compile_features(MexUtilities INTERFACE cxx_std_17)

install(TARGETS MexUtilities EXPORT MexUtilitiesConfig
    ARCHIVE  DESTINATION lib
    LIBRARY  DESTINATION lib
    RUNTIME  DESTINATION bin
)
install(DIRECTORY utilities/ DESTINATION include/utilities)
install(EXPORT MexUtilitiesConfig DESTINATION share/MexUtilities/cmake)

# Existing code
enable_testing()
add_subdirectory(test)