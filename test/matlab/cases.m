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
        

        function getNestedVectors(testCase)
            % A subscript selects an element of the struct array the segment
            % names; the field of the *following* segment is read from it.
            s = struct;
            s.hello.world = 42;
            s.foo = struct('bar',{1,2});
            s.bar.baz = struct('boom',{10,20});

            testCase.verifyEqual(getnested(s,'hello.world'),42,'Plain nested field');
            testCase.verifyEqual(getnested(s,'foo[0].bar'),1,'Zero based index into a struct array');
            testCase.verifyEqual(getnested(s,'foo[1].bar'),2,'Zero based index into a struct array');
            testCase.verifyEqual(getnested(s,'bar.baz[1].boom'),20,'Index below the top level');
            testCase.verifyEqual(getnested(s,'foo[1].bar',true),1,'One based (modelica) index');
            testCase.verifyEqual(getnested(s,'foo[2].bar',true),2,'One based (modelica) index');
        end

        function getNestedMatrices(testCase)
            % Matrices of substructures: one subscript per dimension.
            ll = repmat(struct('v',0),[2 3]);
            for i = 1:2
                for j = 1:3
                    ll(i,j).v = 10*i + j;
                end
            end
            s = struct;
            s.boat.boardP.liftingLine = ll;

            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine[1,1].v',true),11,'First element of a matrix of substructures');
            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine[2,3].v',true),23,'Last element of a matrix of substructures');
            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine[1,2].v',true),12,'Column subscript must not be ignored');
            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine[0,1].v',false),12,'Zero based matrix subscripts');

            % A single subscript stays a column major linear index whatever
            % the shape of the struct array is.
            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine[3].v',false),ll(4).v,'Linear index into a matrix of substructures');

            % Three dimensional arrays of substructures
            t = repmat(struct('v',0),[2 2 2]);
            for i = 1:8
                t(i).v = i;
            end
            s.tensor = t;
            testCase.verifyEqual(getnested(s,'tensor[2,2,2].v',true),8,'Three dimensional array of substructures');
            testCase.verifyEqual(getnested(s,'tensor[1,0,1].v',false),t(2,1,2).v,'Zero based three dimensional subscripts');

            % An unsubscripted leaf of a matrix of substructures reads from the
            % first element.
            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine.v'),11,'Leaf of a matrix of substructures defaults to the first element');

            % Matlab drops trailing singleton dimensions, so a modelica
            % [i,j,1] addressing a matlab i x j array has to resolve.
            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine[2,3,1].v',true),23,'Trailing singleton dimension');

            % A subscript on the last segment addresses the leaf value itself.
            testCase.verifyEqual(getnested(s,'boat.boardP.liftingLine[2,3]',true),ll(2,3),'A subscripted struct array leaf returns that substructure');
        end

        function getNestedLeafSubscripts(testCase)
            % A subscript on the leaf selects that single element of the leaf
            % value rather than returning the whole array.
            s = struct;
            s.mat.values = [11 12 13; 21 22 23];
            s.mat.flags = [true false; false true];
            s.mat.text = 'abcd';
            s.mat.cells = {1, 'two'; 3, 'four'};

            testCase.verifyEqual(getnested(s,'mat.values'),[11 12 13; 21 22 23],'An unsubscripted leaf is returned whole');
            testCase.verifyEqual(getnested(s,'mat.values[2,3]',true),23,'One based leaf subscript');
            testCase.verifyEqual(getnested(s,'mat.values[1,2]',false),23,'Zero based leaf subscript');
            testCase.verifyEqual(getnested(s,'mat.values[3]',false),22,'Column major linear leaf subscript');
            testCase.verifyEqual(getnested(s,'mat.flags[2,2]',true),true,'Logical leaf');
            testCase.verifyEqual(getnested(s,'mat.text[3]',true),'c','Char leaf');
            testCase.verifyEqual(getnested(s,'mat.cells[1,2]',true),{'two'},'Cell leaf');

            testCase.verifyError(@()getnested(s,'mat.values[3,1]',true),...
                'GETNESTED:unspecific','Leaf subscript out of bounds must be reported');
        end

        function getNestedErrors(testCase)
            s = struct;
            s.boat.boardP.liftingLine = repmat(struct('v',0),[2 3]);
            s.notastruct.leaf = 1;
            s.notastruct = struct('cellfield',{{struct('v',1),struct('v',2)}});

            testCase.verifyError(@()getnested(s,'boat.nosuchfield.v'),...
                'GETNESTED:unspecific','Unknown field must be reported');
            testCase.verifyError(@()getnested(s,'boat.boardP.liftingLine[3,1].v',true),...
                'GETNESTED:unspecific','Subscript out of bounds must be reported');
            testCase.verifyError(@()getnested(s,'boat.boardP.liftingLine[6].v'),...
                'GETNESTED:unspecific','Linear index out of bounds must be reported');
            testCase.verifyError(@()getnested(s,'boat.boardP.liftingLine[1,1,2].v',true),...
                'GETNESTED:unspecific','A padded trailing dimension is still bounds checked');
            % Descending into a non-struct used to reinterpret_cast the
            % reference, which crashed MATLAB outright.
            testCase.verifyError(@()getnested(s,'notastruct.cellfield.v'),...
                'GETNESTED:unspecific','Descending into a non-struct must error rather than crash');
            testCase.verifyError(@()getnested(s,'boat.boardP.liftingLine[1,x].v'),...
                'GETNESTED:unspecific','Malformed subscript must be reported');
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
            if ispc
                testCase.verifyWarningFree(@()multifile(1,2),'Failed running multifile mex')
            else
                testCase.verifyWarningFree(@()multifile(3,2),'No Printout should mean no error')
                testCase.verifyError(@()multifile(1,2),'MATLAB:mex:ErrInvalidMEXFile','Known issue that engine not available in non-mex files')
            end
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