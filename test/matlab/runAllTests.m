function runAllTests(outputdir,bindir)
arguments
    outputdir (1,1) string = pwd()
    bindir (1,1) string = fullfile(fileparts(fileparts(fileparts(mfilename('fullpath')))),'build','test')
end
import matlab.unittest.TestSuite
import matlab.unittest.plugins.XMLPlugin
import matlab.unittest.Verbosity

xmlFile = fullfile(outputdir,'testresult.xml');
p = XMLPlugin.producingJUnitFormat(xmlFile,'OutputDetail',Verbosity.Concise);

runner = testrunner();
addPlugin(runner,p);

c = cases;
c.bindir = bindir;
suite = TestSuite.fromClass(?cases);

results = run(runner,suite)


end

