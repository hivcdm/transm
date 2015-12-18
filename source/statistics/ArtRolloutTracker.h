#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <include.h>

#include "BucketCounter.h"
#include "TabularOutput.h"
#include "../data/AgeRangeSizeContainer.h"
#include "../data/EventParams.h"
#include "../entities/Person.h"

class Person;
class Population;

class ArtRolloutTracker : protected TabularOutput
{
public:
	ArtRolloutTracker();
	~ArtRolloutTracker();

	void SetAgeRanges(const std::vector<AgeRange> &ageRanges);

	void recordTest(Person *person, bool accepted, bool returned, SimContext::TEST_RESULT result);

	void recordTreatmentAccessEligiblity(Person *person);
	void recordTreatmentAccess(Person *person);
	void recordTreatmentEligiblity(Person *person);
	void recordTreatment(Person *person);
    void recordPrEP(Person *person);

	void printArtRolloutOutcomes(int time, std::ostream &_outStream, Population *_population);

private:
	static const std::string RISK_GROUP_NAMES[];
	static const std::string TRACKED_OUTCOMES[];

	int numTestsOffered;
	int numTestsAccepted;
	int numTestsReturnedFor;
	std::vector<int> numTestsByResult;
	BucketCounter counter;

	std::vector<AgeRange> ageRanges;

	void buildHeader();

	void buildRow(int time, Population *_population);

	void Reset();
};
