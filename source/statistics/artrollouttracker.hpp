#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <include.h>

#include "bucketcounter.hpp"
#include "tabularoutput.hpp"
#include "data/agerangesizecontainer.hpp"
#include "data/eventparams.hpp"
#include "entities/person.hpp"

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
