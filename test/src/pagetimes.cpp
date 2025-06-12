#include "mex.hpp"
#include "mexAdapter.hpp"
#include "utilities.hpp"
#include "blas.h"

class MexFunction 
    : public matlab::mex::Function 
{

public:
    MexFunction()  {
            matlabPtr = getEngine();
    }
    ~MexFunction() = default;
    void operator()([[maybe_unused]]matlab::mex::ArgumentList outputs, matlab::mex::ArgumentList inputs) {
        
        if (inputs.size() != 2) {
            utilities::error("Usage: page_times(A, B)\n");
        }

        matlab::data::TypedArray<double> A = std::move(inputs[0]);
        matlab::data::TypedArray<double> B = std::move(inputs[1]);

        auto Asz = A.getDimensions();
        auto Bsz = B.getDimensions();

        if (Asz[1] != Bsz[0]) {
            utilities::error("Matrix dimensions must agree.\n");
        }
        auto aIsPaged = Asz.size() > 2 && Asz[2] > 1;
        auto bIsPaged = Bsz.size() > 2 && Bsz[2] > 1;
        std::size_t nPages{1};
        
        if (aIsPaged && bIsPaged) {
            if (Asz[2] != Bsz[2]) {
                utilities::error("Number of pages must agree.\n");
            }
        }
        if (aIsPaged) {
            nPages = Asz[2];
        } else if (bIsPaged) {
            nPages = Bsz[2];
        }
        matlab::data::ArrayFactory factory;
        matlab::data::buffer_ptr_t<double> C_ptr = factory.createBuffer<double>(Asz[0]*Bsz[1]*nPages);
        matlab::data::buffer_ptr_t<double> A_ptr = A.release();
        matlab::data::buffer_ptr_t<double> B_ptr = B.release();

        const char transa = 'N';
        const char transb = 'N';
        ptrdiff_t m = Asz[0];
        ptrdiff_t k = Asz[1];
        ptrdiff_t n = Bsz[1];
        ptrdiff_t lda = Asz[0];
        ptrdiff_t ldb = Bsz[0];
        double alpha = 1.0;
        double beta = 0.0;
        if (aIsPaged && bIsPaged) {
            for (std::size_t iPage = 0; iPage < nPages; ++iPage)
                dgemm(&transa, &transb, &m, &n, &k, &alpha, 
                      A_ptr.get() + iPage*m*k, &lda, B_ptr.get() + iPage*n*k, &ldb, &beta,
                      C_ptr.get() + iPage*m*n, &lda);
        }
        else if (aIsPaged) {
            for (std::size_t iPage = 0; iPage < nPages; iPage++) 
                dgemm(&transa, &transb, &m, &n, &k, &alpha, 
                      A_ptr.get() + iPage*m*k, &lda, B_ptr.get(), &ldb, &beta,
                      C_ptr.get() + iPage*m*n, &lda);
        } else if (bIsPaged) {
            for (std::size_t iPage = 0; iPage < nPages; iPage++) 
                dgemm(&transa, &transb, &m, &n, &k, &alpha, 
                      A_ptr.get(), &lda, B_ptr.get() + iPage*n*k, &ldb, &beta,
                      C_ptr.get() + iPage*m*n, &lda);
        } else {
            
            dgemm(&transa, &transb, &m, &n, &k, &alpha, 
                  A_ptr.get(), &lda, B_ptr.get(), &ldb, &beta,
                  C_ptr.get(), &lda);
        }

        outputs[0] = factory.createArrayFromBuffer<double>({Asz[0], Bsz[1], nPages}, std::move(C_ptr));
    }
};