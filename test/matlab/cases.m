classdef cases < matlab.unittest.TestCase
    
    properties 
        bindir (1,1) string = fullfile(fileparts(fileparts(fileparts(mfilename('fullpath')))),'build','test')
    end

    methods(TestClassSetup)
        
        function classSetup1(testCase)
            % Set up shared state for all tests. 
            addpath(testCase.bindir);
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