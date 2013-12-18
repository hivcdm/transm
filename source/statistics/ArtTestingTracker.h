#pragma once

#include <iostream>
#include <string>

#include "TabularOutput.h"
#include "../cepac/include.h"
#include "../entities/Person.h"
#include "../data/EventParams.h"

class Person;
class Population;

class ArtTestingTracker : protected TabularOutput
{
public:
	ArtTestingTracker();
	~ArtTestingTracker();

	void recordTest(Person *person, bool accepted, bool returned, SimContext::TEST_RESULT result);

	void printArtRolloutOutcomes(int time, std::ostream &_outStream, Population *_population);

private:
	//static const char *RISK_GROUP_NAMES[];

	int numTestsOffered;
	int numTestsAccepted;
	int numTestsByBucket[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];
	int numTestsByResult[SimContext::TEST_RESULT_NUM];

	void buildHeader(const std::vector<std::pair<int, int> > &ageRanges);

	void buildNumTestsHeader();

	void buildNumEligibleHeader();

	void buildNumEnrolledHeader();

	void Reset();
};
