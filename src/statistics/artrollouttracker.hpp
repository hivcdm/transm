#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <include.h>

#include "bucketcounter.hpp"
#include "tabularoutput.hpp"
#include "parameters/agerangesizecontainer.hpp"
#include "parameters/eventparams.hpp"
#include "entities/entity.hpp"

namespace transm {

//class Entity;
class Population;

class ArtRolloutTracker : protected TabularOutput
{
public:
    /** Default constructor */
	ArtRolloutTracker();

	/** Deconstructor */
	~ArtRolloutTracker();

	void SetAgeRanges(const std::vector<AgeRange> &ageRanges);

	/** Recorders */
    /*@{*/
    void recordTest(Entity *person, bool accepted, bool returned, SimContext::TEST_RESULT result);
	void recordTreatmentAccessEligibility(Entity *person);
	void recordTreatmentAccess(Entity *person);
	void recordTreatmentEligibility(Entity *person);
	void recordTreatment(Entity *person);
	void recordTreatmentDeath(Entity *person);
	void recordTreatmentSlots(int numSlots);

	/* Stuff for Miami analysis for HIC care continuum */
	void recordSuppressedVL(Entity *person);            /****> Record the suppressed VL = lowest level VL */
	void recordEnrolledInThirtyDays(Entity *person);
	void recordInCare(Entity *person);
    void recordPLWH(Entity *person);
    void recordNewDiagnosis(Entity *person);
    void recordLTFU(Entity *person);
    void recordUnlinked(Entity *person);
    /*@}*/

	void printArtRolloutOutcomes(Time time, std::ostream &_outStream, Population *_population);

private:
	static const std::string RISK_GROUP_NAMES[];
	static const std::string TRACKED_OUTCOMES[];
    static const std::string BUCKETS[];

	int numTestsOffered;
	int numTestsAccepted;
	int numTestsReturnedFor;
	std::vector<int> numTestsByResult;
	BucketCounter counter;
	int numTreatmentSlots;

	std::vector<AgeRange> ageRanges;

	void buildHeader();

	void buildRow(Time time, Population *_population);

	void Reset();
};

} // namespace transm
