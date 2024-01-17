classdef sparse_test < matlab.unittest.TestCase
    
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
            
            verifyEqual(testCase,C(lCommon),B(lCommon));
        end
    end
    
end