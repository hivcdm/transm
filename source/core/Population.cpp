#include <algorithm>
#include <cstddef>
#include <iostream>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

#include "Population.h"
#include "Constants.h"
#include "Simulation.h"
#include "../entities/Female.h"
#include "../entities/Male.h"
#include "../entities/behaviors/SexualBehavior.h"
#include "../statistics/InfectionsTracker.h"
#include "../statistics/CostsTracker.h"
#include "../util/Utility.h"
#include "../util/rand/RandomNumberGenerator.h"

/***
Data needed :
events stratified by age and CD4
***/
unsigned int Population::idCounter = 0;

/**
Creates an initial population of folks
**/
Population::Population(EventParams &parameters) 
    : populationID(Population::idCounter++),
      parameters_(parameters)
{
}

void Population::Circumcise(Person *p)
{
    if(!p->IsCircumcised())
    {
        p->Circumcise();
        populationStatistics.costsTracker.RecordCircumcision(popWideParams.circumcisionCost, popWideParams.circumcisionCost * p->getCepacDiscountFactor());
    }
}

/**
This is a bit hackish and hardcoded
The method determines who are the partnership initiators and who are available to them
//first we determine who can initiate
//second we determine who can be accosted - this differs by partnershipType
**/
void Population::InitPartnershipBuckets()
{
	//this is used to select ProfileID's of eligible initiators
	DemographicProfile selector;
    selector.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::Active);
    selector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
    selector.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
	//all SA, non-CSW males can form partnerships of any type
	std::vector<DemographicProfile::ProfileID> eligibleInitiators;
	selector.selectProfileIDs(eligibleInitiators, nullptr);
	//men can form all types of partnerships
	std::vector<SexualPartnership::Type> availPartnershipTypes;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		availPartnershipTypes.push_back(SexualPartnership::Type(type));
	}

	//For each initiator demographic profile, store the fact that they can have any partner type
	for(size_t i = 0; i < eligibleInitiators.size(); i++)
	{
		BucketSexualMixing *bucket = (BucketSexualMixing *)entities->getBucket(eligibleInitiators.at(i));

		//if this Bucket is nullptr, the skip
		if(bucket != nullptr)
		{
			partneringInitiators[bucket] = availPartnershipTypes;
			profilesToPartnershipTypes[bucket->getProfileID()] = availPartnershipTypes;
		}
	}

	DemographicProfile currProfileSelector;
	vector<DemographicProfile::ProfileID> selectedIDs;

	//iterate through all partnership types. the available Buckets are different by partnership
	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
		//get the parameters for current relationship type
		const SexualBehavior &partneringParams = popWideParams.defaultMaleParams.getSexualBehavior(partnership_type);

		//get available demographicProfiles that are available for this partnership
		for(unsigned int j = 0; j < partneringParams.getNumAvailableBuckets(); ++j)
		{
			selectedIDs.clear();
			//contains profile ID's that were selected from
			currProfileSelector.set(partneringParams.getAvailableBucket(j).dmgProfileSelector);
			currProfileSelector.selectProfileIDs(selectedIDs, nullptr);

            //TODO:eventually, we should change this.
			assert(selectedIDs.size() == 1);	//we don't want any wild cards in the DemographicProfile string.

			//check to see whether we have a repeat Bucket.
			for(size_t i = 0; i < potentialPartnerBuckets[partnership_type].size(); ++i)
			{
                if(potentialPartnerBuckets[partnership_type].at(i)->getProfileID() == selectedIDs.at(0))
				{
                    throw std::runtime_error("For available buckets for partnership type '"
                        + SexualPartnership::TypeStrings.at(SexualPartnership::Type(partnership_type))
                        + "', " + *DemographicProfile::toString(selectedIDs.at(0))
                        + " is listed multiple times either via repeat or wildcard overlaps");
				}
			}

			BucketSexualMixing *bucket = (BucketSexualMixing *)entities->getBucket(selectedIDs.at(0));

			if(bucket == nullptr)
			{
                throw std::runtime_error("This Demographic Profile " 
                    + *DemographicProfile::toString(selectedIDs.at(0))
                    + " has not been instantiated and so cannot be used");
			}
			else
			{
                potentialPartnerBuckets[partnership_type].push_back(bucket);
                eligibleBucketWeights[partnership_type].push_back(partneringParams.getAvailableBucket(j).weight);
			}
		}

		//make sure that the weights sum to 1
        Utility::normalize(eligibleBucketWeights[partnership_type]);
	}
}

Population::~Population()
{
}

//-----------------< Event-related methods -----------------------------//


void Population::Births(EventParams &parameters_)
{
	//number of people to be born this month
	unsigned long numBorn = Utility::round<unsigned long>(currSize * popWideParams.birthRate);
	unsigned long numMales = static_cast<unsigned long>(popWideParams.proportionMale * numBorn);
	DemographicProfile::Gender gender;
	Person *p = nullptr;

	//create currSize * birthRate New people
	for(unsigned long i = 0; i < numBorn; ++i)
	{
		//determine gender
		gender = (i < numMales) ? DemographicProfile::Gender::Male : DemographicProfile::Gender::Female;
		bool toTrace = parameters_.currTime >= parameters_.monthTraceNewborns ? parameters_.numNewbornsTraced <
		               parameters_.numNewbornsToTrace : false;

		if(toTrace)
		{
			parameters_.numNewbornsTraced++;
		}

		p = GeneratePerson(parameters_, gender, nullptr, toTrace);

        if(parameters_.debugLevel > DebugLevel::One && parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
		{
            p->print(parameters_.trace_files[EventParams::TraceFile::Type::Events].file, Constants::TABTAB);
		}

		//add the newborn to the EntityPool
		//Use addPersonToAll here (initial entrance into population)
		entities->addPersonToAll(p);
	}

    if(parameters_.debugLevel > DebugLevel::Zero)
	{
		PrintMethodResults(parameters_, "Births", "People Born", numBorn, "total born", true);
	}
}

//Updates the age buckets for use with life expectancy
void Population::UpdateAgeBucketsLE()
{
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Person *p = (*p_Iter);
			assert(p != nullptr);
			//calculate life expectancy
			assert((populationStatistics.selectedLEStats != nullptr));
            assert(p->getAge(TimeGranularity::Year) >= 0);
            assert(p->getAge(TimeGranularity::Year) < Person::maxYrForDeathStats);
            populationStatistics.selectedLEStats->popByAge[p->getAge(TimeGranularity::Year)]++;
			p_Iter++;
		}
	}
}

