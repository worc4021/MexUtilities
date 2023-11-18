project(MexTests)

matlab_add_mex(
    NAME isfield_mex
    SRC src/isfield.cpp
    LINK_TO MexUtilities
    R2018a
)