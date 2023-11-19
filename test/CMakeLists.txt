project(MexTests)

matlab_add_mex(
    NAME isfield
    SRC src/isfield.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME structfields
    SRC src/structfields.cpp
    LINK_TO MexUtilities
    R2018a
)

matlab_add_mex(
    NAME eigen
    SRC src/eigen.cpp
    LINK_TO MexUtilities
    R2018a
)