/**
update age (and SAStatus b/c SAStatus depends on age), health,
**/
void Population::UpdatePhysicalState(EventParams &parameters_, bool calculateLE, bool newLEPeriod)
{
    dead_people_this_month_.clear();

	int totalDied = 0;		//keeps track of deaths this timestep
	//holds a pointer to the current bucket we are looking at
	BucketDemographicProfile *currBucket = nullptr;
	//helps us iterate through all BucketDemographicProfiles
	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;

	//Reset the class to calculate LE
	if(newLEPeriod)
	{
		populationStatistics.selectedLEStats = new PopulationStatistics::SingleLEStats;
	}

	//check if there is another bucket of entities to check
	//if there is a bucket, then iterate through people in bucket
	while(currProfileID <= DemographicProfile::MAX)
	{
		currBucket = entities->getBucket(currProfileID);

		//if people of this particular profile don't exist in the population, move on.
		if((currBucket == nullptr) || (currBucket->size() == 0))
		{
			currProfileID++;
			continue;
		}

		std::list<Person *> peopleTooOld = currBucket->ageOneTimeStep();

		//Let all the people who have reached max age die gracefully
		for(p_Iter = peopleTooOld.begin(); p_Iter != peopleTooOld.end(); p_Iter++)
		{
			if((*p_Iter)->rollForDeath(parameters_.randomNums))
			{
				assert((*p_Iter) != nullptr);
				assert(!(*p_Iter)->isAlive());
				//Don't process death until main loop from list so person can be removed from iterator list
			}
			else
			{
				(*p_Iter)->print(cerr, "THIS PERSON WOULDN'T DIE!");
			}
		}

		currProfileID++;
	}

	//Double loop: first iterate through the men, then the women
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Person *p = (*p_Iter);
			assert(p != nullptr);

			if(!p->isAlive())
			{
				//Advances p_Iter one in the list, so no increment is necessary
				p_Iter = entities->removePersonFromAll(p_Iter);
				ProcessDeath(parameters_, p, calculateLE);

				if(parameters_.useRollout)
				{
					//Remove people from the treated/untreated pool if they die
					std::list<Person *>::iterator poolIterator;
					poolIterator = std::find(rolloutUntreatedPool.begin(), rolloutUntreatedPool.end(), p);

					if(poolIterator != rolloutUntreatedPool.end())
					{
						rolloutUntreatedPool.erase(poolIterator);
					}
					else
					{
						poolIterator = std::find(rolloutTreatedPool.begin(), rolloutTreatedPool.end(), p);

						if(poolIterator != rolloutTreatedPool.end())
						{
							rolloutTreatedPool.erase(poolIterator);
						}
					}
				}

				totalDied++;
				continue;
			}

			Person::HIVStatus oldStatus = p->hivStatus;
			//update their health status
			p->updateHealthStatus(parameters_, &populationStatistics.artTracker, &populationStatistics.costsTracker);

			if(oldStatus != p->hivStatus)
			{
                if(p->getDemographicProfile()->get(p->getDemographicProfile()->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
				{
					((BucketSexualMixing *) entities->getBucket(p->getDemographicProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
					        p->hivStatus);
				}
			}

			//see whether this person has died.
			//if this person was a couple, then will push living members to personsToAdd
			// to be reinserted into the EntityPool once we have iterated through all buckets
			if(p->rollForDeath(parameters_.randomNums))
			{
			  //				bool wasProcessed = false;
				p_Iter = entities->removePersonFromAll(p_Iter);
				//				wasProcessed = true;
				ProcessDeath(parameters_, p, calculateLE);
				totalDied++;

				if(parameters_.useRollout)
				{
					//Remove people from the treated/untreated pool if they die
					std::list<Person *>::iterator poolIterator;
					poolIterator = std::find(rolloutUntreatedPool.begin(), rolloutUntreatedPool.end(), p);

					if(poolIterator != rolloutUntreatedPool.end())
					{
						rolloutUntreatedPool.erase(poolIterator);
					}
					else
					{
						poolIterator = std::find(rolloutTreatedPool.begin(), rolloutTreatedPool.end(), p);

						if(poolIterator != rolloutTreatedPool.end())
						{
							rolloutTreatedPool.erase(poolIterator);
						}
					}
				}

				//removePersonFromAll returns iterator to next person in list...
				//no need to increment
				continue;
			}

			//if this person wasn't sexually active but is now old enough to
            if((p->getDemographicProfileVal(DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::Active)
                && (p->getAge(TimeGranularity::Month) >= popWideParams.ageOfMajority))
			{
				// set them as SA and potentially CSWs
				if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && p->trace())
				{
                    if(p->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
					{
						parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Male ";
					}
					else
					{
						parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Female ";
					}

					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << p->getID() << " becomes sexually active" << std::endl;
				}

				Person::HIVStatus oldStatus = p->hivStatus;
				p->becomeSexuallyActive(parameters_);

				if(oldStatus != p->hivStatus)
				{
                    if(p->getDemographicProfile()->get(p->getDemographicProfile()->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
					{
						((BucketSexualMixing *) entities->getBucket(p->getDemographicProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
						        p->hivStatus);
					}
				}

                if(p->getAge(TimeGranularity::Month) < popWideParams.CSWEndAgeMth[p->getDemographicProfileVal(DemographicProfile::Demographic::Gender)])
				{
					p->rollForBecomeSexWorker(parameters_, false);
				}

				Person::RiskLevel oldRisk = p->getRiskLevel();
				//reroll risk group
				p->rerollRiskGroup(parameters_);
				//refresh risk group in dmg bucket and refresh BucketDemographicProfile
				entities->refreshBucketDemographicProfile(p, &p_Iter, oldRisk != p->getRiskLevel());
			}

			//Check for age to stop becoming CSW
            if(p->getDemographicProfileVal(DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw
                && p->getAge(TimeGranularity::Month) >= popWideParams.CSWEndAgeMth[p->getDemographicProfileVal(DemographicProfile::Demographic::Gender)])
			{
				p->quitSexWork(parameters_);
				entities->refreshBucketDemographicProfile(p, &p_Iter);
			}
            
			if(parameters_.useRollout && parameters_.treatedContext && p->isInfected())
			{
				if(p->isOnArt())
				{
					populationStatistics.recordTreatmentEligiblity(p); // if they're on treatment, they should be counted as eligible even if the treatment has worked
					populationStatistics.recordTreatment(p);
				}
				else if(p->isEligibleForTreatment(parameters_.treatedContext->getTreatmentInputs()->startART[0]))
				{
					populationStatistics.recordTreatmentEligiblity(p);
				}
			}

			populationStatistics.costsTracker.RecordLifeMonth(p->getQualityOfLife(), p->getCepacDiscountFactor(), p->getHIVStatus());

			p_Iter++;
		}
	}

    if(parameters_.debugLevel > DebugLevel::Zero)
	{
		PrintMethodResults(parameters_, "UpdatePhysicalState", "People Died", totalDied, "total died", true);
	}
}

std::unordered_set<Person *> Population::Find(std::function<bool(Person *)> predicate)
{
    std::unordered_set<Person *> matches;
    entities->forEach([=, &matches](Person *p) { if(predicate(p)) matches.insert(p); });
    return matches;
}

void Population::RegisterIntervention(const Intervention &intervention)
{
    interventions_.push_back(intervention);
}

/*  who initiates flings? it seems that males do for now

This method is mostly designed for speed as this takes up the bulk of processing
We hopefully only iterate through each initiator once.
*/
void Population::UpdatePartnerships(EventParams &parameters_)
{
	//holds the tallies for any New partnerships that were made and ended this month
	int newPartnershipCount[(std::size_t)SexualPartnership::Type::ENDType];
	//Number of attemptedPartnerships may be higher than the actual partnerships formed if there weren't enough females/males tried to repartner with current partners
	int attemptedPartnershipCount[(std::size_t)SexualPartnership::Type::ENDType];
	int	endedPartnershipCount[(std::size_t)SexualPartnership::Type::ENDType];

	//initialize counters
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		newPartnershipCount[type] = 0;
		attemptedPartnershipCount[type] = 0;
		endedPartnershipCount[type] = 0;
	}

	//iterate through all males
	std::list<Person *>::iterator p_Iter;

	//Iterate twice...
	//First pass: Dissolve ended partnerships
	for(p_Iter = entities->begin(DemographicProfile::Gender::Male); p_Iter != entities->end(DemographicProfile::Gender::Male); p_Iter++)
	{
		Person *person = (*p_Iter);
		Male *initiator = (Male *)person; //We're dealing with this dude
		(*p_Iter)->resetNumActs();
		std::list<SexualPartnership *> partnershipsToEnd;	//list of all partnerships due to end

		//Decide who needs to split up
		for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
		{
			//get partnerships of 'type' whose durations have elapsed, i.e. time to split
			endedPartnershipCount[type] += initiator->getPartnershipsToEnd(parameters_.currTime, SexualPartnership::Type(type), partnershipsToEnd, false);
		}//foreach SexualPartnership::type

		//Now, split them up... man, it would suck for their kids (if they had any)
		DissolveSexualPartnerships(parameters_, initiator, partnershipsToEnd);

		//if this initiator is now single, then make sure they are in singles pool
		if(!initiator->inCorrectBucketDemographicProfile())
		{
			entities->refreshBucketDemographicProfile(initiator, &p_Iter);
		}
	} //for (p_Iter = entities->begin(DemographicProfile::Gender::Male); p_Iter != entities->end(DemographicProfile::Gender::Male); p_Iter++) {

	//Ending the first pass (dissolving partnerships)

	//reset num acts for females
	for(p_Iter = entities->begin(DemographicProfile::Gender::Female); p_Iter != entities->end(DemographicProfile::Gender::Female); p_Iter++)
	{
		(*p_Iter)->resetNumActs();
	}

	//Second pass: Form new partnerships and have sex
	for(p_Iter = entities->begin(DemographicProfile::Gender::Male); p_Iter != entities->end(DemographicProfile::Gender::Male); p_Iter++)
	{
		Person *person = (*p_Iter);
		Male *initiator = (Male *)person;

		if(!person->isAlive())
		{
			continue;
		}

		//Reset the initiator's condom count
		initiator->resetCondomUsage();
		//(The non-initiators (i.e. women) will never have their condom count reset... I don't think we care?)
		assert(initiator != nullptr);

        if(initiator->getAge(TimeGranularity::Month) < popWideParams.ageOfMajority + initiator->GetSexualActivityDelay())
        {
            continue;
        }

		//Get available partnership types
		vector<SexualPartnership::Type> partnershipTypes =
		    profilesToPartnershipTypes[initiator->getCurrBucketProfileID()];

		//iterate through the SexualPartnership::Type that people in the current bucket engage in
		// form new partnerships
		for(size_t i = 0; i < partnershipTypes.size(); i++)
		{
			SexualPartnership::Type type = partnershipTypes.at(i);
			//this method distinguishes between partnerships with and without duration and
			//  executes different code depending on which. If the partnership has no duration
			//  associated with it, then the sexual act is done during this method
			//First, reset the tally of latest unformed partnerships (unformed but intended to form)
			initiator->resetLatestUnformedPartnerships(type);
			//TODO: Get the ratio of numFormed to numIntendedToForm
			int numFormed = CreatePartnerships(parameters_, initiator, &p_Iter, type);
			//if(numFormed > 0) {
			newPartnershipCount[(std::size_t)type] += numFormed;
			//} //if(numFormed > 0) {
			attemptedPartnershipCount[(std::size_t)type] += numFormed + initiator->getLatestUnformedPartnerships(type);
		} //for(int i =0; i < bucketIter->second.size(); i ++) {

		//for existing partnerships, have sexual activity
		//Have all the sexual activity with current partners (includes new partners)
		for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
		{
			std::list<Person *> newlyInfected;
			//sexual activity among any existing partnerships that have a duration associated with them
			Person *infectedMe = initiator->allPartnerSexualActivity(parameters_, SexualPartnership::Type(type), newlyInfected,
			                     &(populationStatistics.infectionsTracker));
			//TODO: Get a condom use count here!
			//record all incident infections
			std::list<Person *>::iterator newlyInfectedIter = newlyInfected.begin();

			while(newlyInfectedIter != newlyInfected.end())
			{
				Person *wasUninfected = *newlyInfectedIter;

				if(wasUninfected->getDemographicProfile()->get(wasUninfected->getDemographicProfile()->getProfileID(),
                    DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
				{
					((BucketSexualMixing *) entities->getBucket(wasUninfected->getDemographicProfile()->getProfileID()))->changeHIVStatus(
					    wasUninfected, Person::NEGATIVE, wasUninfected->hivStatus);
				}

				((BucketSexualMixing *) entities->getBucket(wasUninfected->getDemographicProfile()->getProfileID()))->increaseInfected(
				    wasUninfected);
				//initiator only gets infected once...
				Person *wasInfected = (*newlyInfectedIter == initiator) ? infectedMe : initiator;

                RecordInfection(wasUninfected, wasInfected, parameters_.currTime);

				//Adds person to the untreated pool if using rollout
				if(parameters_.useRollout)
				{
					rolloutUntreatedPool.push_back(wasUninfected);
				}

				if(parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
				{
                    populationStatistics.recordIncidentInfection(parameters_, parameters_.currTime,
                        SexualPartnership::Type(type),
                        wasInfected,
                        wasUninfected,
                        (parameters_.debugLevel > DebugLevel::One),
                        parameters_.trace_files[EventParams::TraceFile::Type::Events].file);
				}

				newlyInfectedIter++;
			}//while(newlyInfectedIter != newlyInfected.end())
		} //for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type) {

		//Add the cost of condom usage
		auto totalCondomCostUndiscounted = initiator->getCondomsUsedThisMonth() * popWideParams.condomCost;
		populationStatistics.costsTracker.RecordCondomUse(totalCondomCostUndiscounted, totalCondomCostUndiscounted * initiator->getCepacDiscountFactor());
	} //for (p_Iter = entities->begin(DemographicProfile::Gender::Male); p_Iter != entities->end(DemographicProfile::Gender::Male); p_Iter++)

	//Ends the second pass through (i.e. the sex acts pass through)

	//print out results to traces
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
        if(parameters_.debugLevel > DebugLevel::Zero)
		{
			//print out how many partnerships were attempted
			std::ostringstream attemptedLabel;
			attemptedLabel << SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)) << " Attempted";
			PrintMethodResults(parameters_, "updatePartnerships(...)", attemptedLabel.str(), attemptedPartnershipCount[type],
				"Attempted" + SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)), true);
			//print out how many partnerships were formed
			std::ostringstream formedLabel;
			formedLabel << SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)) << " Formed";
			PrintMethodResults(parameters_, "updatePartnerships(...)", formedLabel.str(), newPartnershipCount[type],
				"New " + SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)), true);

			//if these partnerships have a duration beyond the month, print out how many were ended
            if(popWideParams.partnershipsHaveDuration[(std::size_t)DemographicProfile::Gender::Male][type])
			{
				std::ostringstream endedLabel;
				endedLabel << SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)) << " Ended";
				PrintMethodResults(parameters_, "updatePartnerships(...)", endedLabel.str(), endedPartnershipCount[type],
				                         "Number Ended", true);
			}
		}
	} //for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type) {

	if(parameters_.calibrationInputs.useCalibration
	        && parameters_.currTime > (parameters_.calibrationInputs.monthOfCalibration - 12)
	        && parameters_.currTime <= parameters_.calibrationInputs.monthOfCalibration)
	{
		//update concurrency status
        for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::list<Person *>::iterator p_Iter = entities->begin((DemographicProfile::Gender) gender);

			while(p_Iter != entities->end((DemographicProfile::Gender) gender))
			{
				//tally concurrent partners
				//create a number between 0 and 15 representing the combination of partnership types person has
				//e.g. if person has partnerships steady and casual concurrent will equal 8+2=10
				int concurrent = 0;
				int numPartners[(std::size_t)SexualPartnership::Type::ENDType];
				int totalNumPartners = 0;

				for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
				{
					numPartners[i] = (*p_Iter)->getNumPartners((SexualPartnership::Type) i);
					concurrent = (concurrent << 1) + (numPartners[i] != 0 ? 1 : 0);
					totalNumPartners += numPartners[i];
				}

				concurrent = 15 - concurrent;
				assert(concurrent <= Constants::NUMBER_CONCURRENCY_DEFS);

				if(parameters_.concurrencyDef[concurrent].useDefinition
				        && totalNumPartners >= parameters_.concurrencyDef[concurrent].minPartnershipsNeeded)
				{
					//count as concurrent partnership
					(*p_Iter)->setMonthOfLatestConcurrent(parameters_.currTime);
				}

				p_Iter++;
			}
		}
	}
}

/*
void Population::SaveIndividualSummaries(std::ostream &stream) const
{
    std::vector<PersonSummary> ordered_(individual_summaries_.size());

    std::map<int, int> infection_numbers;

    for(auto summary : individual_summaries_)
    {
        ordered_[summary.second.infection_number] = summary.second;
        infection_numbers[summary.second.person_id] = summary.second.infection_number;
    }

    stream << "{" << std::endl;
    stream << "\"nodes\" : [" << std::endl;

    for(auto summary : ordered_)
    {
        stream << "{\"id\"" << ":" << summary.person_id;
        stream << ",\"gender\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::Gender) == 0 ? "\"male\"" : "\"female\"");
        stream << ",\"employment\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::Employment) == 0 ? "\"non-csw\"" : "\"csw\"");
        stream << ",\"sexual_activity_status\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::SexualActivityStatus) == 0 ? "\"sexually active\"" : "\"not active\"");
        stream << ",\"sexual_orientation\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::SexualOrientation) == 0 ? "\"hetero\"" : "\"homo\"");
        stream << ",\"relationship_status\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::RelationshipStatus) == 0 ? "\"non-single\"" : "\"single\"");
        stream << ",\"risk_group\"" << ":" << (summary.risk_group == Person::HIGH ? "\"high\"" : "\"low\"");
        stream << ",\"age_at_infection\"" << ":" << summary.age_at_infection;
        stream << ",\"generation_number\"" << ":" << summary.generation_number;
        stream << ",\"infection_number\"" << ":" << summary.infection_number;
        stream << ",\"time_infected\"" << ":" << summary.time_infected;
        stream << ",\"infected_by\"" << ":" << summary.infected_by;
        stream << ",\"group\":1";
        stream << "}";
        if(summary.person_id != ordered_.back().person_id)
        {
            stream << ",";
        }
        stream << std::endl;
    }

    stream << "]," << std::endl;
    stream << "\"links\" : [" << std::endl;

    for(auto summary : ordered_)
    {
        if(summary.infected_by == -1)
        {
            continue;
        }

        stream << "{\"source\":" << infection_numbers[summary.infected_by] << ",\"target\":" << summary.infection_number << ",\"value\":1}";
        if(summary.person_id != ordered_.back().person_id)
        {
            stream << ",";
        }
        stream << std::endl;

    }

    stream << "]" << std::endl;
    stream << "}" << std::endl;
}
*/

void WritePerson(std::ostream &stream, const std::map<int, std::vector<int>> &infected, int person_id, const std::unordered_map<unsigned long, Population::PersonSummary> &summaries)
{
    if(person_id == -1)
    {
        stream << "{\"id\"" << ":" << "initial prevalent case";
    }
    else
    {
        auto &summary = summaries.at(person_id);

        stream << "{\"id\"" << ":" << summary.person_id;
        stream << ",\"gender\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::Gender) == 0 ? "\"male\"" : "\"female\"");
        stream << ",\"employment\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::Employment) == 0 ? "\"non-csw\"" : "\"csw\"");
        stream << ",\"sexual_activity_status\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::SexualActivityStatus) == 0 ? "\"sexually active\"" : "\"not active\"");
        stream << ",\"sexual_orientation\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::SexualOrientation) == 0 ? "\"hetero\"" : "\"homo\"");
        stream << ",\"relationship_status\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::RelationshipStatus) == 0 ? "\"non-single\"" : "\"single\"");
        stream << ",\"risk_group\"" << ":" << (summary.risk_group == Person::HIGH ? "\"high\"" : "\"low\"");
        stream << ",\"age_at_infection\"" << ":" << summary.age_at_infection;
        stream << ",\"generation_number\"" << ":" << summary.generation_number;
        stream << ",\"infection_number\"" << ":" << summary.infection_number;
        stream << ",\"time_infected\"" << ":" << summary.time_infected;
    }

    if(infected.find(person_id) != infected.end())
    {
        stream << ",\"infected\"" << ":[" << std::endl;
        for(auto id : infected.at(person_id))
        {
            WritePerson(stream, infected, id, summaries);
            if(id != infected.at(person_id).back())
            {
                stream << ",";
            }
            stream << std::endl;
        }
        stream << "]" << std::endl;
    }

    stream << "}";
}

void Population::SaveIndividualSummaries(std::ostream &stream) const
{
    std::map<int, std::vector<int>> infected;

    for(auto summary : individual_summaries_)
    {
        infected[summary.second.infected_by].push_back((int)summary.second.person_id);
    }

    WritePerson(stream, infected, -1, individual_summaries_);
}

