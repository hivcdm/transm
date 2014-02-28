#include <cxxtest/TestSuite.h>

#include "../Simulation.h"

class SimulationTestSuite : public CxxTest::TestSuite
{
public:
	SimulationTestSuite()
	{
	}

	~SimulationTestSuite()
	{
	}

	void testNonExistentInputFile()
	{
		Simulation s("../../../source/tests/non-existent.xml");
		TS_ASSERT_EQUALS(s.GetXmlFilename(), "../../../source/tests/non-existent.xml");
		TS_ASSERT_EQUALS(s.IsInitialized(), false);
		TS_ASSERT_THROWS_ANYTHING(s.Initialize());
		TS_ASSERT_EQUALS(s.IsInitialized(), false);
	}

	void testBadInputFile()
	{
		Simulation s("../../../source/tests/simulation-bad.xml");

		TS_ASSERT_EQUALS(s.GetXmlFilename(), "../../../source/tests/simulation-bad.xml");
		TS_ASSERT_EQUALS(s.IsInitialized(), false);
		TS_ASSERT_THROWS_ANYTHING(s.Initialize());
		TS_ASSERT_EQUALS(s.IsInitialized(), false);
	}

	void testGoodInputFile()
	{
		Simulation s("../../../source/tests/simulation-good.xml");

		TS_ASSERT_EQUALS(s.GetXmlFilename(), "../../../source/tests/simulation-good.xml");
		TS_ASSERT_EQUALS(s.IsInitialized(), false);
		s.SetMessageCallback([](const std::string &s) { std::cout << s; });
		TS_ASSERT_THROWS_NOTHING(s.Initialize());
		TS_ASSERT_EQUALS(s.IsInitialized(), true);

		auto params = s.GetEventParams();

		TS_ASSERT_DIFFERS(params, nullptr);
		TS_ASSERT_EQUALS(params->simName, "simulation-good");
	}
};
