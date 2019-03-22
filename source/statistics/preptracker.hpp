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
#include "entities/prep.hpp"

namespace transm {

class Population;

class PrepTracker : protected TabularOutput
{
public:
    PrepTracker();
    ~PrepTracker();

    void SetAgeRanges(const std::vector<AgeRange> &ageRanges);

    void recordTreatmentSlots(int numSlots);
    void recordEligiblity(Entity *person);
    void recordAccess(Entity *person);
    void recordAdherence(Entity *person);
    void recordLossToCare(Entity *person);
    void recordReturnToCare(Entity *person);

    void printPrepOutcomes(Time time, std::ostream &_outStream, Population *_population);

private:
    static const std::string RISK_GROUP_NAMES[];
    static const std::string RACE_ETHNICITY_GROUP_NAMES[];
    static const std::string TRACKED_OUTCOMES[];

    BucketCounter counter;

    int numTreatmentSlots;
    int numOnPrEP;

    std::vector<AgeRange> ageRanges;

    void buildHeader();

    void buildRow(Time time, Population *_population);

    void Reset();
};

} // namespace transm