/**
Iterates through current entities in the population and returns a total number of people
***/
std::size_t Population::UpdateSize()
{
	currSize = entities->size();
	//Also update size of non-sexually active
	currNASize = entities->sizeNotSexuallyActive();

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		currNASizeByGender[(std::size_t)gender] = entities->sizeNotSexuallyActive(gender);

		for(int j = 0; j < Person::ENDRiskLevel; j++)
		{
			currSASizeGenderRisk[(std::size_t)gender][j] = entities->sizeSexuallyActive(gender, (Person::RiskLevel) j);
		}
	}

	//Size by gender
	DemographicProfile GenderProfile;
	//First tally the men
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
	std::vector<DemographicProfile::ProfileID> GenderProfileIDs;
    currSizeGender[(std::size_t)DemographicProfile::Gender::Male] = 0;
	GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);

	for(size_t i = 0; i < GenderProfileIDs.size(); i++)
	{
        currSizeGender[(std::size_t)DemographicProfile::Gender::Male] += entities->size(GenderProfileIDs[i]);
	}

	//Next tally the women
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
	GenderProfileIDs.clear();
    currSizeGender[(std::size_t)DemographicProfile::Gender::Female] = 0;
	GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);

	for(size_t i = 0; i < GenderProfileIDs.size(); i++)
	{
        currSizeGender[(std::size_t)DemographicProfile::Gender::Female] += entities->size(GenderProfileIDs[i]);
	}

	//Count all the CSW's
	DemographicProfile CSWProfile;
    CSWProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::Csw);
	vector<DemographicProfile::ProfileID> CSWProfileIDs;
	currCSWSize = 0;
	CSWProfile.selectProfileIDs(CSWProfileIDs, nullptr);

	for(size_t i = 0; i < CSWProfileIDs.size(); i++)
	{
		currCSWSize += entities->size(CSWProfileIDs[i]);
	}

	//Update size by risk level
	for(int risk = Person::LOW; risk < Person::ENDRiskLevel; risk++)
	{
		currSizeRisk[risk] = 0;
		currSizeRiskCSW[risk] = 0;

        for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
		{
			currSizeGenderRiskCSW[j][risk] = 0;
		}

		//loop through all buckets
		BucketDemographicProfile *currBucket = nullptr;
		DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;

		//iterate through all buckets
		while(currProfileID <= DemographicProfile::MAX)
		{
			currBucket = entities->getBucket(currProfileID);

			//if people of this particular profile don't exist in the population, move on.
			if((currBucket == nullptr) || (currBucket->size() == 0))
			{
				currProfileID++;
				continue;
			}

            if(DemographicProfile::get(currBucket->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
			{
				//Add all people in bucket of particular risk group
				currSizeRisk[risk] += ((BucketSexualMixing *)(currBucket))->sizeRisk((Person::RiskLevel) risk);

                if(DemographicProfile::get(currBucket->getProfileID(), DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw)
				{
					currSizeRiskCSW[risk] += ((BucketSexualMixing *)(currBucket))->sizeRiskCSW((Person::RiskLevel)risk);
					currSizeGenderRiskCSW[DemographicProfile::get(currBucket->getProfileID(),
					                            DemographicProfile::Demographic::Gender)][risk] += ((BucketSexualMixing *)(currBucket))->sizeRiskCSW((Person::RiskLevel)risk);
				}
			}

			currProfileID++;
		}	//while(currBucketIndex < entityBuckets->size()) {
	}

	for(auto &ageRangeSizePair : currSizeByAgeRange)
	{
		ageRangeSizePair.second = entities->sizeSexuallyActiveByAge(ageRangeSizePair.first.lower, ageRangeSizePair.first.upper);
	}

	for(auto &ageRangeSizePair : currSizeByAgeRangeMale)
	{
		ageRangeSizePair.second = entities->sizeSexuallyActiveByAge(ageRangeSizePair.first.lower, ageRangeSizePair.first.upper, DemographicProfile::Gender::Male);
	}

	for(auto &ageRangeSizePair : currSizeByAgeRangeFemale)
	{
		ageRangeSizePair.second = entities->sizeSexuallyActiveByAge(ageRangeSizePair.first.lower, ageRangeSizePair.first.upper, DemographicProfile::Gender::Female);
	}

    num_circumcised_na = 0;
    num_circumcised_sa = 0;

    entities->forEach([this](Person *p)
    {
        if(p->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Male
            && ((Male *)p)->isCircumcised())
        {
            if(p->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() == DemographicProfile::SexualActivityStatus::Active)
            {
                num_circumcised_sa++;
            }
            else
            {
                num_circumcised_na++;
            }
        }
    });

	return currSize;
}

//Resets Monthly Population Statistics
void Population::ResetMonthlyStats()
{
	//reset curr month death stats
	for(int i = 0; i < Person::ENDDeathStatus; i++)
	{
		currDeathCauses[i] = 0;
	}
}

//After all of the population dynamics have run through, run all of the remaining infected persons through CEPAC until they die to get the life expectancy and such in the CEPAC output files.
void Population::UpdateFinalPhysicalState(EventParams &parameters_)
{
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Person *p = (*p_Iter);
			assert(p != nullptr);

			//Removing entity because the age is about to change and this will fuck up being able to find the person!
			entities->removeEntity(p);
			p->runCEPACtoDeath(parameters_.randomNums);
			populationStatistics.processPostMaxTimeDeath(p);
			//Putting them back after age is updated
			entities->addEntity(p);
			p_Iter++;
		}
	}
}


//--------------------------< BEGIN helper methods  >-------------------------------------//

void Population::DissolveSexualPartnerships(EventParams &parameters_, Person *_initiator,
        std::list<SexualPartnership *> &_partnershipsToEnd)
{
    bool initiatorMale = (_initiator->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male);
	//iterate through each partner list
	std::list<SexualPartnership *>::iterator partnerIter = _partnershipsToEnd.begin();

	while(partnerIter != _partnershipsToEnd.end())
	{
		Person *partner = (*partnerIter)->getOtherPartner(_initiator);

		//print the couple that is getting divorced
        if(parameters_.debugLevel > DebugLevel::One && parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
		{
            (*partnerIter)->printPartners(parameters_.trace_files[EventParams::TraceFile::Type::Events].file,
                "This couple is splitting up: " + Constants::TABTAB);
		}

		if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (_initiator->trace() || partner->trace()))
		{
			if(_initiator->trace())
			{
				if(initiatorMale)
				{
					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "- Male ";
				}
				else
				{
					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "- Female ";
				}

				parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << _initiator->getID() 
					<< " ends " << (SexualPartnership::TypeStrings.at((*partnerIter)->getType()));
				parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " partnership with " << partner->getID();
			}
			else
			{
				if(initiatorMale)
				{
					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "- Female ";
				}
				else
				{
					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "- Male ";
				}

				parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << partner->getID() << " ends " << (SexualPartnership::TypeStrings.at((*partnerIter)->getType()));
				parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " partnership with " << _initiator->getID();
			}

			if(!_initiator->isAlive())
			{
				parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " -> " << _initiator->getID() << " has died";
			}

			if(!partner->isAlive())
			{
				parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " -> " << partner->getID() << " has died";
			}

			parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << std::endl;
		}//if (parameters_.outputTrace[EventParams::TraceFile::Type::SinglePerson] && (_initiator->trace() || partner->trace()))

		//if the partnership has any duration, destructor removes the pointer from both members partner lists
		delete(*partnerIter);

		//refresh BucketDemographicProfile placement if necessary
		if(!partner->inCorrectBucketDemographicProfile())
		{
			entities->refreshBucketDemographicProfile(partner, nullptr);
		}

		partnerIter++;
	}
}

