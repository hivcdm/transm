/* Generated file, do not edit */

#ifndef CXXTEST_RUNNING
#define CXXTEST_RUNNING
#endif

#define _CXXTEST_HAVE_STD
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
bool suite_PersonTestSuite_init = false;
#include "C:\Users\taf656\Desktop\Development\cdm\tests\PersonTestSuite.h"

static PersonTestSuite suite_PersonTestSuite;

static CxxTest::List Tests_PersonTestSuite = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_PersonTestSuite( "C:/Users/taf656/Desktop/Development/cdm/workspaces/VS2010/../../tests/PersonTestSuite.h", 6, "PersonTestSuite", suite_PersonTestSuite, Tests_PersonTestSuite );

static class TestDescription_suite_PersonTestSuite_testConstructorsDestructors : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PersonTestSuite_testConstructorsDestructors() : CxxTest::RealTestDescription( Tests_PersonTestSuite, suiteDescription_PersonTestSuite, 19, "testConstructorsDestructors" ) {}
 void runTest() { suite_PersonTestSuite.testConstructorsDestructors(); }
} testDescription_suite_PersonTestSuite_testConstructorsDestructors;

static class TestDescription_suite_PersonTestSuite_testReseters : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PersonTestSuite_testReseters() : CxxTest::RealTestDescription( Tests_PersonTestSuite, suiteDescription_PersonTestSuite, 30, "testReseters" ) {}
 void runTest() { suite_PersonTestSuite.testReseters(); }
} testDescription_suite_PersonTestSuite_testReseters;

static class TestDescription_suite_PersonTestSuite_testRolls : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PersonTestSuite_testRolls() : CxxTest::RealTestDescription( Tests_PersonTestSuite, suiteDescription_PersonTestSuite, 38, "testRolls" ) {}
 void runTest() { suite_PersonTestSuite.testRolls(); }
} testDescription_suite_PersonTestSuite_testRolls;

static class TestDescription_suite_PersonTestSuite_testFullVector : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PersonTestSuite_testFullVector() : CxxTest::RealTestDescription( Tests_PersonTestSuite, suiteDescription_PersonTestSuite, 67, "testFullVector" ) {}
 void runTest() { suite_PersonTestSuite.testFullVector(); }
} testDescription_suite_PersonTestSuite_testFullVector;

static class TestDescription_suite_PersonTestSuite_testMutators : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PersonTestSuite_testMutators() : CxxTest::RealTestDescription( Tests_PersonTestSuite, suiteDescription_PersonTestSuite, 79, "testMutators" ) {}
 void runTest() { suite_PersonTestSuite.testMutators(); }
} testDescription_suite_PersonTestSuite_testMutators;

static class TestDescription_suite_PersonTestSuite_testBasicGettersAndSetters : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PersonTestSuite_testBasicGettersAndSetters() : CxxTest::RealTestDescription( Tests_PersonTestSuite, suiteDescription_PersonTestSuite, 95, "testBasicGettersAndSetters" ) {}
 void runTest() { suite_PersonTestSuite.testBasicGettersAndSetters(); }
} testDescription_suite_PersonTestSuite_testBasicGettersAndSetters;

static class TestDescription_suite_PersonTestSuite_testComplexGettersAndSetters : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PersonTestSuite_testComplexGettersAndSetters() : CxxTest::RealTestDescription( Tests_PersonTestSuite, suiteDescription_PersonTestSuite, 138, "testComplexGettersAndSetters" ) {}
 void runTest() { suite_PersonTestSuite.testComplexGettersAndSetters(); }
} testDescription_suite_PersonTestSuite_testComplexGettersAndSetters;

#include "C:\Users\taf656\Desktop\Development\cdm\tests\PopulationTestSuite.h"

static PopulationTestSuite suite_PopulationTestSuite;

static CxxTest::List Tests_PopulationTestSuite = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_PopulationTestSuite( "C:/Users/taf656/Desktop/Development/cdm/workspaces/VS2010/../../tests/PopulationTestSuite.h", 5, "PopulationTestSuite", suite_PopulationTestSuite, Tests_PopulationTestSuite );

static class TestDescription_suite_PopulationTestSuite_testConstructorsDestructors : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_PopulationTestSuite_testConstructorsDestructors() : CxxTest::RealTestDescription( Tests_PopulationTestSuite, suiteDescription_PopulationTestSuite, 18, "testConstructorsDestructors" ) {}
 void runTest() { suite_PopulationTestSuite.testConstructorsDestructors(); }
} testDescription_suite_PopulationTestSuite_testConstructorsDestructors;

#include "C:\Users\taf656\Desktop\Development\cdm\tests\SimTestSuite.h"

static SimTestSuite suite_SimTestSuite;

static CxxTest::List Tests_SimTestSuite = { 0, 0 };
CxxTest::StaticSuiteDescription suiteDescription_SimTestSuite( "C:/Users/taf656/Desktop/Development/cdm/workspaces/VS2010/../../tests/SimTestSuite.h", 5, "SimTestSuite", suite_SimTestSuite, Tests_SimTestSuite );

static class TestDescription_suite_SimTestSuite_testConstructorsDestructors : public CxxTest::RealTestDescription {
public:
 TestDescription_suite_SimTestSuite_testConstructorsDestructors() : CxxTest::RealTestDescription( Tests_SimTestSuite, suiteDescription_SimTestSuite, 18, "testConstructorsDestructors" ) {}
 void runTest() { suite_SimTestSuite.testConstructorsDestructors(); }
} testDescription_suite_SimTestSuite_testConstructorsDestructors;

#include <cxxtest/Root.cpp>
const char* CxxTest::RealWorldDescription::_worldName = "cxxtest";
