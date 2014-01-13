#pragma once

#include <iostream>
#include <string>
#include <boost/tuple/tuple.hpp>

#include "TabularOutput.h"
#include "BucketCounter.h"
#include "../cepac/include.h"
#include "../entities/Person.h"
#include "../data/EventParams.h"

class Person;
class Population;

class ArtRolloutTracker : protected TabularOutput
{
public:
	ArtRolloutTracker();
	~ArtRolloutTracker();

	void SetAgeRanges(const std::vector<boost::tuple<long, int, int>> &ageGroupSizes);

	void recordTest(Person *person, bool accepted, bool returned, SimContext::TEST_RESULT result);

	void recordEligiblePerson(Person *person);

	void recordTreatment(Person *person);

	void printArtRolloutOutcomes(int time, std::ostream &_outStream, Population *_population);

private:
	static const std::string RISK_GROUP_NAMES[];

	int numTestsOffered;
	int numTestsAccepted;
	int numTestsReturnedFor;
	std::vector<int> numTestsByResult;
	BucketCounter testsByBucketCounter;
	BucketCounter eligibleByBucketCounter;
	BucketCounter treatedByBucketCounter;

	std::vector<std::pair<int, int>> ageRanges;

	void buildHeader();

	void buildRow(int time, Population *_population);

	void Reset();
};