Person *Population::GeneratePerson(EventParams &parameters_, DemographicProfile::Gender _gender,
                                   PopulationParameters::AgeBucketPrevalenceInfo *_ageBucketParams, bool toTrace)
{
	Person *toReturn = nullptr;	//pointer to the person that was just generated
	//determine age of current person. If we have no age _ageBucketParams, then this is a newborn.
	//Otherwise, generate an age from a uniform distribution bounded by _ageBucketParams
	int ageMth = (_ageBucketParams == nullptr) ? 0 : parameters_.randomNums.randInt(_ageBucketParams->minAgeMth,
	             _ageBucketParams->maxAgeMth);

	//create the person
	if(_gender == DemographicProfile::Gender::Male)
	{
		toReturn = new Male(parameters_, ageMth, parameters_.randomNums.chance(popWideParams.proportionCircumcised),
		                    populationID, popWideParams.defaultMaleParams);
	}
	else
	{
		toReturn = new Female(parameters_, ageMth, populationID, popWideParams.defaultFemaleParams);
	}

    toReturn->SetSexualActivityDelay(popWideParams.sexualActivityDelay);

	//If it was a boy and he was circumcised, add the costs
    if(toReturn->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
	{
		Male *m = (Male *)toReturn;

		if(m->isCircumcised())
		{
			populationStatistics.costsTracker.RecordCircumcision(popWideParams.circumcisionCost, popWideParams.circumcisionCost * m->getCepacDiscountFactor());
		}
	}

	//Set this person to be trace if toTrace is true
	if(toTrace)
	{
		toReturn->setToBeTraced();
	}

	//if person is of sexually active age, roll and see if they are a CSW
	//DO NOT SET THEM AS SEXUALLY ACTIVE UNTIL AFTER DETERMINING IF THEY ARE A PREVALENT CASE BECAUSE THE CEPAC PERSON IS CREATED HERE!
	if(ageMth >= popWideParams.ageOfMajority)
	{
		//see if they will be a CSW
        if(ageMth < popWideParams.CSWEndAgeMth[(std::size_t)_gender])
		{
            toReturn->rollForBecomeSexWorker(parameters_, true, popWideParams.initProbCSW[(std::size_t)_gender]);
		}

		//reroll their risk group
		toReturn->rerollRiskGroup(parameters_);
		//If they are of age, set them to be sexually active here: this is where toReturn->cepacPerson is initialized for non-prevalent cases
		Person::HIVStatus oldStatus = toReturn->hivStatus;
		toReturn->becomeSexuallyActive(parameters_);

		if(oldStatus != toReturn->hivStatus)
		{
			if(toReturn->getDemographicProfile()->get(toReturn->getDemographicProfile()->getProfileID(),
                DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
			{
				((BucketSexualMixing *) entities->getBucket(toReturn->getDemographicProfile()->getProfileID()))->changeHIVStatus(toReturn,
				        oldStatus, toReturn->hivStatus);
			}
		}
	}

	if(toTrace)
	{
        if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
		{
			toReturn->print(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].file, "Tracing the following patient: ");
		}
	}

	return toReturn;
}
/*
* Resets the counter for incidient infections by age for infectionstracker
*/
void Population::InitIncidentInfectionsByAge()
{
	AgeRangeSizeContainer incidentInfsAgeMale, incidentInfsAgeFemale, totalIncidentInfsAge;

	for(auto ageBucketParams : popWideParams.initialAgeBuckets)
	{
		AgeRange ageRange = {ageBucketParams.minAgeMth, ageBucketParams.maxAgeMth};
		incidentInfsAgeMale.push_back({ageRange, 0});
		incidentInfsAgeFemale.push_back({ageRange, 0});
		totalIncidentInfsAge.push_back({ageRange, 0});
	}

	populationStatistics.infectionsTracker.initializeIncidentInfectionsByAge(incidentInfsAgeMale, incidentInfsAgeFemale, totalIncidentInfsAge);
}

void Population::ApplyIncidentPrevalence(EventParams &parameters_)
{
	parameters_.displayOut("Applying incident prevalence data\n");
	//counter for number of people in each age bucket who are infected (used to initialize prevalence) (CSW, High risk, Low risk)
	std::vector<std::array<int, 3>> numInfectedByAgeBucketMale(popWideParams.initialAgeBuckets.size());
	std::vector<std::array<int, 3>> numInfectedByAgeBucketFemale(popWideParams.initialAgeBuckets.size());

	//loop through all males and apply prevalence to population
	for(std::list<Person *>::iterator males_iter = entities->begin(DemographicProfile::Gender::Male);
	        males_iter != entities->end(DemographicProfile::Gender::Male); males_iter++)
	{
		Person *p = *(males_iter);
		int ageBucketIndex = GetAgeBucketIndex(p);
		auto _ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucketIndex);
		DemographicProfile::Gender _gender = DemographicProfile::Gender::Male;
		//if this is a prevalent person, see if they're infected. Right now, newborns cannot be infected
		//TODO: Have counter in ageBucketParams for persons infected

        bool isCSW = p->getDemographicProfileVal(DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw;
		Person::RiskLevel risk = p->getRiskLevel();
		//apply prevalence if we have not yet reached the quoto of infected people for that bucket
		bool isPrevalent = false;

		if(isCSW)
		{
            if(numInfectedByAgeBucketMale.at(ageBucketIndex)[0] < _ageBucketParams.numInfectedCSW[(std::size_t)_gender])
			{
				isPrevalent = true;
				numInfectedByAgeBucketMale.at(ageBucketIndex)[0]++;
			}
		}
		else
		{
			if(risk == Person::HIGH)
			{
                if(numInfectedByAgeBucketMale.at(ageBucketIndex)[1] < _ageBucketParams.numInfectedRisk[(std::size_t)_gender][risk])
				{
					isPrevalent = true;
					numInfectedByAgeBucketMale.at(ageBucketIndex)[1]++;
				}
			}
			else
			{
                if(numInfectedByAgeBucketMale.at(ageBucketIndex)[2] < _ageBucketParams.numInfectedRisk[(std::size_t)_gender][risk])
				{
					isPrevalent = true;
					numInfectedByAgeBucketMale.at(ageBucketIndex)[2]++;
				}
			}
		}

		if(isPrevalent)
		{
			//Generation of infection for all prevalent cases is 0
			//toReturn->cepacPatient is initialized HERE for prevalent cases
			Person::HIVStatus oldStatus = p->hivStatus;

			if(parameters_.tracePrevalentCases)
			{
				p->setToBeTraced();
			}

			p->becomeInfected(Constants::PREVALENT_INFECTION, parameters_);
            RecordInfection(p, nullptr, parameters_.currTime);

            if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
            {
                p->printCurrentPartners(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].file, "Current partners for newly infected prevalent case:");
            }

			if(oldStatus != p->hivStatus)
			{
				((BucketSexualMixing *) entities->getBucket(p->getDemographicProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
					    p->hivStatus);
			}

			//Adds person to the untreated pool if using rollout
			if(parameters_.useRollout)
			{
				rolloutUntreatedPool.push_back(p);
			}

            if(p->getDemographicProfile()->get(p->getDemographicProfile()->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
			{
				((BucketSexualMixing *)entities->getBucket(p->getDemographicProfile()->getProfileID()))->increaseInfected(p);
			}
		}
	}//for()

	//loop through all females and apply prevalence to population
	for(std::list<Person *>::iterator females_iter = entities->begin(DemographicProfile::Gender::Female);
	        females_iter != entities->end(DemographicProfile::Gender::Female); females_iter++)
	{
		Person *p = *(females_iter);
		int ageBucketIndex = GetAgeBucketIndex(p);
		PopulationParameters::AgeBucketPrevalenceInfo &_ageBucketParams = popWideParams.initialAgeBuckets.at(
		            ageBucketIndex);
		DemographicProfile::Gender _gender = DemographicProfile::Gender::Female;
		//if this is a prevalent person, see if they're infected. Right now, newborns cannot be infected
		//TODO: Have counter in ageBucketParams for persons infected

        bool isCSW = p->getDemographicProfileVal(DemographicProfile::Demographic::Employment) == (std::size_t)DemographicProfile::Employment::Csw;
		Person::RiskLevel risk = p->getRiskLevel();
		bool isPrevalent = false;

		if(isCSW)
		{
            if(numInfectedByAgeBucketFemale.at(ageBucketIndex)[0] < _ageBucketParams.numInfectedCSW[(std::size_t)_gender])
			{
				isPrevalent = true;
				numInfectedByAgeBucketFemale.at(ageBucketIndex)[0]++;
			}
		}
		else
		{
			if(risk == Person::HIGH)
			{
                if(numInfectedByAgeBucketFemale.at(ageBucketIndex)[1] < _ageBucketParams.numInfectedRisk[(std::size_t)_gender][risk])
				{
					isPrevalent = true;
					numInfectedByAgeBucketFemale.at(ageBucketIndex)[1]++;
				}
			}
			else
			{
                if(numInfectedByAgeBucketFemale.at(ageBucketIndex)[2] < _ageBucketParams.numInfectedRisk[(std::size_t)_gender][risk])
				{
					isPrevalent = true;
					numInfectedByAgeBucketFemale.at(ageBucketIndex)[2]++;
				}
			}
		}

		if(isPrevalent)
		{
			//Generation of infection for all prevalent cases is 0
			//toReturn->cepacPatient is initialized HERE for prevalent cases
			Person::HIVStatus oldStatus = p->hivStatus;

			if(parameters_.tracePrevalentCases)
			{
				p->setToBeTraced();
			}

			p->becomeInfected(Constants::PREVALENT_INFECTION, parameters_);
            RecordInfection(p, nullptr, parameters_.currTime);

			if(oldStatus != p->hivStatus)
			{
				((BucketSexualMixing *) entities->getBucket(p->getDemographicProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
					    p->hivStatus);
			}

			if(parameters_.useRollout)
			{
				rolloutUntreatedPool.push_back(p);
			}

            if(p->getDemographicProfile()->get(p->getDemographicProfile()->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
			{
				((BucketSexualMixing *)entities->getBucket(p->getDemographicProfile()->getProfileID()))->increaseInfected(p);
			}
		}
	}//for()
}

void Population::RecordInfection(const Person *infectee, const Person *infector, int time)
{
    if(individual_summaries_.size() >= 1000)
    {
        return;
    }

    PersonSummary summary;

    summary.person_id = (int)infectee->getID();
    summary.generation_number = infectee->getGenerationOfInfection(false);
    summary.infected_by = infector == nullptr ? -1 : infector->getID();
    summary.infection_number = (int)individual_summaries_.size();
    summary.profile = *infectee->getDemographicProfile();
    summary.time_infected = time;
    summary.time_of_death = -1;
    summary.age_at_infection = infectee->getAge(TimeGranularity::Month);
    summary.risk_group = infectee->getRiskLevel();

    if(individual_summaries_.find(infectee->getID()) != individual_summaries_.end())
    {
        throw std::runtime_error("already being recorded");
    }

    individual_summaries_[infectee->getID()] = summary;
}

/**
* Sets the untreated and treated cepac files if using ART Rollout
*/
void Population::ApplyRolloutContext(EventParams &parameters_, int time)
{
	for(vector <EventParams::RolloutContext *>::iterator rolloutContextIter = parameters_.rolloutSimContexts.begin();
	        rolloutContextIter != parameters_.rolloutSimContexts.end(); rolloutContextIter++)
	{
		EventParams::RolloutContext *rolloutContext = *rolloutContextIter;

		if(rolloutContext->timeToApply == time)
		{
			//Switch the cepac file depending on the population the new file is applied to
			switch(rolloutContext->popOfInterest)
			{
			case 0: //All Untreated
				parameters_.untreatedContext = rolloutContext->rolloutSimContext.get();
				break;

			case 1:  //All Treated
			{
				parameters_.treatedContext = rolloutContext->rolloutSimContext.get();
				//Apply to all current treated patients
				std::list<Person *>::iterator personIter;

				for(personIter = rolloutTreatedPool.begin(); personIter != rolloutTreatedPool.end(); personIter++)
				{
					(*personIter)->setSimContext(parameters_.treatedContext);
				}

				break;
			}

			case 2: //Untreated Getting new art
				parameters_.treatedContext = rolloutContext->rolloutSimContext.get();
				break;

			default:
				break;
			}
		}
	}
}

void Population::DetermineRankings(const EventParams::RolloutEligibility &criteria)
{
	std::unordered_set<Person *> rankedPeople;

	//loop through the eligibility rankings
	for(int currentRank = 1; currentRank <= 5; ++currentRank)
	{
		bool checkOiHist = currentRank == criteria.oiHistRank;
		bool checkCd4 = currentRank == criteria.cd4Rank;
		bool checkCd4OiHist = currentRank == criteria.cd4OiHistRank;
		bool checkHvl = currentRank == criteria.hvlRank;
		bool checkCd4Hvl = currentRank == criteria.cd4HvlRank;
		rankedForTreatment[currentRank - 1].clear();

		if(checkOiHist || checkCd4 || checkCd4OiHist || checkHvl || checkCd4Hvl)
		{
			list <Person *>::iterator untIter = rolloutUntreatedPool.begin();

			//loop through people in the untreated pool to check for their eligibility
			while(untIter != rolloutUntreatedPool.end())
			{
				bool isEligible = false;
				Person *untPerson = *untIter;

				if(rankedPeople.find(untPerson) != rankedPeople.end())
				{
					untIter++;
					continue;
				}

				if(checkOiHist)
				{
					int numMatchingOIs = 0;

					for(int oiNum = 0; oiNum < Constants::NUMBER_OF_OIS; oiNum++)
					{
						if(criteria.oiHistOIs[oiNum] && untPerson->oiHistory[oiNum])
						{
							numMatchingOIs++;
						}
					}

					if(numMatchingOIs >= criteria.oiHistNumToStart)
					{
						isEligible = true;
					}
				}

				if(checkCd4)
				{
					if(untPerson->cd4 >= criteria.cd4Bounds.lower && untPerson->cd4 <= criteria.cd4Bounds.upper)
					{
						isEligible = true;
					}
				}

				if(checkCd4OiHist)
				{
					int numMatchingOIs = 0;

					for(int oiNum = 0; oiNum < Constants::NUMBER_OF_OIS; oiNum++)
					{
						if(criteria.cd4OiHistOIs[oiNum] && untPerson->oiHistory[oiNum])
						{
							numMatchingOIs++;
						}
					}

					if(untPerson->cd4 >= criteria.cd4OiHistCd4Bounds.lower
					        && untPerson->cd4 <= criteria.cd4OiHistCd4Bounds.upper && numMatchingOIs >= 1)
					{
						isEligible = true;
					}
				}

				if(checkHvl)
				{
					Person::HVLStrata currHvl = untPerson->currentTrueHvl;

					if((int)currHvl >= criteria.hvlBounds.lower && (int)currHvl <= criteria.hvlBounds.upper)
					{
						isEligible = true;
					}
				}

				if(checkCd4Hvl)
				{
					Person::HVLStrata currHvl = untPerson->currentTrueHvl;
					double currCd4 = untPerson->cd4;

					if(currCd4 >= criteria.cd4HvlCd4Bounds.lower && currCd4 <= criteria.cd4HvlCd4Bounds.upper
					        && currHvl >= criteria.cd4HvlHvlBounds.lower && currHvl <= criteria.cd4HvlHvlBounds.upper)
					{
						isEligible = true;
					}
				}

				if(isEligible)
				{
					rankedForTreatment[currentRank - 1].push_back(untPerson);
					rankedPeople.insert(untPerson);
				}

				untIter++;
			}
		}
	}
}

void Population::StartTreatment(Person *person, SimContext *treatedContext)
{
	std::list<Person *>::iterator untreatedIterator;
	untreatedIterator = std::find(rolloutUntreatedPool.begin(), rolloutUntreatedPool.end(), person);
	assert(untreatedIterator != rolloutUntreatedPool.end());
	rolloutUntreatedPool.erase(untreatedIterator);
	rolloutTreatedPool.push_back(person);
	person->setSimContext(treatedContext);
}

double InterpolateProportion(const std::map<int, double> &yearly_proportions, int month, int monthOf1990)
{
	if(month >= monthOf1990)
	{
		int relative_year = 1990 + (month - monthOf1990) / 12;

		if(relative_year >= yearly_proportions.begin()->first)
		{
			auto last = *(--yearly_proportions.end());
			if(relative_year < last.first)
			{
				double currentYearTargetProportion = yearly_proportions.at(relative_year);
				double nextYearTargetProportion = yearly_proportions.at(relative_year + 1);
				double x = ((month - monthOf1990) % 12) / 12.0;
				return currentYearTargetProportion + (nextYearTargetProportion - currentYearTargetProportion) * x;
			}
			else
			{
				return last.second;
			}
		}
	}

	return 0;
}

int Population::UpdateTreatmentSlots(double rolloutProportion)
{
	int numAccessingTreatment = (int)rolloutTreatedPool.size();
	double targetTreatmentSlots = GetSize() * rolloutProportion;

	if(parameters_.enableDynamicTreatmentScaling)
	{
		int position = (parameters_.currTime - parameters_.monthOf1990) % parameters_.dynamicFeedbackPeriod;

		if(position == 0)
		{
			treatmentCorrectionFactor_ = 1;
			auto numTreated = std::count_if(rolloutTreatedPool.begin(), rolloutTreatedPool.end(), [](Person *p) { return p->isOnArt(); });

			if(numTreated > 0)
			{
				treatmentCorrectionFactor_ = numAccessingTreatment / static_cast<double>(numTreated);
			}
		}

		return static_cast<int>(targetTreatmentSlots * treatmentCorrectionFactor_) - numAccessingTreatment;
	}
	else
	{
		return static_cast<int>(GetSize() * rolloutProportion) - numAccessingTreatment;
	}
}

void Population::ApplyARTRollout(EventParams &parameters_)
{
	double rolloutProportion = InterpolateProportion(parameters_.targetYearlyRolloutProportions, 
		parameters_.currTime, parameters_.monthOf1990);
	int newSlots = UpdateTreatmentSlots(rolloutProportion);

	if(rolloutProportion > 0)
	{
		DetermineRankings(parameters_.rolloutEligibility);
	}

	if(newSlots > 0)
	{
		for(auto &current_ranking_bucket : rankedForTreatment)
		{
			while(newSlots > 0 && !current_ranking_bucket.empty())
			{
				int randomPersonIndex = (int)parameters_.randomNums.randInt(0, (uint32_t)current_ranking_bucket.size() - 1);
				StartTreatment(current_ranking_bucket[randomPersonIndex], parameters_.treatedContext);

				if(randomPersonIndex != static_cast<int>(current_ranking_bucket.size() - 1))
				{
					std::swap(current_ranking_bucket[randomPersonIndex], current_ranking_bucket.back());
				}

				current_ranking_bucket.pop_back();
				--newSlots;
			}
		}
	}

	for(auto &current_ranking_bucket : rankedForTreatment)
	{
		for(auto &person : current_ranking_bucket)
		{
			populationStatistics.recordTreatmentAccessEligiblity(person);
		}
	}

	for(auto &person : rolloutTreatedPool)
	{
		// double counting shouldn't be a problem, they're either in rolloutTreatedPool or rankedForTreatment but not both
		populationStatistics.recordTreatmentAccessEligiblity(person);
		populationStatistics.recordTreatmentAccess(person);
	}
}

void Population::RecordShiftedOutcomes(EventParams &parameters_, std::ostream &_outStream)
{
	if(parameters_.monthOf1990 <= parameters_.currTime)
	{
		int year = 1990 + (parameters_.currTime - parameters_.monthOf1990) / 12;
		int month = (parameters_.currTime - parameters_.monthOf1990) % 12;
		int numTests = parameters_.cepacRunStats->getHIVScreening()->numAcceptTest;
		std::vector<int> numTestsByResult(SimContext::TEST_RESULT_NUM, 0);

		for(int i = 0; i < SimContext::TEST_RESULT_NUM; i++)
		{
			numTestsByResult[i] += parameters_.cepacRunStats->getHIVScreening()->numTestResultsPrevalentType[i];
			numTestsByResult[i] += parameters_.cepacRunStats->getHIVScreening()->numTestResultsIncidentType[i];
			numTestsByResult[i] += parameters_.cepacRunStats->getHIVScreening()->numTestResultsHIVNegativeType[i];
		}

		if(numTests)
		{
			populationStatistics.recordTestStats(numTests, numTestsByResult);
		}

		if(month == 11)
		{
			populationStatistics.printShiftedOutcomes(_outStream, year);
			populationStatistics.resetYear(year + 1);
		}
	}
}

bool Population::PassesPartnershipCalibration(EventParams &parameters_)
{
	//calculate partnership prevalence values
    unsigned long numInPartnership[(std::size_t)SexualPartnership::Type::ENDType][(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numInConcurrent[(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numActsMonth[(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numActsMonthRisk[(std::size_t)DemographicProfile::Gender::Last][Person::ENDRiskLevel];
    unsigned long numSexuallyActive[(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numSexuallyActiveRisk[(std::size_t)DemographicProfile::Gender::Last][Person::ENDRiskLevel];
	bool passesCalib = true;
	bool passesSteadyPrev = true;
	bool passesCasualPrev = true;
	bool passesCSWPrev = true;
	bool passesPropInConcurrent = true;
	bool passesNumActs = true;
	bool passesCasualPartPrevRatio = true;
	bool passesPropConcRatio = true;
	bool passesNumActsLRtoHRRatio = true;
	std::ostringstream firstRow;
	std::ostringstream secondRow;
	std::ostringstream thirdRow;
	secondRow << "Steady Partnership Prevalence" << Constants::TAB << "Casual Partnership Prevalence" << Constants::TAB <<
	          "CSW Partnership Prevalence" << Constants::TAB << "Prop in Concurrent" << Constants::TAB <<
	          "Num Acts (per person per month)" << Constants::TAB << "Casual Partnership Prev Ratio (FtM)" << Constants::TAB <<
	          "Prop in Concurrent Ratio (FtM)" << Constants::TAB << "Avg Num Acts Ratio (LR to HR Females)" << Constants::TAB <<
	          "Steady Partnership Prev" << Constants::TAB << "Casual Partnership Prevalence" << Constants::TAB <<
	          "CSW Partnership Prevalence" << Constants::TAB << "Prop In Concurrent" << Constants::TAB << "Num Acts" << Constants::TAB
	          << "Casual Partnership Prev Ratio (FtM)" << Constants::TAB << "Prop in Concurrent Ratio (FtM)" << Constants::TAB <<
	          "Avg Num Acts Ratio (LR to HR Females)" << Constants::TAB;

    for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
	{
        for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
		{
			numInPartnership[i][j] = 0;
		}
	}

    for(int i = 0; i < (int)DemographicProfile::Gender::Last; i++)
	{
		numInConcurrent[i] = 0;
		numSexuallyActive[i] = 0;
		numActsMonth[i] = 0;

		for(int j = 0; j < Person::ENDRiskLevel; j++)
		{
			numActsMonthRisk[i][j] = 0;
			numSexuallyActiveRisk[i][j] = 0;
		}
	}

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		std::list<Person *>::iterator p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
            if((*p_Iter)->getDemographicProfileVal(DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::Active)
			{
				p_Iter++;
				continue;
			}

			Person::RiskLevel risk = (*p_Iter)->getRiskLevel();
            numSexuallyActive[(std::size_t)gender]++;
            numSexuallyActiveRisk[(std::size_t)gender][risk]++;
            numActsMonth[(std::size_t)gender] += (*p_Iter)->getNumActsThisMonth();
            numActsMonthRisk[(std::size_t)gender][risk] += (*p_Iter)->getNumActsThisMonth();

			for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
			{
				if((*p_Iter)->getMonthOfLatestPartnershipDissolution((SexualPartnership::Type) i) > max<int>((
				            parameters_.currTime - 12), 0))
				{
                    numInPartnership[i][(std::size_t)gender]++;
				}
			}

			if((*p_Iter)->getMonthOfLatestConcurrent() > max<int>((parameters_.currTime - 12), 0))
			{
                numInConcurrent[(std::size_t)gender]++;
			}

			p_Iter++;
		}
	}

	//check if population meets bounds for partnership prevalance
	int numInSteady, numInCasual, numInCSW, concurrentNum, numActs;
    int maleSA = numSexuallyActive[(std::size_t)DemographicProfile::Gender::Male];
    int femaleSA = numSexuallyActive[(std::size_t)DemographicProfile::Gender::Female];
	int totalSA = maleSA + femaleSA;
	double steadyPrev, casualPrev, CSWPrev, propInConcurrent, numActsAvg;

	if(parameters_.calibrationInputs.steadyPrevPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
        numInSteady = numInPartnership[(int)SexualPartnership::Type::Steady][(std::size_t)DemographicProfile::Gender::Male] +
            numInPartnership[(int)SexualPartnership::Type::Steady][(std::size_t)DemographicProfile::Gender::Female]; //entire SA pop

		if(totalSA != 0)
		{
			steadyPrev = numInSteady / (double)totalSA;

			if(steadyPrev < parameters_.calibrationInputs.steadyPrevBounds.lower
			        || steadyPrev > parameters_.calibrationInputs.steadyPrevBounds.upper)
			{
				passesCalib = false;
				passesSteadyPrev = false;
			}
		}
		else
		{
			steadyPrev = -1;
		}
	}
	else
	{
		firstRow << "Male SA Pop" << Constants::TAB;
        numInSteady = numInPartnership[(int)SexualPartnership::Type::Steady][(std::size_t)DemographicProfile::Gender::Male]; //only male SA

		if(maleSA != 0)
		{
			steadyPrev = numInSteady / (double)maleSA;

			if(steadyPrev < parameters_.calibrationInputs.steadyPrevBounds.lower
			        || steadyPrev > parameters_.calibrationInputs.steadyPrevBounds.upper)
			{
				passesCalib = false;
				passesSteadyPrev = false;
			}
		}
		else
		{
			steadyPrev = -1;
		}
	}

	if(parameters_.calibrationInputs.casualPrevPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
        numInCasual = numInPartnership[(int)SexualPartnership::Type::Casual][(std::size_t)DemographicProfile::Gender::Male] +
            numInPartnership[(int)SexualPartnership::Type::Casual][(std::size_t)DemographicProfile::Gender::Female]; //entire SA pop

		if(totalSA != 0)
		{
			casualPrev = numInCasual / (double)totalSA;

			if(casualPrev < parameters_.calibrationInputs.casualPrevBounds.lower
			        || casualPrev > parameters_.calibrationInputs.casualPrevBounds.upper)
			{
				passesCalib = false;
				passesCasualPrev = false;
			}
		}
		else
		{
			casualPrev = -1;
		}
	}
	else
	{
		firstRow << "Male SA Pop" << Constants::TAB;
        numInCasual = numInPartnership[(int)SexualPartnership::Type::Casual][(std::size_t)DemographicProfile::Gender::Male]; //only male SA

		if(maleSA != 0)
		{
			casualPrev = numInCasual / (double)maleSA;

			if(casualPrev < parameters_.calibrationInputs.casualPrevBounds.lower
			        || casualPrev > parameters_.calibrationInputs.casualPrevBounds.upper)
			{
				passesCalib = false;
				passesCasualPrev = false;
			}
		}
		else
		{
			casualPrev = -1;
		}
	}

	if(parameters_.calibrationInputs.CSWPrevPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
        numInCSW = numInPartnership[(int)SexualPartnership::Type::Csw][(std::size_t)DemographicProfile::Gender::Male] +
            numInPartnership[(int)SexualPartnership::Type::Csw][(std::size_t)DemographicProfile::Gender::Female]; //entire SA pop

		if(totalSA != 0)
		{
			CSWPrev = numInCSW / (double)totalSA;

			if(CSWPrev < parameters_.calibrationInputs.CSWPrevBounds.lower
			        || CSWPrev > parameters_.calibrationInputs.CSWPrevBounds.upper)
			{
				passesCalib = false;
				passesCSWPrev = false;
			}
		}
		else
		{
			CSWPrev = -1;
		}
	}
	else
	{
		firstRow << "Male SA Pop" << Constants::TAB;
        numInCSW = numInPartnership[(int)SexualPartnership::Type::Csw][(std::size_t)DemographicProfile::Gender::Male]; //only male SA

		if(maleSA != 0)
		{
			CSWPrev = numInCSW / (double)maleSA;

			if(CSWPrev < parameters_.calibrationInputs.CSWPrevBounds.lower
			        || CSWPrev > parameters_.calibrationInputs.CSWPrevBounds.upper)
			{
				passesCalib = false;
				passesCSWPrev = false;
			}
		}
		else
		{
			CSWPrev = -1;
		}
	}

	if(parameters_.calibrationInputs.propInConcurrentPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
        concurrentNum = numInConcurrent[(std::size_t)DemographicProfile::Gender::Male] + numInConcurrent[(std::size_t)DemographicProfile::Gender::Female]; //entire SA pop

		if(totalSA != 0)
		{
			propInConcurrent = concurrentNum / (double)totalSA;

			if(propInConcurrent < parameters_.calibrationInputs.propInConcurrentBounds.lower
			        || propInConcurrent > parameters_.calibrationInputs.propInConcurrentBounds.upper)
			{
				passesCalib = false;
				passesPropInConcurrent = false;
			}
		}
		else
		{
			propInConcurrent = -1;
		}
	}
	else
	{
		firstRow << "Male SA Pop" << Constants::TAB;
        concurrentNum = numInConcurrent[(std::size_t)DemographicProfile::Gender::Male]; //only male SA

		if(maleSA != 0)
		{
			propInConcurrent = concurrentNum / (double)maleSA;

			if(propInConcurrent < parameters_.calibrationInputs.propInConcurrentBounds.lower
			        || propInConcurrent > parameters_.calibrationInputs.propInConcurrentBounds.upper)
			{
				passesCalib = false;
				passesPropInConcurrent = false;
			}
		}
		else
		{
			propInConcurrent = -1;
		}
	}

	if(parameters_.calibrationInputs.numActsPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
        numActs = numActsMonth[(std::size_t)DemographicProfile::Gender::Male] + numActsMonth[(std::size_t)DemographicProfile::Gender::Female]; //entire SA pop

		if(totalSA != 0)
		{
			numActsAvg = numActs / (double)totalSA;

			if(numActsAvg < parameters_.calibrationInputs.numActsBounds.lower
			        || numActsAvg > parameters_.calibrationInputs.numActsBounds.upper)
			{
				passesCalib = false;
				passesNumActs = false;
			}
		}
		else
		{
			numActsAvg = -1;
		}
	}
	else
	{
		firstRow << "Male SA Pop" << Constants::TAB;
        numActs = numActsMonth[(std::size_t)DemographicProfile::Gender::Male]; //only male SA

		if(maleSA != 0)
		{
			numActsAvg = numActs / (double)maleSA;

			if(numActsAvg < parameters_.calibrationInputs.numActsBounds.lower
			        || numActsAvg > parameters_.calibrationInputs.numActsBounds.upper)
			{
				passesCalib = false;
				passesNumActs = false;
			}
		}
		else
		{
			numActsAvg = -1;
		}
	}

	//female to male ratios
    unsigned long numInCasualMale = numInPartnership[(int)SexualPartnership::Type::Casual][(std::size_t)DemographicProfile::Gender::Male];
    unsigned long numInCasualFemale = numInPartnership[(int)SexualPartnership::Type::Casual][(std::size_t)DemographicProfile::Gender::Female];
	double casualPartPrevRatio = -1;
	firstRow << Constants::TAB;

	if(maleSA != 0 && femaleSA != 0)
	{
		double casualPrevMale = numInCasualMale / (double)maleSA;
		double casualPrevFemale = numInCasualFemale / (double)femaleSA;

		if(casualPrevMale != 0)
		{
			casualPartPrevRatio = casualPrevFemale / casualPrevMale;
		}

		if(casualPrevFemale > (casualPrevMale * parameters_.calibrationInputs.femaleCasualPrevRatio))
		{
			passesCalib = false;
			passesCasualPartPrevRatio = false;
		}
	}

    unsigned long numInConcurrentMale = numInConcurrent[(std::size_t)DemographicProfile::Gender::Male];
    unsigned long numInConcurrentFemale = numInConcurrent[(std::size_t)DemographicProfile::Gender::Female];
	double propConcurrentRatio = -1;
	firstRow << Constants::TAB;

	if(maleSA != 0 && femaleSA != 0)
	{
		double propConcurrentMale = numInConcurrentMale / (double)maleSA;
		double propConcurrentFemale = numInConcurrentFemale / (double)femaleSA;

		if(propConcurrentMale != 0)
		{
			propConcurrentRatio = propConcurrentFemale / propConcurrentMale;
		}

		if(propConcurrentFemale > (propConcurrentMale * parameters_.calibrationInputs.femalePropInConcurrentRatio))
		{
			passesCalib = false;
			passesPropConcRatio = false;
		}
	}

    unsigned long numSAFemaleHR = numSexuallyActiveRisk[(std::size_t)DemographicProfile::Gender::Female][Person::HIGH];
    unsigned long numSAFemaleLR = numSexuallyActiveRisk[(std::size_t)DemographicProfile::Gender::Female][Person::LOW];
	double numActsFemaleLRtoHRRatio = -1;
	firstRow << Constants::TAB;

	if(numSAFemaleHR != 0 && numSAFemaleLR != 0)
	{
        double avgNumActsFemaleHR = numActsMonthRisk[(std::size_t)DemographicProfile::Gender::Female][Person::HIGH] / (double)numSAFemaleHR;
        double avgNumActsFemaleLR = numActsMonthRisk[(std::size_t)DemographicProfile::Gender::Female][Person::LOW] / (double)numSAFemaleLR;

		if(avgNumActsFemaleHR != 0)
		{
			numActsFemaleLRtoHRRatio = avgNumActsFemaleLR / avgNumActsFemaleHR;
		}

		if(avgNumActsFemaleLR > (avgNumActsFemaleHR * parameters_.calibrationInputs.femaleNumActsLRtoHRRatio))
		{
			passesCalib = false;
			passesNumActsLRtoHRRatio = false;
		}
	}

	firstRow << "Violation of Constraint (1=Condition Met, 0=Not Met)";
	thirdRow << steadyPrev << Constants::TAB << casualPrev << Constants::TAB << CSWPrev << Constants::TAB <<
	         propInConcurrent << Constants::TAB << numActsAvg << Constants::TAB << casualPartPrevRatio << Constants::TAB <<
	         propConcurrentRatio << Constants::TAB << numActsFemaleLRtoHRRatio << Constants::TAB;
	thirdRow << (int)passesSteadyPrev << Constants::TAB << (int)passesCasualPrev << Constants::TAB <<
	         (int)passesCSWPrev << Constants::TAB << (int)passesPropInConcurrent << Constants::TAB <<
	         (int)passesNumActs << Constants::TAB << (int)passesCasualPartPrevRatio << Constants::TAB <<
	         (int)passesPropConcRatio << Constants::TAB << (int)passesNumActsLRtoHRRatio << Constants::TAB;

	if(parameters_.trace_files[EventParams::TraceFile::Type::CalibrationStatistics].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::CalibrationStatistics] << firstRow.str() << std::endl << secondRow.str() << std::endl <<
		        thirdRow.str() << std::endl;
	}

	return passesCalib;
}

unsigned long Population::CreatePartnerships(EventParams &parameters_, Person *_initiator,
        std::list<Person *>::iterator * /*_p_Iter*/, SexualPartnership::Type _partnershipType, bool _forceNumPartnersOne)
{
	assert((_initiator != nullptr));
	assert((_initiator->isAlive()));
	//Boolean for determining whether we print this creation to singlePersonTrace
	bool printToTrace = false;

	if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && _initiator->trace())
	{
		printToTrace = true;
	}

	unsigned long numFormed = 0;
	//number of partners of _partnershipType for this particular person for this month
	int	numPartners;

	if(_forceNumPartnersOne)
	{
		numPartners = 1;
	}
	else
	{
		numPartners = _initiator->rollForNumPartners(parameters_.randomNums, _partnershipType);
	}

	if(printToTrace && numPartners > 0)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "+ Male " << _initiator->getID() << " attempts to form " <<
		        numPartners << " " << (SexualPartnership::TypeStrings.at(_partnershipType)) << " partnerships:" << std::endl;
	}

	while(numPartners > 0)
	{
		//Decrement numPartners
		numPartners--;
		//pick the bucket that we will attempt to choose from
		BucketDemographicProfile *bucket = potentialPartnerBuckets[_partnershipType].at(parameters_.randomNums.chooseIndex(
			Population::eligibleBucketWeights[_partnershipType]));
		assert(bucket != nullptr);
		std::list<Person *> attemptedPartners;
		bool foundPartner = false;
		Person *chosenPartner = nullptr;
		bool printTracePartner = false;

        int maxRejections = ((Male *)_initiator)->getMaxPartnershipRejections();

		//the partner that this man will have a relationship with
		//remove the partner from the pool will be added back later
		for(int i = 0; i < 10; i++)
		{
			Person *partner = bucket->drawMember(parameters_.randomNums, _initiator, _partnershipType, true);

			if(partner == nullptr)
			{
				if(printToTrace)
				{
					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "  +x Male " << _initiator->getID() <<
					        " attempted to draw from empty bucket" << std::endl;
					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "   x Partnership not formed!" << std::endl;
				}

				//This partnership will not be formed: increase the number of unformed partnerships
				_initiator->increaseUnformedPartnershipTallies(_partnershipType);
				break;
			}

            if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && partner->trace())
			{
				parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "+ Female " << partner->getID() << " is chosen for a " << 
				        (SexualPartnership::TypeStrings.at(_partnershipType)) << " partnership:" << std::endl;
				printTracePartner = true;
			}

			assert(partner != nullptr);
			assert(partner->isAlive());
			attemptedPartners.push_back(partner);

			//if we tried to draw someone we are already seeing, then redraw until we pick someone new
			if(_initiator->isPartneredWith(partner))
			{
                if(parameters_.debugLevel > DebugLevel::One)
				{
					_initiator->print(cerr, "");
					cerr << _initiator->getID() << " Attempted repeat partnership with " << partner->getID() << ", " <<
					     (SexualPartnership::TypeStrings.at(_partnershipType)) << std::endl;
					_initiator->printCurrentPartners(cerr, "");
				}

                if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (_initiator->trace() || partner->trace()))
				{
					if(_initiator->trace())
					{
                        if(_initiator->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
						{
							parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "  +x Male ";
						}
						else
						{
							parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "  +x Female ";
						}

						parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << _initiator->getID() << " attempted repeat partnership with " <<
						        partner->getID() << std::endl;
					}
					else
					{
                        if(partner->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Female)
						{
							parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "  +x Female ";
						}
						else
						{
							parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "  +x Male ";
						}

						parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << partner->getID() << " was selected *again* by " <<
						        _initiator->getID() << std::endl;
					}

					parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "   x Repeat partnership not formed!" << std::endl;
				}
			}
			else if(partner != _initiator && popWideParams.ageOfMajority + partner->GetSexualActivityDelay() <= partner->getAge(TimeGranularity::Month))
			{
                auto rejectionChance = partner->GetPartnershipRejectionChance(_initiator->getRiskLevel(), _partnershipType);

                if(rejectionChance == 0 || !parameters_.randomNums.chance(rejectionChance)) // not rejected or partner rejection not set
                {
                    foundPartner = true;
                    chosenPartner = partner;
                    break;
                }
                else if(maxRejections != 0 && --maxRejections == 0) // rejected -> max reached?
                {
                    break;
                }
			}
		}

		if(!foundPartner)
		{
			//This partnership will not be formed: increase the number of unformed partnerships
			_initiator->increaseUnformedPartnershipTallies(_partnershipType);

			for(std::list<Person *>::iterator it = attemptedPartners.begin(); it != attemptedPartners.end(); it++)
			{
				//If woman was removed from entity pool, put her back!
				if((*it)->getCurrBucketProfileID() == DemographicProfile::END)
				{
					entities->addEntity((*it));
				}
			}

			continue;
		}

		if(printToTrace || printTracePartner)
		{
			parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << "  + Male " << _initiator->getID() << " (";
			_initiator->getDemographicProfile()->print(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].file, "");
            parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " age " << _initiator->getAge(TimeGranularity::Month) << ", " 
                << chosenPartner->getSexualActivity() << " marbles, " << ((chosenPartner->getRiskLevel() == Person::HIGH) ? "HIGH" : "LOW") << " risk) forms " <<
			        (SexualPartnership::TypeStrings.at(_partnershipType)) << " with female " << chosenPartner->getID() << " (";
			chosenPartner->getDemographicProfile()->print(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].file, "");
			parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " age " << chosenPartner->getAge(TimeGranularity::Month) 
                << ", " << chosenPartner->getSexualActivity() << " marbles, " << ((chosenPartner->getRiskLevel() == Person::HIGH) ? "HIGH" : "LOW") << " risk)";
		}

		//the pointer to this partnership will be stored within initiator.
		new SexualPartnership(_initiator, chosenPartner, parameters_, _partnershipType);

		//add all persons back to entity pool
		for(std::list<Person *>::iterator it = attemptedPartners.begin(); it != attemptedPartners.end(); it++)
		{
			//If woman was removed from entity pool, put her back!
			if((*it)->getCurrBucketProfileID() == DemographicProfile::END)
			{
				entities->addEntity((*it));
			}
		}

		if(!_initiator->inCorrectBucketDemographicProfile())
		{
			entities->refreshBucketDemographicProfile(_initiator, nullptr);
		}

		assert(chosenPartner->inCorrectBucketDemographicProfile());
		assert(_initiator->inCorrectBucketDemographicProfile());

		//increment if partnership was formed
		numFormed++;
	} //for(int partner = 0; partner < numPartners; partner++) {

	//TODO: Return difference between numFormed and original value of numPartners as well as numFormed... create field inside initiator
	return numFormed;
}

void Population::ProcessDeath(EventParams &parameters_, Person *_p, bool calculateLE)
{
	assert((_p != nullptr));
	assert((!_p->isAlive()));

    dead_people_this_month_.insert(_p);

	//Calculate life expectancy info
	if(calculateLE)
	{
		assert((populationStatistics.selectedLEStats != nullptr));
        assert(_p->getAge(TimeGranularity::Year) >= 0);
        assert(_p->getAge(TimeGranularity::Year) <= Person::maxYrForDeathStats);
        populationStatistics.selectedLEStats->deathsByAge[_p->getAge(TimeGranularity::Year)]++;
	}

	//print out this info to the trace
    if(parameters_.debugLevel > DebugLevel::One && parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
	{
		_p->print(parameters_.trace_files[EventParams::TraceFile::Type::Events].file, "Someone died: " + Constants::TAB);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && _p->trace())
	{
		_p->print(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].file, ">> Today we mourn: ");
	}

	//holds any former steady partners that are widowed after a partner's death
	// we may need to put the partners back into the singles pool
	std::list<SexualPartnership *> formerPartnerships;

	//we have to take care of what happens to any ongoing partnerships
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		_p->getPartnershipsToEnd(parameters_.currTime, SexualPartnership::Type(type), formerPartnerships, true);
	} //for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type) {

	DissolveSexualPartnerships(parameters_, _p, formerPartnerships);
	currDeathCauses[_p->deathStatus]++;
	populationStatistics.processDeath(_p, parameters_);

	delete _p;
}

long Population::CalcPrevalentPopulation(long _time)
{
	assert(_time >= 0);
	int totalInfected = 0;		//total infected in the while population
	//holds a pointer to the current bucket we are looking at
	BucketDemographicProfile *currBucket = nullptr;
	//holds number of prevalent infections
	unsigned long prevalenceByBucket[DemographicProfile::TotalNumBuckets][InfectionsTracker::NUMBER_GENERATIONS_TO_TRACE];
    unsigned long prevalenceByRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last];

	AgeRangeSizeContainer prevalenceByAgeMale, prevalenceByAgeFemale;

	//initialize prevalent infections by age
	for(auto ageBucketParams : popWideParams.initialAgeBuckets)
	{
		AgeRange range = {ageBucketParams.minAgeMth, ageBucketParams.maxAgeMth};
		prevalenceByAgeMale.push_back({range, 0});
		prevalenceByAgeFemale.push_back({range, 0});
	}

	//initialize prevalence tallies to 0
	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
	  for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
		{
            for(int k = 0; k < (int)DemographicProfile::Employment::Last; k++)
			{
				prevalenceByRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;

	while(currProfileID <= DemographicProfile::MAX)
	{
		for(int i = 0; i < populationStatistics.infectionsTracker.NUMBER_GENERATIONS_TO_TRACE; i++)
		{
			prevalenceByBucket[currProfileID][i] = 0;
		}

		currProfileID++;
	}

	//check if there is another bucket of entities to check
	//if there is a bucket, then iterate through people in bucket
	currProfileID = DemographicProfile::MIN;

	while(currProfileID <= DemographicProfile::MAX)
	{
		currBucket = entities->getBucket(currProfileID);

		//if people of this particular profile don't exist in the population, move on.
		if((currBucket == nullptr) || (currBucket->size() == 0))
		{
			currProfileID++;
			continue;
		}

		for(int generation = Constants::PREVALENT_INFECTION;
		        generation < populationStatistics.infectionsTracker.NUMBER_GENERATIONS_TO_TRACE; generation++)
		{
			//count number of infected in bucket
			prevalenceByBucket[currProfileID][generation] = currBucket->getNumInfected(generation);
			//count total # of infected people
			totalInfected += prevalenceByBucket[currProfileID][generation];
		}

		//add the sizes of sexually active buckets
        if(DemographicProfile::get(currBucket->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::Active)
		{
			//Update size by risk
			for(int i = 0; i < Person::ENDRiskLevel; i++)
			{
				prevalenceByRiskGenderEmployment[i][DemographicProfile::get(currBucket->getProfileID(),
				                                    DemographicProfile::Demographic::Gender)][DemographicProfile::get(currBucket->getProfileID(),
				                                            DemographicProfile::Demographic::Employment)] += ((BucketSexualMixing *)currBucket)->getNumInfected((Person::RiskLevel) i);
			}

			//Update size by age range
            bool isMale = DemographicProfile::get(currBucket->getProfileID(), DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male;
			auto &associatedAgeRangePrevalenceContainer = isMale ? prevalenceByAgeMale : prevalenceByAgeFemale;
			auto currentSexualMixingBucket = static_cast<BucketSexualMixing *>(currBucket);

			for(auto &ageRangeSize : associatedAgeRangePrevalenceContainer)
			{
				auto prevalentInAgeGroup = currentSexualMixingBucket->sizeInfectedByAge(ageRangeSize.first.lower, ageRangeSize.first.upper);
				ageRangeSize.second += prevalentInAgeGroup;
			}
		}

		currProfileID++;
	}

	//save the prevalent infections by bucket in the PopulationStatistics
	populationStatistics.infectionsTracker.setPrevalentInfections(_time, prevalenceByBucket, prevalenceByAgeMale, prevalenceByAgeFemale, prevalenceByRiskGenderEmployment);

	return totalInfected;
}

PopulationParameters::AgeBucketPrevalenceInfo &Population::GetAgeBucket(Person *p)
{
    int age = p->getAge(TimeGranularity::Month);

	for(unsigned int ageBucket = 0; ageBucket < popWideParams.initialAgeBuckets.size(); ageBucket++)
	{
		//holds the parameters for the current age bucket
		auto &ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucket);

		if(age >= ageBucketParams.minAgeMth && age <= ageBucketParams.maxAgeMth)
		{
			return ageBucketParams;
		}
	}

	throw std::runtime_error("bucket not found");
	//return popWideParams.initialAgeBuckets.back();
}

int Population::GetAgeBucketIndex(Person *p)
{
    int age = p->getAge(TimeGranularity::Month);
	unsigned int ageBucket;

	for(ageBucket = 0; ageBucket < popWideParams.initialAgeBuckets.size(); ageBucket++)
	{
		//holds the parameters for the current age bucket
		auto &ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucket);

		if(age >= ageBucketParams.minAgeMth && age <= ageBucketParams.maxAgeMth)
		{
			return ageBucket;
		}
	}

	throw std::runtime_error("bucket not found");
	//return ageBucket;
}

std::size_t Population::GetSize()
{
	return currSize;
}

std::size_t Population::GetNASize()
{
	return currNASize;
}

std::size_t Population::GetSize(DemographicProfile::Gender gender)
{
    return currSizeGender[(std::size_t)gender];
}

std::size_t Population::GetSASize(DemographicProfile::Gender _gender, Person::RiskLevel _risk)
{
    return currSASizeGenderRisk[(std::size_t)_gender][(std::size_t)_risk];
}

std::size_t Population::GetCSWSize(DemographicProfile::Gender _gender, Person::RiskLevel _risk)
{
    return currSizeGenderRiskCSW[(std::size_t)_gender][(std::size_t)_risk];
}

void Population::PrintMethodResults(EventParams &parameters_, const std::string &_methodName, const std::string &_eventLabel,
    long _totalAffected, const std::string &_totalAffectedLabel, bool _showInfections)
{
	unsigned long totalInfected = 0;
	unsigned long totalPopSize = 0;
	unsigned long totalInSteady = 0;
	unsigned long totalInRegular = 0;
	unsigned long totalSexuallyActive = 0;
	_showInfections = false;

    if(parameters_.debugLevel > DebugLevel::Zero && parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
	{
		//if we are at time 0, then print out headers
        if((parameters_.currTime == 0) && (parameters_.debugLevel == DebugLevel::One))
		{
			//print out headers for DEBUG level 1 in the BucketDemographicProfile size trace
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Time\t" << "EventLabel\tNumAffected\t";
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Currently Infected" << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Current Population Size" << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Sexually Active Population" << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Steady Partnership Population" << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Regular Partnership Population" << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << Constants::TAB;
			entities->printBucketLabels(parameters_.trace_files[EventParams::TraceFile::Type::Events].file, _showInfections);
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << std::endl;
		} //if( parameters_.currTime == 0 ) {

		//print out the sizes of the buckets to a string stream
		std::ostringstream bucketTotalsStr;
		entities->printBucketSizes(bucketTotalsStr, Constants::TAB, _showInfections, totalInfected, totalPopSize,
            totalSexuallyActive, totalInSteady, totalInRegular, (parameters_.debugLevel > DebugLevel::One));

		//if debug level > 1, then print trace format in verbose form and include labels
        if(parameters_.debugLevel > DebugLevel::One)
		{
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "T: " << parameters_.currTime << " -- " << _methodName << ": " <<
			        endl;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << Constants::TAB << _eventLabel;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << _totalAffectedLabel << "= " << _totalAffected << std::endl;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Current Person Pool Sizes:\t";
		}
		else
		{
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << parameters_.currTime << Constants::TAB << _eventLabel <<
			        Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << _totalAffected << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << totalInfected << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << totalPopSize << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << totalSexuallyActive << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << totalInSteady << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << totalInRegular << Constants::TAB;
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << Constants::TAB;
		}//if( _d > One) {

		parameters_.trace_files[EventParams::TraceFile::Type::Events] << bucketTotalsStr.str();

        if(parameters_.debugLevel > DebugLevel::One)
		{
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << std::endl;
		}

		//print out all people in the population
        if(parameters_.debugLevel > DebugLevel::Two)
		{
			parameters_.trace_files[EventParams::TraceFile::Type::Events] << std::endl;
			entities->print(parameters_.trace_files[EventParams::TraceFile::Type::Events].file);
		}

		parameters_.trace_files[EventParams::TraceFile::Type::Events] << std::endl;
	}
}

void Population::PrintPartnerships(EventParams &parameters_, long _time, std::ostream &_outStream)
{
	assert(_time >= 0);
	std::string genderLabels[] = { "Male", "Female" };
	std::string employmentLabels[] = { "Non-CSW", "CSW" };
	std::string riskLabels[] = { "LR", "HR" };
	std::string relationshipLabels[] = { "Non-Single", "Single" };
	std::string partnershipLabels[] = { "Steady", "Regular", "Casual", "CSW" };
	std::string riskLabels2[] = { "HR", "Mix", "LR" };

	if(_time == 0)
	{
		std::ostringstream firstRow;
		std::ostringstream secondRow;
		std::ostringstream thirdRow;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Month" << Constants::TAB;
		//Partnership Headers
		//Steady Partnerships
		firstRow << "Individuals by Partnership" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Steady" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB;

        for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
		{
            for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
			{
				for(int m = Person::HIGH; m >= 0; m--)
				{
					thirdRow << genderLabels[j] << Constants::SPACE << "Non-Single" << Constants::SPACE << employmentLabels[l] <<
					         Constants::SPACE << riskLabels[m] << Constants::TAB;
				}
			}
		}

		//Regular Partnerships
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Regular" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;

        for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
		{
            for(int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
			{
                for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
				{
					for(int m = Person::HIGH; m >= 0; m--)
					{
						thirdRow << genderLabels[j] << Constants::SPACE << relationshipLabels[k] << Constants::SPACE << employmentLabels[l] <<
						         Constants::SPACE << riskLabels[m] << Constants::TAB;
					}
				}
			}
		}

		//Casual Partnerships
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Casual" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;

        for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
		{
            for(int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
			{
                for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
				{
					for(int m = Person::HIGH; m >= 0; m--)
					{
						thirdRow << genderLabels[j] << Constants::SPACE << relationshipLabels[k] << Constants::SPACE << employmentLabels[l] <<
						         Constants::SPACE << riskLabels[m] << Constants::TAB;
					}
				}
			}
		}

		//CSW Partnerships
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "CSW" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;

        for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
		{
		  for(int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
			{
                for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
				{
					for(int m = Person::HIGH; m >= 0; m--)
					{
						thirdRow << genderLabels[j] << Constants::SPACE << relationshipLabels[k] << Constants::SPACE << employmentLabels[l] <<
						         Constants::SPACE << riskLabels[m] << Constants::TAB;
					}
				}
			}
		}

		//Concurrent Partnerships
		//CSW
		firstRow << "Individuals by Concurrent Partnerships" << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB;
		secondRow << "CSW HR" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "Concurrent" << Constants::TAB << "2 Partners" << Constants::TAB << "3 Partners" << Constants::TAB <<
		         "4 Partners" << Constants::TAB << "5+ Partners" << Constants::TAB;
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "CSW LR" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "Concurrent" << Constants::TAB << "2 Partners" << Constants::TAB << "3 Partners" << Constants::TAB <<
		         "4 Partners" << Constants::TAB << "5+ Partners" << Constants::TAB;
		//High Risk Male Non-CSW
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Male High Risk Non-CSW" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB;;
		thirdRow << "Concurrent" << Constants::TAB << "2 Partners" << Constants::TAB << "3 Partners" << Constants::TAB <<
		         "4 Partners" << Constants::TAB << "5+ Partners" << Constants::TAB;
		//High Risk Female Non-CSW
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Female High Risk Non-CSW" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB;;
		thirdRow << "Concurrent" << Constants::TAB << "2 Partners" << Constants::TAB << "3 Partners" << Constants::TAB <<
		         "4 Partners" << Constants::TAB << "5+ Partners" << Constants::TAB;
		//Low Risk Male Non-CSW
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Male Low Risk Non-CSW" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB;;
		thirdRow << "Concurrent" << Constants::TAB << "2 Partners" << Constants::TAB << "3 Partners" << Constants::TAB <<
		         "4 Partners" << Constants::TAB << "5+ Partners" << Constants::TAB;
		//Low Risk Female Non-CSW
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "Female Low Risk Non-CSW" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB;;
		thirdRow << "Concurrent" << Constants::TAB << "2 Partners" << Constants::TAB << "3 Partners" << Constants::TAB <<
		         "4 Partners" << Constants::TAB << "5+ Partners" << Constants::TAB;
		//Partnerships by partnership type
		firstRow << "Partnerships by Partnership Type" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB
		         << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB;
		secondRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;

		for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
		{
			for(int j = 0; j < 3; j++)
			{
				thirdRow << partnershipLabels[i] << Constants::SPACE << riskLabels2[j] << Constants::TAB;
			}
		}

		//write out string buffers to trace file
		_outStream << firstRow.str() << std::endl;
		_outStream << secondRow.str() << std::endl;
		_outStream << thirdRow.str() << std::endl;
	}

	//Month
	if(_time == 0)
	{
		_outStream << "Init" << Constants::TAB;
	}
	else
	{
		_outStream << _time << Constants::TAB;
	}

	//Partnerships
	unsigned long
        numInPartnership[(int)SexualPartnership::Type::ENDType][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::RelationshipStatus::Last][(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];
    unsigned long numInConcurrent[(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel];
    unsigned long numInMultiple[(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last][Person::ENDRiskLevel][4];
	unsigned long doubleNumPartnerships[(int)SexualPartnership::Type::ENDType][3];

    for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
	{
        for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
		{
            for(int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
			{
                for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
				{
					for(int m = 0; m < Person::ENDRiskLevel; m++)
					{
						numInPartnership[i][j][k][l][m] = 0;
					}
				}
			}
		}

		for(int j = 0; j < 3; j++)
		{
			doubleNumPartnerships[i][j] = 0;
		}
	}

    for(int i = 0; i < (int)DemographicProfile::Gender::Last; i++)
	{
        for(int j = 0; j < (int)DemographicProfile::Employment::Last; j++)
		{
			for(int k = 0; k < Person::ENDRiskLevel; k++)
			{
				numInConcurrent[i][j][k] = 0;

				for(int m = 0; m < 4; m++)
				{
					numInMultiple[i][j][k][m] = 0;
				}
			}
		}
	}

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		std::list<Person *>::iterator p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			int numPartners[(int)SexualPartnership::Type::ENDType];
			bool hasType[(int)SexualPartnership::Type::ENDType];
			int totalNumPartners = 0;
			Person::RiskLevel risk = (*p_Iter)->getRiskLevel();

			for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
			{
				numPartners[i] = (*p_Iter)->getNumPartners((SexualPartnership::Type)i);
				hasType[i] = (numPartners[i] != 0);
				int j = 0;

				if(risk == Person::LOW)
				{
					j = 2;
				}

				doubleNumPartnerships[i][1] += (*p_Iter)->getNumPartners((SexualPartnership::Type) i, false);
				doubleNumPartnerships[i][j] += (*p_Iter)->getNumPartners((SexualPartnership::Type) i, true);
				totalNumPartners += numPartners[i];
			}

			if(totalNumPartners >= 2 && totalNumPartners <= 4)
			{
                numInMultiple[(std::size_t)gender][(*p_Iter)->getDemographicProfileVal(DemographicProfile::Demographic::Employment)][(*p_Iter)->getRiskLevel()][totalNumPartners -
				        2]++;
			}
			else if(totalNumPartners >= 5)
			{
                numInMultiple[(std::size_t)gender][(*p_Iter)->getDemographicProfileVal(DemographicProfile::Demographic::Employment)][(*p_Iter)->getRiskLevel()][3]++;
			}

			for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
			{
				if(numPartners[i] != 0)
				{
                    numInPartnership[i][(std::size_t)gender][(*p_Iter)->getDemographicProfileVal(DemographicProfile::Demographic::RelationshipStatus)][(*p_Iter)->getDemographicProfileVal(
					            DemographicProfile::Demographic::Employment)][(*p_Iter)->getRiskLevel()]++;
				}
			}

			//tally concurrent partners
			//create a number between 0 and 15 representing the combination of partnership types person has
			//e.g. if person has partnerships steady and casual concurrent will equal 8+2=10
			int concurrent = 0;

			for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
			{
				concurrent = (concurrent << 1) + (hasType[i] ? 1 : 0);
			}

			concurrent = 15 - concurrent;
			assert(concurrent <= Constants::NUMBER_CONCURRENCY_DEFS);

			if(parameters_.concurrencyDef[concurrent].useDefinition
			        && totalNumPartners >= parameters_.concurrencyDef[concurrent].minPartnershipsNeeded)
			{
				//count as concurrent partnership
                numInConcurrent[(std::size_t)gender][(*p_Iter)->getDemographicProfileVal(DemographicProfile::Demographic::Employment)][(*p_Iter)->getRiskLevel()]++;
			}

			p_Iter++;
		}
	}

	//steady
    for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
	{
        for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
		{
			for(int m = Person::HIGH; m >= 0; m--)
			{
                _outStream << numInPartnership[(int)SexualPartnership::Type::Steady][j][(std::size_t)DemographicProfile::RelationshipStatus::NonSingle][l][m] << Constants::TAB;
			}
		}
	}

	//regular
    for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
	{
        for(int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
		{
            for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
			{
				for(int m = Person::HIGH; m >= 0; m--)
				{
					_outStream << numInPartnership[(int)SexualPartnership::Type::Regular][j][k][l][m] << Constants::TAB;
				}
			}
		}
	}

	//casual
    for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
	{
        for(int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
		{
            for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
			{
				for(int m = Person::HIGH; m >= 0; m--)
				{
					_outStream << numInPartnership[(int)SexualPartnership::Type::Casual][j][k][l][m] << Constants::TAB;
				}
			}
		}
	}

	//CSW
    for(int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
	{
        for(int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
		{
            for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
			{
				for(int m = Person::HIGH; m >= 0; m--)
				{
					_outStream << numInPartnership[(int)SexualPartnership::Type::Csw][j][k][l][m] << Constants::TAB;
				}
			}
		}
	}

	//Concurrent Partnerships
	//CSW
	unsigned long numInConcurrentCSW[Person::ENDRiskLevel];
	unsigned long numInMultipleCSW[Person::ENDRiskLevel][4];

	for(int k = 0; k < Person::ENDRiskLevel; k++)
	{
		numInConcurrentCSW[k] = 0;

		for(int m = 0; m < 4; m++)
		{
			numInMultipleCSW[k][m] = 0;
		}
	}

    for(int i = 0; i < (int)DemographicProfile::Gender::Last; i++)
	{
		for(int k = 0; k < Person::ENDRiskLevel; k++)
		{
            numInConcurrentCSW[k] += numInConcurrent[i][(std::size_t)DemographicProfile::Employment::Csw][k];

			for(int m = 0; m < 4; m++)
			{
                numInMultipleCSW[k][m] += numInMultiple[i][(std::size_t)DemographicProfile::Employment::Csw][k][m];
			}
		}
	}

	for(int k = Person::HIGH; k >= 0; k--)
	{
		_outStream << numInConcurrentCSW[k] << Constants::TAB;

		for(int m = 0; m < 4; m++)
		{
			_outStream << numInMultipleCSW[k][m] << Constants::TAB;
		}
	}

	//High Risk Males Non CSW
    _outStream << numInConcurrent[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw][Person::HIGH] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
        _outStream << numInMultiple[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw][Person::HIGH][m] << Constants::TAB;
	}

	//High Risk Females Non CSW
    _outStream << numInConcurrent[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw][Person::HIGH] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
        _outStream << numInMultiple[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw][Person::HIGH][m] << Constants::TAB;
	}

	//Low Risk Males Non CSW
    _outStream << numInConcurrent[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw][Person::LOW] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
        _outStream << numInMultiple[(std::size_t)DemographicProfile::Gender::Male][(std::size_t)DemographicProfile::Employment::NonCsw][Person::LOW][m] << Constants::TAB;
	}

	//Low Risk Females Non CSW
    _outStream << numInConcurrent[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw][Person::LOW] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
        _outStream << numInMultiple[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)DemographicProfile::Employment::NonCsw][Person::LOW][m] << Constants::TAB;
	}

	//Partnerships by type
	for(int i = 0; i < 4; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			_outStream << doubleNumPartnerships[i][j] / 2 << Constants::TAB;
		}
	}

	_outStream << std::endl;
}

void Population::PrintClinical(EventParams &/*parameters_*/, long _time, std::ostream &_outStream)
{
	assert(_time >= 0);

	if(_time == 0)
	{
		std::ostringstream firstRow;
		std::ostringstream secondRow;
		std::ostringstream thirdRow;
		firstRow << Constants::TAB;
		secondRow << Constants::TAB;
		thirdRow << "Month" << Constants::TAB;
		//HIV Status among all SA pop
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among Sexually Active Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//CSW
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among CSW SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//High Risk
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among High Risk Non-CSW SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//Low Risk
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among Low Risk Non-CSW SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//Male
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among Male SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//Male High Risk
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among High Risk Male SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//Male Low Risk
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among Low Risk Male SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//Female
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among Female SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//Female High Risk
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among High Risk Female SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//Female Low Risk
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB;
		secondRow << "HIV Status Among Low Risk Female SA Population" << Constants::TAB << Constants::TAB << Constants::TAB <<
		          Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "HIV-" << Constants::TAB << "Observed Acute" << Constants::TAB << "Unobserved Acute" << Constants::TAB <<
		         "Observed Chronic" << Constants::TAB << "Unobserved Chronic" << Constants::TAB << "Observed Latestage" << Constants::TAB
		         << "Unobserved Latestage" << Constants::TAB;
		//CD4 At Transmission
		firstRow << Constants::TAB << Constants::TAB << Constants::TAB;
		secondRow << "CD4 At Transmission" << Constants::TAB << Constants::TAB << Constants::TAB;
		thirdRow << "Num Transmissions" << Constants::TAB << "Mean CD4" << Constants::TAB << "SD CD4" << Constants::TAB;
		_outStream << firstRow.str() << std::endl;
		_outStream << secondRow.str() << std::endl;
		_outStream << thirdRow.str() << std::endl;
	}

	//Month
	if(_time == 0)
	{
		_outStream << "Init" << Constants::TAB;
	}
	else
	{
		_outStream << _time << Constants::TAB;
	}

	unsigned long
        numWithHIVStatus[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Employment::Last][(std::size_t)DemographicProfile::Gender::Last][Person::ENDHIVStatus];

	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
        for(int j = 0; j < (int)DemographicProfile::Employment::Last; j++)
		{
            for(int k = 0; k < (int)DemographicProfile::Gender::Last; k++)
			{
				for(int m = 0; m < Person::ENDHIVStatus; m++)
				{
					numWithHIVStatus[i][j][k][m] = 0;
				}
			}
		}
	}

	//iterate through bucket
	//holds a pointer to the current bucket we are looking at
	BucketDemographicProfile *currBucket = nullptr;
	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;

	while(currProfileID <= DemographicProfile::MAX)
	{
		currBucket = entities->getBucket(currProfileID);

		//if people of this particular profile don't exist in the population, move on.
		if((currBucket == nullptr) || (currBucket->size() == 0))
		{
			currProfileID++;
			continue;
		}

		//accumulate all SA buckets
        if(DemographicProfile::get(currBucket->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) == (std::size_t)DemographicProfile::SexualActivityStatus::Active)
		{
			for(int i = 0; i < Person::ENDRiskLevel; i++)
			{
                for(int m = 0; m < Person::ENDHIVStatus; m++)
                {
                    numWithHIVStatus[i][DemographicProfile::get(currBucket->getProfileID(),
                        DemographicProfile::Demographic::Employment)][DemographicProfile::get(currBucket->getProfileID(),
                        DemographicProfile::Demographic::Gender)][m] += ((BucketSexualMixing *)currBucket)->sizeRiskHIVStatus((Person::RiskLevel) i,
                        (Person::HIVStatus) m);
                }
			}
		}

		currProfileID++;
	}

	//HIV Status of entire SA population
	unsigned long hivStatusSA[Person::ENDHIVStatus];

	for(int m = 0; m < Person::ENDHIVStatus; m++)
	{
		hivStatusSA[m] = 0;
	}

	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
        for(int j = 0; j < (int)DemographicProfile::Employment::Last; j++)
		{
            for(int k = 0; k < (int)DemographicProfile::Gender::Last; k++)
			{
				for(int m = 0; m < Person::ENDHIVStatus; m++)
				{
					hivStatusSA[m] += numWithHIVStatus[i][j][k][m];
				}
			}
		}
	}

	for(int m = 0; m < Person::ENDHIVStatus; m++)
	{
		_outStream << hivStatusSA[m] << Constants::TAB;
	}

	//HIV Status of CSW SA population
	unsigned long hivStatusCSW[Person::ENDHIVStatus];

	for(int m = 0; m < Person::ENDHIVStatus; m++)
	{
		hivStatusCSW[m] = 0;
	}

	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
        for(int k = 0; k < (int)DemographicProfile::Gender::Last; k++)
		{
			for(int m = 0; m < Person::ENDHIVStatus; m++)
			{
                hivStatusCSW[m] += numWithHIVStatus[i][(std::size_t)DemographicProfile::Employment::Csw][k][m];
			}
		}
	}

	for(int m = 0; m < Person::ENDHIVStatus; m++)
	{
		_outStream << hivStatusCSW[m] << Constants::TAB;
	}

	//HIV Status of Risk Non CSW SA population
	for(int i = Person::HIGH; i >= 0; i--)
	{
		unsigned long hivStatusRisk[Person::ENDHIVStatus];

		for(int m = 0; m < Person::ENDHIVStatus; m++)
		{
			hivStatusRisk[m] = 0;
		}

        for(int k = 0; k < (int)DemographicProfile::Gender::Last; k++)
		{
			for(int m = 0; m < Person::ENDHIVStatus; m++)
			{
                hivStatusRisk[m] += numWithHIVStatus[i][(std::size_t)DemographicProfile::Employment::NonCsw][k][m];
			}
		}

		for(int m = 0; m < Person::ENDHIVStatus; m++)
		{
			_outStream << hivStatusRisk[m] << Constants::TAB;
		}
	}

	//HIV Status of SA population by Gender and Risk
    for(int k = 0; k < (int)DemographicProfile::Gender::Last; k++)
	{
		unsigned long hivStatusGender[Person::ENDHIVStatus];
		unsigned long hivStatusGenderRisk[Person::ENDRiskLevel][Person::ENDHIVStatus];

		for(int m = 0; m < Person::ENDHIVStatus; m++)
		{
			hivStatusGender[m] = 0;

			for(int i = 0; i < Person::ENDRiskLevel; i++)
			{
				hivStatusGenderRisk[i][m] = 0;
			}
		}

		for(int i = 0; i < Person::ENDRiskLevel; i++)
		{
            for(int j = 0; j < (int)DemographicProfile::Employment::Last; j++)
			{
				for(int m = 0; m < Person::ENDHIVStatus; m++)
				{
					hivStatusGender[m] += numWithHIVStatus[i][j][k][m];
					hivStatusGenderRisk[i][m] += numWithHIVStatus[i][j][k][m];
				}
			}
		}

		for(int m = 0; m < Person::ENDHIVStatus; m++)
		{
			_outStream << hivStatusGender[m] << Constants::TAB;
		}

		for(int i = Person::HIGH; i >= 0; i--)
		{
			for(int m = 0; m < Person::ENDHIVStatus; m++)
			{
				_outStream << hivStatusGenderRisk[i][m] << Constants::TAB;
			}
		}
	}

	populationStatistics.infectionsTracker.recordCD4AtTransmission(_outStream);
	_outStream << std::endl;
}

void Population::PrintPopulation(EventParams &/*parameters_*/, long _time, std::ostream &_outStream)
{
	assert(_time >= 0);
	//total # of age ranges to print out
	auto &currSizeByAgeRange = GetSizeByAgeRange();
	int numAgeRanges = (int)currSizeByAgeRange.size();

	//write headers for infections sheet
	if(_time == 0)
	{
		std::ostringstream firstRow;
		std::ostringstream secondRow;
		//incidence-related headers
		firstRow << Constants::TAB;
		secondRow << "Month" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << "Pop Size" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << "SA Pop Size" << Constants::TAB;
		firstRow << "Deaths" << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
		         Constants::TAB << Constants::TAB;
		secondRow << "Any OI" << Constants::TAB << "Chronic AIDS" << Constants::TAB << "Non-AIDS" << Constants::TAB <<
		          "ART Toxicity" << Constants::TAB << "Proph Toxicity" << Constants::TAB << "Other" << Constants::TAB << "Total" <<
		          Constants::TAB;
		firstRow << "Gender" << Constants::TAB;
		secondRow << "Males" << Constants::TAB;
		firstRow << Constants::TAB;
		secondRow << "Females" << Constants::TAB;
		firstRow << "Risk Group (SA Pop)" << Constants::TAB << Constants::TAB;
		secondRow << "CSW HR" << Constants::TAB << "CSW LR" << Constants::TAB;
		firstRow << Constants::TAB << Constants::TAB;
		secondRow << "Non-CSW High Risk Male" << Constants::TAB << "Non-CSW High Risk Female" << Constants::TAB;
		firstRow << Constants::TAB << Constants::TAB;
		secondRow << "Non-CSW Low Risk Male" << Constants::TAB << "Non-CSW Low Risk Female" << Constants::TAB;
		//write out headers for population by age
		firstRow << "Non-SA Pop (All Ages)" << Constants::TAB;
		secondRow << "All ages" << Constants::TAB;

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "SA Pop (Age Months)";
			}

			firstRow << Constants::TAB;
			secondRow << currSizeByAgeRange.at(i).first.lower << "-" << currSizeByAgeRange.at(i).first.upper << Constants::TAB;
		}

		firstRow << "Male (By Age)" << Constants::TAB;
		secondRow << "Non-SA" << Constants::TAB;

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "SA Pop (Age Months)";
			}

			firstRow << Constants::TAB;
			secondRow << currSizeByAgeRange.at(i).first.lower << "-" << currSizeByAgeRange.at(i).first.upper << Constants::TAB;
		}

		firstRow << "Female (By Age)" << Constants::TAB;
		secondRow << "Non-SA" << Constants::TAB;

		for(int i = 0; i < numAgeRanges; i++)
		{
			if(i == 0)
			{
				firstRow << "SA Pop (Age Months)";
			}

			firstRow << Constants::TAB;
			secondRow << currSizeByAgeRange.at(i).first.lower << "-" << currSizeByAgeRange.at(i).first.upper << Constants::TAB;
		}

		//write out string buffers to trace file
		_outStream << firstRow.str() << "Number circumcised" << std::endl;
		_outStream << secondRow.str() << "NA" << Constants::TAB << "SA" << std::endl;
	} //if( _time == 0) {

	//Month
	if(_time == 0)
	{
		_outStream << "init" << Constants::TAB;
	}
	else
	{
		_outStream << _time << Constants::TAB;
	}

	//output population size and SA pop size
	_outStream << GetSize() << Constants::TAB;
	_outStream << GetSize() - GetNASize() << Constants::TAB;

	//output deaths by causes
	_outStream << currDeathCauses[Person::DTH_OI] << Constants::TAB << currDeathCauses[Person::DTH_CHRAIDS] <<
	           Constants::TAB << currDeathCauses[Person::DTH_NONAIDS] << Constants::TAB <<
	           currDeathCauses[Person::DTH_TOX_ART] << Constants::TAB << currDeathCauses[Person::DTH_TOX_PROPH] <<
	           Constants::TAB << currDeathCauses[Person::DTH_OTHER] << Constants::TAB;

    std::size_t totalDeaths = 0;

	for(int i = 1; i < Person::ENDDeathStatus; i++)
	{
		totalDeaths += currDeathCauses[i];
	}

	_outStream << totalDeaths << Constants::TAB;
	//output size of male and female populations
	_outStream << GetSize(DemographicProfile::Gender::Male) << Constants::TAB;
	_outStream << GetSize(DemographicProfile::Gender::Female) << Constants::TAB;
	//output size by risk
	_outStream << currSizeRiskCSW[Person::HIGH] << Constants::TAB << currSizeRiskCSW[Person::LOW] <<
	           Constants::TAB;

	for(int i = Person::HIGH; i >= 0; i--)
	{
        for(int j = 0; j < (int)DemographicProfile::Gender::Last; j++)
        {
            _outStream << GetSASize((DemographicProfile::Gender) j,
                (Person::RiskLevel) i) - currSizeGenderRiskCSW[j][i] << Constants::TAB;
        }
	}

	//output size by age
	_outStream << GetNASize() << Constants::TAB;

	for(auto &ageRangeSize : currSizeByAgeRange)
	{
		_outStream << ageRangeSize.second << Constants::TAB;
	}

	//output size by age and gender
	_outStream << currNASizeByGender[0] << Constants::TAB;

	for(auto &ageRangeSize : currSizeByAgeRangeMale)
	{
		_outStream << ageRangeSize.second << Constants::TAB;
	}

	_outStream << currNASizeByGender[1] << Constants::TAB;

	for(auto &ageRangeSize : currSizeByAgeRangeFemale)
	{
		_outStream << ageRangeSize.second << Constants::TAB;
	}

    _outStream << num_circumcised_na << Constants::TAB;
    _outStream << num_circumcised_sa << Constants::TAB;

	_outStream << std::endl;
}

void Population::SaveState(std::ostream &_outStream, long currTime)
{
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;
	bool isFirst = true;

	//Double loop: first iterate through the men, then the women
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Person *p = (*p_Iter);
			assert(p != nullptr);

			if(!isFirst)
			{
				_outStream << "," << std::endl << std::endl;
			}

			isFirst = false;
			_outStream << "{";
			//save the state of each person
			p->saveState(_outStream, currTime);
			_outStream << "}";
			p_Iter++;
		}
	}
}

void Population::PrintARTRolloutOutcomes(EventParams &parameters_, std::ostream &_outStream)
{
	populationStatistics.artTracker.printArtRolloutOutcomes(parameters_.currTime, _outStream, this);
}

/**
this is called at specified time points to record the partner history frequency
**/
void Population::RecordPartAcqFreq()
{
	assert(populationStatistics.selectedPartAcqStats != nullptr);
	std::list<Person *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			int numPartnersInHistory = (*p_Iter)->getNumPartnersInHistory();

			if(numPartnersInHistory >= PopulationStatistics::SinglePartAcqStats::NUM_PARTNER_BINS)
			{
				numPartnersInHistory = PopulationStatistics::SinglePartAcqStats::NUM_PARTNER_BINS - 1;
			}

			if(numPartnersInHistory < 0)
			{
				numPartnersInHistory = 0;
			}

			populationStatistics.selectedPartAcqStats->partnerFreq[numPartnersInHistory]++;
			p_Iter++;
		}
	}
}
