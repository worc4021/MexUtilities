classdef cases < baseTest
    
    methods(Test)
        function isfieldTest(testCase)
            tests = {struct('a',1),'a';...
                     struct('a',1),'b';...
                     struct('a',1),"a";...
                     struct('a',1),"b"};
            for i = 1:size(tests,1)
                testCase.verifyEqual(...
                    isfield(tests{i,1},tests{i,2}),...
                    isfield_mex(tests{i,1},tests{i,2}));
            end
        end

        function mexNameTest(testCase)
            ref = string(strrep(which('mex_name'),['.',mexext()],''));
            testPath = mex_name();
            testCase.verifyEqual(testPath,ref, 'Mex could not determine its own name successfully');
        end

        function structFields(testCase)
            tests = {struct('a',1),-1;...
                     struct('toplevel',1),-1;...
                     struct('toplevel',struct('a',1)),-1;...
                     struct('toplevel',struct('nested','hello')),0};
            for i = 1:size(tests,1)
                if tests{i,2} == -1
                    testCase.verifyError(...
                        @()structfields_mex(tests{i,1}),'STRUCTFIELDS_MEX:getfield','Failed in the wrong way');
                else
                    testCase.verifyReturnsTrue(...
                        @()structfields_mex(tests{i,1}),'Failed to pull predefined substructure');
                end
            end
        end

        function isField(testCase)
            s = struct;
            s.a = 1;
            s.b = struct;
            s.b.c = 2;
            testCase.verifyReturnsTrue(@()isfield_mex(s,'a'),'Could not find existing field with char fieldname')
            testCase.verifyReturnsTrue(@()isfield_mex(s,"b.c"),'Could not find exsting nested sequence with string fieldname')
            testCase.verifyReturnsTrue(@()~isfield_mex(s,'c'),'Returns true even when it should not');
        end
        

        function printf(testCase)
            testCase.verifyWarningFree(@()printf(),'Produced warnings or errors during printf')
        end

        function warnAndError(testCase)
            testCase.verifyWarning(@()warnanderror("warn"),'WARNANDERROR:unspecific','Produced the wrong kind of warning or no warning at all')
            testCase.verifyWarning(@()warnanderror("wspec"),'WARNANDERROR:specific','Produced the wrong kind of warning or no warning at all')
            testCase.verifyError(@()warnanderror("err"),'WARNANDERROR:unspecific','Produced the wrong kind of error or no warning at all')
            testCase.verifyError(@()warnanderror("espec"),'WARNANDERROR:specific','Produced the wrong kind of error or no warning at all')
        end

        function stringTransforms(testCase)
            testCase.verifyWarningFree(@()string_example,'Failed to do some STL string transformations');
        end

        function multifile(testCase)
            testCase.verifyWarningFree(@()multifile(1,2),'Failed running multifile mex')
        end

        function pagetimes(testCase)
            n = 5;
            m = 4;
            k = 7;
            p = 5;
            A = rand(n,k,p);
            B = rand(k,m,p);
            C = pagemtimes(A,B);
            Cp = page_times(A,B);
            testCase.verifyLessThan(norm(C(:)-Cp(:),inf),1e-12,'Pagewise multiplication lead to error')
        end

        function ranges(testCase)
            n = 5;
            k = 7;
            p = 2;
            A = rand(n,k,p);
            B = ranges(A);
            testCase.verifyTrue(all(B(1,[1:3,5:end],1)==1),'Something went wrong assigning initial set of ones');
            testCase.verifyTrue(all(B(:,4,1)==4),'Something went wrong assigning the column of 4s')
            testCase.verifyTrue(all(B(3,:,2)==5),'Failed to assign row of 5s');
            testCase.verifyTrue(all(B(2,1,:)==6),'Failed to assign in tensor direction');
        end

    end
    
end