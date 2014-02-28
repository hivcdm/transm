/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#define _CXXTEST_HAVE_STD
#define _CXXTEST_HAVE_EH
#include <cxxtest/TestListener.h>
#include <cxxtest/TestTracker.h>
#include <cxxtest/TestRunner.h>
#include <cxxtest/RealDescriptions.h>
#include <cxxtest/TestMain.h>
#include <cxxtest/ErrorPrinter.h>

int main( int argc, char *argv[] ) {
 int status;
    CxxTest::ErrorPrinter tmp;
    CxxTest::RealWorldDescription::_worldName = "cxxtest";
    status = CxxTest::Main< CxxTest::ErrorPrinter >( tmp, argc, argv );
    return status;
}
bool suite_SimulationTestSuite_init = false;
#include "C:\Users\taf656\Development\cdm\source\tests\SimulationTestSuite.h"

static SimulationTestSuite suite_SimulationTestSuite;

static CxxTest::List Tests_SimulationTestSuite = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_SimulationTestSuite( "SimulationTestSuite.h", 5, "SimulationTestSuite", suite_SimulationTestSuite, Tests_SimulationTestSuite );

static class TestDescription_suite_SimulationTestSuite_testNonExistentInputFile : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_SimulationTestSuite_testNonExistentInputFile() : CxxTest::RealTestDescription( Tests_SimulationTestSuite, suiteDescription_SimulationTestSuite, 16, "testNonExistentInputFile" ) {}
 void runTest() { suite_SimulationTestSuite.testNonExistentInputFile(); }
} testDescription_suite_SimulationTestSuite_testNonExistentInputFile;

static class TestDescription_suite_SimulationTestSuite_testBadInputFile : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_SimulationTestSuite_testBadInputFile() : CxxTest::RealTestDescription( Tests_SimulationTestSuite, suiteDescription_SimulationTestSuite, 25, "testBadInputFile" ) {}
 void runTest() { suite_SimulationTestSuite.testBadInputFile(); }
} testDescription_suite_SimulationTestSuite_testBadInputFile;

static class TestDescription_suite_SimulationTestSuite_testGoodInputFile : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_SimulationTestSuite_testGoodInputFile() : CxxTest::RealTestDescription( Tests_SimulationTestSuite, suiteDescription_SimulationTestSuite, 35, "testGoodInputFile" ) {}
 void runTest() { suite_SimulationTestSuite.testGoodInputFile(); }
} testDescription_suite_SimulationTestSuite_testGoodInputFile;

#include <cxxtest/Root.cpp>
const char* CxxTest::RealWorldDescription::_worldName = "cxxtest";
