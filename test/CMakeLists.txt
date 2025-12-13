project(MexTests)

include(CheckCXXSourceCompiles)

check_cxx_source_compiles("#include <concepts>
int main() {
    return 0;
}" 
HAVE_CPP20)

if(NOT NO_MATLAB)
get_target_property(Matlab_Mex_Path Matlab::mex IMPORTED_LOCATION)
cmake_path(GET Matlab_Mex_Path PARENT_PATH Matlab_Library_DIR)


matlab_add_mex(
    NAME isfield_mex
    SRC src/isfield.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME structfields_mex
    SRC src/structfields.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME printf_mex
    OUTPUT_NAME printf
    SRC src/printf.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME warnanderror_mex
    OUTPUT_NAME warnanderror
    SRC src/warningsanderror.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME mex_name
    SRC src/mexName.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME string_example
    SRC src/stringexample.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME movefield
    SRC src/movefield.cpp
    LINK_TO MexUtilities
    R2018a
)
target_compile_features(movefield PRIVATE cxx_std_23)

matlab_add_mex(
    NAME multifile
    SRC src/multifile1.cpp
        src/multifile2.cpp
    LINK_TO MexUtilities
    R2018a
)
target_include_directories(multifile PRIVATE src)

matlab_add_mex(
    NAME page_times
    SRC src/pagetimes.cpp
    LINK_TO MexUtilities $<IF:$<PLATFORM_ID:Windows>,libmwblas.lib,mwblas>
    R2018a
)

target_link_directories(page_times PRIVATE ${Matlab_Library_DIR})


matlab_add_mex(
    NAME catandpagetimes
    SRC src/catandpagetimes.cpp
    LINK_TO MexUtilities $<IF:$<PLATFORM_ID:Windows>,libmwblas.lib,mwblas>
    R2018a
)

target_link_directories(catandpagetimes PRIVATE ${Matlab_Library_DIR})


target_compile_features(catandpagetimes PRIVATE cxx_std_23)

matlab_add_mex(
    NAME ranges
    SRC src/ranges.cpp
    LINK_TO MexUtilities
    R2018a
)
target_compile_features(ranges PRIVATE cxx_std_23)

matlab_add_mex(
    NAME catandpagetimesRanges
    SRC src/catandpagetimesranges.cpp
    LINK_TO MexUtilities $<IF:$<PLATFORM_ID:Windows>,libmwblas.lib,mwblas>
    R2018a
)

target_link_directories(catandpagetimesRanges PRIVATE ${Matlab_Library_DIR})

target_compile_features(catandpagetimesRanges PRIVATE cxx_std_23)

if (USE_EIGEN)
    matlab_add_mex(
        NAME eigen_mex
        SRC src/eigen.cpp
        LINK_TO MexUtilities
        R2018a
    )

    matlab_add_mex(
        NAME eigen_sparse_mex
        SRC src/sparse_eigen.cpp
        LINK_TO MexUtilities
        R2018a
    )
endif(USE_EIGEN)


matlab_add_unit_test(
    NAME mexTest
    UNITTEST_FILE cases.m
    ADDITIONAL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/matlab $<TARGET_FILE_DIR:isfield_mex>
    )

endif(NOT NO_MATLAB)    
    
if (HAVE_CPP20)

if(NOT NO_MATLAB)
    matlab_add_mex(
        NAME sparse_mex
        SRC src/sparse.cpp
        LINK_TO MexUtilities
        R2018a
    )

    matlab_add_unit_test(
        NAME sparseTest
        UNITTEST_FILE sparse_test.m
        ADDITIONAL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/matlab $<TARGET_FILE_DIR:sparse_mex>
    )
endif(NOT NO_MATLAB)
    add_executable(standalone_sparse_test standalone/sparse.cpp)
    target_link_libraries(standalone_sparse_test MexUtilities GTest::gtest_main)
    
    add_executable(standlone_blockdata_test standalone/blockdata.cpp)
    target_link_libraries(standlone_blockdata_test MexUtilities GTest::gtest_main)
    target_compile_features(standlone_blockdata_test PRIVATE cxx_std_23)

    add_executable(standalone_views_test standalone/views.cpp)
    target_link_libraries(standalone_views_test fmt::fmt)
    target_compile_features(standalone_views_test PRIVATE cxx_std_23)

    include(GoogleTest)
    gtest_discover_tests(standalone_sparse_test DISCOVERY_MODE PRE_TEST)
    gtest_discover_tests(standlone_blockdata_test DISCOVERY_MODE PRE_TEST)
        
endif(HAVE_CPP20)

