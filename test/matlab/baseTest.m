classdef baseTest < matlab.unittest.TestCase
   
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

end