classdef cases < matlab.unittest.TestCase
   
    methods(TestClassSetup)
        
        function setPathUp(testCase)
            repoPath = fileparts(fileparts(fileparts(mfilename('fullpath'))));
            availablePaths = dir(fullfile(repoPath,'out','build','*','CMakeCache.txt'));
            if isscalar(availablePaths)
                bindir = fullfile(availablePaths.folder,'test');
            else
                dates = datetime(arrayfun(@(x)x.date,availablePaths,'UniformOutput',false));
                [~,i] = sort(dates,1,"descend");
                bindir = fullfile(availablePaths(i(1)).folder,'test');
            end
            
            % Set up shared state for all tests. 
            addpath(bindir);
            testCase.addTeardown(@()rmpath(bindir))
            
            % Tear down with testCase.addTeardown.
        end
        % Shared setup for the entire test class
        


    end
    
    methods(TestMethodSetup)
        % Setup for each test
    end
    
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

        % function structFields(testCase)
        %     tests = {struct('a',1),-1;...
        %              struct('toplevel',1),-1;...
        %              struct('toplevel',struct('a',1)),-1;...
        %              struct('toplevel',struct('nested','hello')),0};
        %     for i = 1:size(tests,1)
        %         if tests{i,2} == -1
        %             testCase.verifyError(...
        %                 structfields(tests{i,1}));
        %         else
        %             testCase.verifyReturnsTrue(...
        %                 structfields(tests{i,1}));
        %         end
        %     end
        % end
        
    end
    
end