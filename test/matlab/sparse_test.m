classdef sparse_test < baseTest
    
    properties (TestParameter)
        dimensions = {5,50,500,50000}
    end
       
    
    methods(TestMethodSetup)
        function clearMex(~)
            clear sparse_mex
        end
    end
    
    methods(Test)
        % Test methods
        
        function reapeatUpdating(testCase, dimensions)
            A = sprand(dimensions,dimensions,1/dimensions);
            lidxA = find(A);
            A1 = sparse_mex("set",A);

            B = sprand(dimensions, dimensions, 1/dimensions);
            lidxB = find(B);
            lCommon = intersect(lidxA,lidxB);

            C = sparse_mex("update", B);
            
            [iRow,jCol] = find(A);
            [iR,jC] = sparse_mex("find");
            v = sparse_mex("values");

            verifyEqual(testCase,C(lCommon),B(lCommon));
            verifyEqual(testCase,v(ismember(lidxA,lidxB)),full(B(lCommon)))
            verifyEqual(testCase,double(iR+1),iRow);
            verifyEqual(testCase,double(jC+1),jCol);
        end

        function denseTest(testCase, dimensions)
            if dimensions > 1000
                return
            end
            A = sprand(dimensions,dimensions,1/dimensions);
            A1 = full(A);
            A2 = sparse_mex("set",A1);
            
            verifyEqual(testCase,A,A2);
        end
    end
    
end