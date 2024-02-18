project(MexTests)

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

if (USE_EIGEN)
    matlab_add_mex(
        NAME eigen_mex
        SRC src/eigen.cpp
        LINK_TO MexUtilities
        R2018a
    )
endif()

matlab_add_mex(
    NAME sparse_mex
    SRC src/sparse.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_unit_test(
    NAME mexTest
    UNITTEST_FILE cases.m
    ADDITIONAL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/matlab $<TARGET_FILE_DIR:isfield_mex>
)

matlab_add_unit_test(
    NAME sparseTest
    UNITTEST_FILE sparse_test.m
    ADDITIONAL_PATH ${CMAKE_CURRENT_SOURCE_DIR}/matlab $<TARGET_FILE_DIR:sparse_mex>
)

enable_testing()
find_package(GTest CONFIG REQUIRED)
add_executable(standalone_sparse_test standalone/sparse.cpp)
set_target_properties(standalone_sparse_test 
                        PROPERTIES 
                        CXX_STANDARD 17 
                        CXX_STANDARD_REQUIRED ON 
                        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_link_libraries(standalone_sparse_test MexUtilities GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(standalone_sparse_test)