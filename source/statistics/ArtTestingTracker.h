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

class ArtTestingTracker : protected TabularOutput
{
public:
	ArtTestingTracker();
	~ArtTestingTracker();

	void SetAgeRanges(const std::vector<boost::tuple<long, int, int> > &ageGroupSizes);

	void recordTest(Person *person, bool accepted, bool returned, SimContext::TEST_RESULT result);

	void recordEligiblePerson(Person *person);

	void recordTreatedPerson(Person *person);

	void printArtRolloutOutcomes(int time, std::ostream &_outStream, Population *_population);

private:
	static const std::string RISK_GROUP_NAMES[];

	int numTestsOffered;
	int numTestsAccepted;
	int numTestsReturnedFor;
	int numTestsByResult[SimContext::TEST_RESULT_NUM];
	BucketCounter testsByBucketCounter;
	BucketCounter eligibleByBucketCounter;
	BucketCounter enrolledByBucketCounter;

	std::vector<std::pair<int, int> > ageRanges;

	void buildHeader();

	void buildRow(int time, Population *_population);

	void Reset();
};
