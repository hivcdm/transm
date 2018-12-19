#include <algorithm>
#include <cstddef>
#include <iostream>
#include <list>
#include <string>
#include <unordered_set>
#include <vector>

#include "population.hpp"
#include "constants.hpp"
#include "simulation.hpp"
#include "entities/female.hpp"
#include "entities/male.hpp"
#include "entities/sexualbehavior.hpp"
#include "statistics/infectionstracker.hpp"
#include "statistics/coststracker.hpp"
#include "utility/descriptive_stats_container.hpp"
#include "utility/utility.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

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
    //risk_group_changed_.push_back([&](const Entity *e) { trace_files_.at("single-person").RecordRiskGroupChanged(parameters.currTime, e); });
}


void Population::Circumcise(double proportion)
{
    std::vector<Entity *> people = FindNonCircumcised();

    if(people.empty())
	return;

    int males = GetSize(DemographicProfile::Gender::Male);
    int numberToCircumcise = Utility::round<int>(proportion * males);

    assert(numberToCircumcise > 0);

    for (Entity *p : people) {
    	Circumcise(p);
	numberToCircumcise--;
	if (numberToCircumcise == 0)
	    break;
    }
}

void Population::Circumcise(Entity *p)
{
    if (!p->IsCircumcised())
    {
	double discount = 1.0;
	if (parameters_.useRollout) {
	    discount = p->getCepacDiscountFactor(parameters_);
        }
 	p->Circumcise();
	populationStatistics.costsTracker.RecordCircumcision(popWideParams.circumcisionCost,
            popWideParams.circumcisionCost * discount);
	p->add_cdm_cost(popWideParams.circumcisionCost,
	    popWideParams.circumcisionCost * discount);
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
	// Select ProfileID's of eligible initiators
	//all SA, non-CSW males can initiate partnerships of any type
	DemographicProfile selector;
	std::vector<DemographicProfile::ProfileID> eligibleInitiators;
    selector.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::Active);
    selector.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
    selector.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
	selector.selectProfileIDs(eligibleInitiators, nullptr);

	//For each initiator demographic profile, store the fact that they can have any partner type
	std::vector<SexualPartnership::Type> availPartnershipTypes;

	for(int type = 0; type < (int)SexualPartnership::Type::Last; ++type)
	{
		availPartnershipTypes.push_back(SexualPartnership::Type(type));
	}

	for(std::size_t i = 0; i < eligibleInitiators.size(); i++)
	{
		BucketSexualMixing *bucket = (BucketSexualMixing *)entities->getBucket(eligibleInitiators.at(i));

		//if this Bucket is nullptr, the skip
		if(bucket != nullptr)
		{
			partneringInitiators[bucket] = availPartnershipTypes;
			profilesToPartnershipTypes[bucket->getProfileID()] = availPartnershipTypes;
		}
	}

#if 0
	DemographicProfile currProfileSelector;
	std::vector<DemographicProfile::ProfileID> selectedIDs;

	for (auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
        try
        {
            //get the parameters for current relationship type
            assert(popWideParams.defaultMaleParams.hasSexualBehavior(partnership_type));
            const SexualBehavior &partneringParams = popWideParams.defaultMaleParams.getSexualBehavior(partnership_type);

            //get available demographicProfiles that are available for this partnership
            for(unsigned int j = 0; j < partneringParams.getNumAvailableBuckets(); ++j)
            {
		    selectedIDs.clear();
		    //contains profile ID's that were selected from
		    currProfileSelector.set(partneringParams.getAvailableBucket(j).dmgProfileSelector);
		    currProfileSelector.selectProfileIDs(selectedIDs, nullptr);

		    //TODO:eventually, we should change this.
		    //assert(selectedIDs.size() == 1);	//we don't want any wild cards in the DemographicProfile string.

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
	catch(const std::exception &/*e*/)
	{
		continue;
	}
    }
#endif
}

Population::~Population()
{
}

void Population::Births(EventParams &parameters_)
{
    unsigned long numBorn = 0;
    if (GetParameters().GetUseBirthRate())
    {
	    // get numBorn from birth rate
	    numBorn = Utility::round<unsigned long>(currSize * popWideParams.birthRate);
    }
    else
    {
	    // get numBorn from fertility rates
	    // numBorn = sum((rate*females) / 1000
	    double sumRates = 0.0;
	    for ( auto rate : popWideParams.GetFertilityRates()) {
		    int numFemales = entities->sizeByAgeFemales(rate.Lower(), rate.Upper());
		    sumRates += rate.Rate() * numFemales;
	    }
	    numBorn = Utility::round<unsigned long>(sumRates / 1000);
    }

	for (auto profileDoublePair : popWideParams.GetBirthProportions())
	{
		DemographicProfile profile = profileDoublePair.first;
		double value = profileDoublePair.second;
		unsigned long numToCreate = Utility::round<unsigned long>(numBorn * value);
		GenerateEntities(profile, numToCreate, nullptr);
	}
}

//Updates the age buckets for use with life expectancy
void Population::UpdateAgeBucketsLE()
{
	//Used to iterate through persons
	std::list<Entity *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
	for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Entity *p = (*p_Iter);
			assert(p != nullptr);
			//calculate life expectancy
			assert((populationStatistics.selectedLEStats != nullptr));
            assert(p->getAge() >= Age::from_months(0));
            assert(p->getAge() < Age(Entity::maxYrForDeathStats, 0));
            populationStatistics.selectedLEStats->popByAge[(long)p->getAge().in_months()]++;
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

	//holds a pointer to the current bucket we are looking at
	BucketDemographicProfile *currBucket = nullptr;
	//helps us iterate through all BucketDemographicProfiles
	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;
	//Used to iterate through persons
	std::list<Entity *>::iterator p_Iter;

	double cepacDiscountFactor = 1.0;
	if (parameters_.useRollout) {
		cepacDiscountFactor = Utility::computeCepacDiscountFactor(parameters_.currTime.in_months(),
		    parameters_.untreatedContext->getRunSpecsInputs()->discountFactor);
	}
	else {
		cepacDiscountFactor = Utility::computeCepacDiscountFactor(parameters_.currTime.in_months(),
		    parameters_.cepacSimContexts.front()->getRunSpecsInputs()->discountFactor);
	}

	//Reset the class to calculate LE
	if(newLEPeriod)
	{
		populationStatistics.selectedLEStats = new PopulationStatisticsOld::SingleLEStats;
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

		currBucket->ageOneTimeStep();
		currProfileID++;
	}

	//Double loop: first iterate through the men, then the women
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Entity *p = (*p_Iter);
			assert(p != nullptr);

			Entity::HIVStatus oldStatus = p->hivStatus;
			//update their health status
			p->updateHealthStatus(parameters_, &populationStatistics.artTracker, &populationStatistics.costsTracker);

			if (p->UsingPrEP()) {
			    populationStatistics.costsTracker.RecordPrEPCost(popWideParams.prEPCost,
				popWideParams.prEPCost * cepacDiscountFactor);
			    p->add_cdm_cost(popWideParams.prEPCost, popWideParams.prEPCost * cepacDiscountFactor);
			}

			if(oldStatus != p->hivStatus)
			{
                if(p->getDemographicProfile()->get(p->getDemographicProfile()->getProfileID(), DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
				{
					((BucketSexualMixing *) entities->getBucket(p->getDemographicProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
					        p->hivStatus);
				}
			}

			if(parameters_.useRollout && parameters_.treatedContext && p->isInfected())
			{
				if(p->isOnArt())
				{
					populationStatistics.recordTreatment(p);
				}
			}

			//see whether this person has died.
			//if this person was a couple, then will push living members to personsToAdd
			// to be reinserted into the EntityPool once we have iterated through all buckets
			if(p->rollForDeath(parameters_.randomNums))
			{
				if(parameters_.useRollout && parameters_.treatedContext &&
				    p->isInfected() && p->isOnArt()) {
				    populationStatistics.recordTreatmentDeath(p);
				}

				p_Iter = entities->removeEntityFromAll(p_Iter);
				ProcessDeath(parameters_, p, calculateLE);

				if(parameters_.useRollout)
				{
					//Remove people from the treated/untreated pool if they die
					std::list<Entity *>::iterator poolIterator;
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

			if(parameters_.useRollout && parameters_.treatedContext && p->isInfected())
			{
			    if((p->isOnArt()) ||
			     (p->isEligibleForTreatment(parameters_.treatedContext->getTreatmentInputs()->startART[0])))
			    {
			        populationStatistics.recordTreatmentEligiblity(p);
			    }
			}


			//if this person wasn't sexually active but is now old enough to
            if((p->getDemographicProfileVal(DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::Active)
                && (p->getAge() >= popWideParams.ageOfMajority))
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

				Entity::HIVStatus oldStatus = p->hivStatus;
				p->becomeSexuallyActive(parameters_);

				if(oldStatus != p->hivStatus)
				    {
					auto sa_status = p->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>();

					if(sa_status != DemographicProfile::SexualActivityStatus::NotActive)
					    {
						auto bucket = entities->getBucket(p->getDemographicProfile()->getProfileID());
						// This shouldn't fail because all sexually activity people are in sexual mixing buckets
						auto sexual_mixing_bucket = static_cast<BucketSexualMixing *>(bucket); 
						sexual_mixing_bucket->changeHIVStatus(p, oldStatus, p->hivStatus);
					    }
				    }

				if (!p->PassedCSWEndAge())
				{
					p->rollForBecomeSexWorker(parameters_);
				}

				Entity::RiskLevel oldRisk = p->getRiskLevel();
				//reroll risk group
				p->rerollRiskGroup(parameters_);

				if(oldRisk != p->getRiskLevel())
				    {
					OnRiskGroupChanged(p);
				    }

				//refresh risk group in dmg bucket and refresh BucketDemographicProfile
				entities->refreshBucketDemographicProfile(p, &p_Iter, oldRisk != p->getRiskLevel());
			}

	    //Check for age to stop becoming CSW
            if(p->isCSW() && p->PassedCSWEndAge()) {
		p->quitSexWork(parameters_);
		entities->refreshBucketDemographicProfile(p, &p_Iter);
	    }

	    if(((parameters_.useRollout && parameters_.treatedContext) || p->HasTargetedCepacContext()) &&
	       p->isInfected())
	    {
                 auto context = p->HasTargetedCepacContext() ? p->GetTargetedCepacContext() : parameters_.treatedContext;

		 if(p->isOnArt())
		 {
		     // if they're on treatment, they should be counted as eligible even if the treatment has worked  
		     populationStatistics.recordTreatmentEligiblity(p);
		     populationStatistics.recordTreatment(p);
		 }
		 else if(p->isEligibleForTreatment(context->getTreatmentInputs()->startART[0]))
		 {
		     populationStatistics.recordTreatmentEligiblity(p);
		 }
	    }

	    populationStatistics.costsTracker.RecordLifeMonth(p->getQualityOfLife(),
		cepacDiscountFactor, p->getHIVStatus());

	    p_Iter++;
		}
	}
}

std::vector<Entity *> Population::Find(std::function<bool(Entity *)> predicate)
{
    std::vector<Entity *> matches;
    entities->forEach([=, &matches](Entity *p) { if(predicate(p)) matches.push_back(p); });
    return matches;
}

std::vector<Entity *> Population::FindNonCircumcised()
{
    auto match = [&](Entity *person) {
        return person->isMale() && !person->IsCircumcised();
    };

    std::vector<Entity *> matches;
    entities->forEach([=, &matches](Entity *p) {
	    if(match(p))
		matches.push_back(p);
	});
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
	int newPartnershipCount[(std::size_t)SexualPartnership::Type::Last];
	//Number of attemptedPartnerships may be higher than the actual partnerships formed if there weren't enough females/males tried to repartner with current partners
	int attemptedPartnershipCount[(std::size_t)SexualPartnership::Type::Last];
	int	endedPartnershipCount[(std::size_t)SexualPartnership::Type::Last];

	//initialize counters
	for(int type = 0; type < (int)SexualPartnership::Type::Last; ++type)
	{
		newPartnershipCount[type] = 0;
		attemptedPartnershipCount[type] = 0;
		endedPartnershipCount[type] = 0;
	}

	double cepacDiscountFactor = 1.0;
	if (parameters_.useRollout) {
		cepacDiscountFactor = Utility::computeCepacDiscountFactor(parameters_.currTime.in_months(),
		    parameters_.untreatedContext->getRunSpecsInputs()->discountFactor);
	}
	else {
		cepacDiscountFactor = Utility::computeCepacDiscountFactor(parameters_.currTime.in_months(),
		    parameters_.cepacSimContexts.front()->getRunSpecsInputs()->discountFactor);
	}

	//iterate through all males
	std::list<Entity *>::iterator p_Iter;

	//Iterate twice...
	//First pass: Dissolve ended partnerships
	for(p_Iter = entities->begin(DemographicProfile::Gender::Male); p_Iter != entities->end(DemographicProfile::Gender::Male); p_Iter++)
	    {
		auto person = *p_Iter;
		person->resetNumActs();
		person->reset_costs();
		std::list<SexualPartnership *> partnershipsToEnd;

		//Decide who needs to split up
		for(int type = 0; type < (int)SexualPartnership::Type::Last; ++type)
		{
		    endedPartnershipCount[type] += ((Male *)person)->getPartnershipsToEnd(parameters_.currTime, SexualPartnership::Type(type), partnershipsToEnd, false);
		}

		//Now, split them up... man, it would suck for their kids (if they had any)
		DissolveSexualPartnerships(parameters_, person, partnershipsToEnd);

		//if this initiator is now single, then make sure they are in singles pool
		if(!person->inCorrectBucketDemographicProfile())
		    {
			entities->refreshBucketDemographicProfile(person, &p_Iter);
		    }

		((Male *)person)->ResetTimesSelected();
	    }

	//Ending the first pass (dissolving partnerships)

	//reset num acts for females
	for(p_Iter = entities->begin(DemographicProfile::Gender::Female); p_Iter != entities->end(DemographicProfile::Gender::Female); p_Iter++)
	{
		(*p_Iter)->resetNumActs();
		((Female *)(*p_Iter))->ResetTimesSelected();
		(*p_Iter)->reset_costs();
		((Female *)*p_Iter)->ResetVaginalMicrobicideUsage();
	}

	//Second pass: Form new partnerships and have sex
	for(p_Iter = entities->begin(DemographicProfile::Gender::Male); p_Iter != entities->end(DemographicProfile::Gender::Male); p_Iter++)
	{
	    Male *person = (Male *)*p_Iter;

		if(!person->isAlive())
		{
			continue;
		}

		//Reset the initiator's condom count
		person->resetCondomUsage();
		//(The non-initiators (i.e. women) will never have their condom count reset... I don't think we care?)
		assert(person != nullptr);

		if(person->getAge() < popWideParams.ageOfMajority + person->GetSexualActivityDelay())
        {
			continue;
        }

		//Get available partnership types
		std::vector<SexualPartnership::Type> partnershipTypes =
		    profilesToPartnershipTypes[person->getCurrBucketProfileID()];

		//iterate through the SexualPartnership::Type that people in the current bucket engage in
		// form new partnerships
		for(std::size_t i = 0; i < partnershipTypes.size(); i++)
		{
			SexualPartnership::Type type = partnershipTypes.at(i);

            // skip MSM CSW partnerships for now
            if ((person->getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
                DemographicProfile::SexualOrientation::Msm) &&
                type == SexualPartnership::Type::Csw) continue;

			//this method distinguishes between partnerships with and without duration and
			//  executes different code depending on which. If the partnership has no duration
			//  associated with it, then the sexual act is done during this method
			//First, reset the tally of latest unformed partnerships (unformed but intended to form)
			person->resetLatestUnformedPartnerships(type);
			//TODO: Get the ratio of numFormed to numIntendedToForm
			int numFormed = CreatePartnerships(parameters_, person, &p_Iter, type);
			newPartnershipCount[(std::size_t)type] += numFormed;
			attemptedPartnershipCount[(std::size_t)type] += numFormed + person->getLatestUnformedPartnerships(type);
		}

		//for existing partnerships, have sexual activity
		//Have all the sexual activity with current partners (includes new partners)
		for(int type = 0; type < (int)SexualPartnership::Type::Last; ++type)
		{
			std::list<Entity *> newlyInfected;
			//sexual activity among any existing partnerships that have a duration associated with them
			Entity *infectedMe = person->allPartnerSexualActivity(parameters_, SexualPartnership::Type(type), newlyInfected,
                &populationStatistics.infectionsTracker, popWideParams.transmission_coefficients_);
			//TODO: Get a condom use count here!
			//record all incident infections
			std::list<Entity *>::iterator newlyInfectedIter = newlyInfected.begin();

			while(newlyInfectedIter != newlyInfected.end())
			{
				Entity *wasUninfected = *newlyInfectedIter;

				if(wasUninfected->getDemographicProfile()->get(wasUninfected->getDemographicProfile()->getProfileID(),
                    DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
				{
					((BucketSexualMixing *) entities->getBucket(wasUninfected->getDemographicProfile()->getProfileID()))->changeHIVStatus(
					    wasUninfected, Entity::HIVStatus::NEGATIVE, wasUninfected->hivStatus);
				}

				((BucketSexualMixing *) entities->getBucket(wasUninfected->getDemographicProfile()->getProfileID()))->increaseInfected(
				    wasUninfected);
				//initiator only gets infected once...
				Entity *wasInfected = (*newlyInfectedIter == person) ? infectedMe : person;

				RecordInfection(wasUninfected, wasInfected, parameters_.currTime);

				//Adds person to the untreated pool if using rollout
				if(parameters_.useRollout)
				{
					rolloutUntreatedPool.push_back(wasUninfected);
				}

				populationStatistics.recordIncidentInfection(parameters_, parameters_.currTime,
				    SexualPartnership::Type(type), wasInfected, wasUninfected);

				newlyInfectedIter++;
			}
		}

		auto totalCondomCostUndiscounted = person->getCondomsUsedThisMonth() *
		    popWideParams.condomCost;
		populationStatistics.costsTracker.RecordCondomUse(totalCondomCostUndiscounted,
			totalCondomCostUndiscounted * cepacDiscountFactor);
		person->add_cdm_cost(totalCondomCostUndiscounted,
			totalCondomCostUndiscounted * cepacDiscountFactor);
	}

	for (p_Iter = entities->begin(DemographicProfile::Gender::Female);
	     p_Iter != entities->end(DemographicProfile::Gender::Female); p_Iter++)
	{
	    auto p = static_cast<Female *>(*p_Iter);
	    if (p->GetVaginalMicrobicideApplicationsThisMonth() > 0)
	    {
		auto cost = popWideParams.vaginalMicrobicideApplicationCost * p->GetVaginalMicrobicideApplicationsThisMonth();
		populationStatistics.costsTracker.RecordVaginalMicrobicideCost(cost, cost *
		    cepacDiscountFactor);
	    }
	}


	//Ends the second pass through (i.e. the sex acts pass through)

	if (parameters_.calibrationInputs.useCalibration
	        && parameters_.currTime > (parameters_.calibrationInputs.monthOfCalibration - TimeSpan::Year)
	        && parameters_.currTime <= parameters_.calibrationInputs.monthOfCalibration)
	{
		//update concurrency status
        for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::list<Entity *>::iterator p_Iter = entities->begin((DemographicProfile::Gender) gender);

			while(p_Iter != entities->end((DemographicProfile::Gender) gender))
			{
				//tally concurrent partners
				//create a number between 0 and 15 representing the combination of partnership types person has
				//e.g. if person has partnerships steady and casual concurrent will equal 8+2=10
				//Whoever wrote this deserves a special place in C programmer's hell. GA
				int concurrent = 0;
				int numPartners[(std::size_t)SexualPartnership::Type::Last];
				int totalNumPartners = 0;

				for(int i = 0; i < (int)SexualPartnership::Type::Last; i++)
				{
					numPartners[i] = (*p_Iter)->getNumPartners((SexualPartnership::Type) i);
					concurrent = (concurrent << 1) + (numPartners[i] != 0 ? 1 : 0);
					totalNumPartners += numPartners[i];
				}

				concurrent = 15 - concurrent;
				assert(concurrent <= Constants::NumberConcurrencyDefs);

				if(parameters_.concurrencyDef[concurrent].useDefinition
				        && totalNumPartners >= parameters_.concurrencyDef[concurrent].minPartnershipsNeeded)
				{
					//count as concurrent partnership
					(*p_Iter)->setTimeOfLatestConcurrent(parameters_.currTime);
				}

				p_Iter++;
			}
		}
	}
}

void Population::SaveIndividualSummaries(std::ostream &stream) const
{
    std::vector<EntitySummary> ordered_(individual_summaries_.size());

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
        stream << (summary.profile.get(DemographicProfile::Demographic::SexualActivityStatus) == 0 ? "\"sexually-active\"" : "\"not-active\"");
        stream << ",\"sexual_orientation\"" << ":";
		switch(summary.profile.get(DemographicProfile::Demographic::SexualOrientation))
		{
		    case (std::size_t)DemographicProfile::SexualOrientation::Msw: stream << "\"msw\"" << ":"; break;
			case (std::size_t)DemographicProfile::SexualOrientation::Msmw: stream << "\"msmw\"" << ":"; break;
			case (std::size_t)DemographicProfile::SexualOrientation::Msm: stream << "\"msm\"" << ":"; break;
				default: throw std::runtime_error("unknown sexual orientation");
		}
        stream << ",\"relationship_status\"" << ":";
        stream << (summary.profile.get(DemographicProfile::Demographic::RelationshipStatus) == 0 ? "\"non-single\"" : "\"single\"");
        stream << ",\"risk_group\"" << ":" << (summary.risk_group == Entity::RiskLevel::HIGH ? "\"high\"" : "\"low\"");
        stream << ",\"age_at_infection\"" << ":" << summary.age_at_infection.in_months();
        stream << ",\"generation_number\"" << ":" << summary.generation_number;
        stream << ",\"infection_number\"" << ":" << summary.infection_number;
        stream << ",\"time_infected\"" << ":" << summary.time_infected.in_months();
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

std::vector<AgeRange> Population::GetAgeRanges() const
{
	return popWideParams.GetAgeRanges();
}

/**
Iterates through current entities in the population and returns a total number of people
***/
std::size_t Population::UpdateSize()
{
    currSize = entities->size();
    currNASize = entities->sizeNotSexuallyActive();

    //Size by gender
    DemographicProfile GenderProfile;
    std::vector<DemographicProfile::ProfileID> GenderProfileIDs;

    //First tally the men
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
    currSizeGender[(std::size_t)DemographicProfile::Gender::Male] = 0;
    GenderProfileIDs.clear();
    GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);
    for(std::size_t i = 0; i < GenderProfileIDs.size(); i++)
    {
        currSizeGender[(std::size_t)DemographicProfile::Gender::Male] +=
		  entities->size(GenderProfileIDs[i]);
    }

    //Next tally the women
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
    GenderProfileIDs.clear();
    currSizeGender[(std::size_t)DemographicProfile::Gender::Female] = 0;
    GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);
    for(size_t i = 0; i < GenderProfileIDs.size(); i++)
    {
        currSizeGender[(std::size_t)DemographicProfile::Gender::Female] +=
	    entities->size(GenderProfileIDs[i]);
    }

    //Count all the CSW's
    DemographicProfile CSWProfile;
    CSWProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::Csw);
    std::vector<DemographicProfile::ProfileID> CSWProfileIDs;
    currCSWSize = 0;
    CSWProfile.selectProfileIDs(CSWProfileIDs, nullptr);
    for(std::size_t i = 0; i < CSWProfileIDs.size(); i++)
    {
		currCSWSize += entities->size(CSWProfileIDs[i]);
    }

    currNASizeByEntityType.clear();
    currSASizeEntityTypeRisk.clear();

    //Update size by risk level
    for(int risk = 0; risk < (int)Entity::RiskLevel::Last; risk++)
    {
		currSizeRisk[risk] = 0;
		currSizeRiskCSW[risk] = 0;

		for(auto profileID : demographicProfileIDs)
		{
			currSizeEntityTypeRiskCSW[profileID][risk] = 0;
			currSASizeEntityTypeRisk[profileID][risk] = 0;

            if(risk == 0)
            {
                currSizeEntityType[profileID] = 0;
				currNASizeByEntityType[profileID] = 0;
			}
		}
    }

    for(auto profileID : demographicProfileIDs)
    {
		for (auto &age_range_size_pair : currSizeByEntityTypeAgeRange[profileID])
		{
			age_range_size_pair.second = 0;
		}
    }

    num_circumcised_na = 0;
    num_circumcised_sa = 0;

    entities->countEntitiesPerAge();

    entities->forEach([&](Entity *e)
    {
		auto profile = e->getDemographicProfile()->getProfileID();
		auto risk = (std::size_t)e->getRiskLevel();
        currSizeEntityType[profile]++;
        currSizeRisk[(std::size_t)e->getRiskLevel()]++;

        if(e->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() ==
			DemographicProfile::SexualActivityStatus::Active)
        {
            currSASizeEntityTypeRisk[profile][risk]++;
        }
        else
        {
            currNASizeByEntityType[profile]++;
        }

        if(e->getDemographicProfileVal<DemographicProfile::Employment>() ==
			DemographicProfile::Employment::Csw)
        {
            currSizeRiskCSW[risk]++;
            currSizeEntityTypeRiskCSW[profile][risk]++;
        }

		for (auto &age_range_size_pair : currSizeByEntityTypeAgeRange[profile])
		{
			if (age_range_size_pair.first.lower <= e->age && age_range_size_pair.first.upper >= e->age)
			{
				age_range_size_pair.second++;
				break;
			}
		}

        if(e->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Male)
        {
            if(((Male *)e)->IsCircumcised())
            {
                if(e->getDemographicProfileVal<DemographicProfile::SexualActivityStatus>() ==
					DemographicProfile::SexualActivityStatus::Active)
                {
                    num_circumcised_sa++;
                }
                else
                {
                    num_circumcised_na++;
                }
            }
        }
    });

    return currSize;
}

//Resets Monthly Population Statistics
void Population::ResetMonthlyStats()
{
	//reset curr month death stats
	for(int i = 0; i < (int)Entity::DeathStatus::Last; i++)
	{
		currDeathCauses[i] = 0;
	}
}

//After all of the population dynamics have run through, run all of the remaining infected persons through CEPAC until they die to get the life expectancy and such in the CEPAC output files.
void Population::UpdateFinalPhysicalState(EventParams &parameters_)
{
	//Used to iterate through persons
	std::list<Entity *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Entity *p = (*p_Iter);
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

void Population::DissolveSexualPartnerships(EventParams &parameters_, Entity *_initiator,
        std::list<SexualPartnership *> &_partnershipsToEnd)
{
    bool initiatorMale = (_initiator->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male);
	//iterate through each partner list
	std::list<SexualPartnership *>::iterator partnerIter = _partnershipsToEnd.begin();

	while(partnerIter != _partnershipsToEnd.end())
	{
		Entity *partner = (*partnerIter)->getOtherPartner(_initiator);

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
		}

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

/*
 * The params.xml file should have detailed the  characteristics of each age bucket.
 * We will go through each age bucket and create the part of the prevalent population that
 * falls within the bucket.
 */
void Population::GenerateInitialEntities()
{
    for(auto &ageBucketParams : popWideParams.GetInitialAgeBuckets())
    {
        AgeRange ageRange = {ageBucketParams.GetMinAge(), ageBucketParams.GetMaxAge()};
		for (auto profileDoublePair : ageBucketParams.GetEntityProportions())
		{
            auto gender = profileDoublePair.first.get(DemographicProfile::Demographic::Gender);
            double ageRangeValue = profileDoublePair.second;

            for (auto birthProfile : popWideParams.GetBirthProportions())
            {
                if (gender == birthProfile.first.get(DemographicProfile::Demographic::Gender))
                {
                    unsigned long numToCreate = Utility::round<unsigned long>(
                      popWideParams.GetInitialSize() * ageRangeValue * birthProfile.second);
                    GenerateEntities(birthProfile.first, numToCreate, &ageRange);
                }
            }
		}
    }
}

void Population::GenerateEntities(const DemographicProfile &profile,
	unsigned long numInProfileToCreate, AgeRange *ageRange)
{
	for (unsigned long count = 0; count < numInProfileToCreate; count++)
	{
			bool toTrace = parameters_.currTime >= parameters_.monthTraceNewborns
			  && parameters_.numNewbornsTraced < parameters_.numNewbornsToTrace;
			if(toTrace) parameters_.numNewbornsTraced++;

			auto age = Age::from_months(0);
			if (ageRange) {
				// Generate an age from a uniform distribution bounded by _ageBucketParams
				auto randAge = parameters_.randomNums.randInt(ageRange->lower.in_months(),
					ageRange->upper.in_months());
				age = Age::from_months(randAge);
			}
			auto p = GenerateEntity(parameters_, profile, age, toTrace);

			// add the newborn to the EntityPool
			// use addEntityToAll here (initial entrance into population)
			entities->addEntityToAll(p);
	}
}

Entity *Population::GenerateEntity(EventParams &parameters_, const DemographicProfile &profile,
    Age age, bool toTrace)
{
	// pointer to the person that will be generated
    Entity *toReturn;

    // create the person with this gender and age
    auto gender = profile.get(DemographicProfile::Demographic::Gender);
    if(gender == (std::size_t)DemographicProfile::Gender::Male) {
		auto circumcised = parameters_.randomNums.chance(popWideParams.GetProportionCircumcised());

		toReturn = new Male(parameters_, age, circumcised, profile, populationID, popWideParams.defaultMaleParams);

		if (toReturn->IsCircumcised()) {
			auto discount = parameters_.useRollout ?
			  parameters_.untreatedContext->getRunSpecsInputs()->discountFactor
			  : parameters_.cepacSimContexts.front()->getRunSpecsInputs()->discountFactor;
			populationStatistics.costsTracker.RecordCircumcision(popWideParams.circumcisionCost,
				popWideParams.circumcisionCost * discount);
			toReturn->add_cdm_cost(popWideParams.circumcisionCost,
				popWideParams.circumcisionCost * discount);
		}

    } else {
		toReturn = new Female(parameters_, age, profile, populationID, popWideParams.defaultFemaleParams);
    }

	toReturn->SetSexualActivityDelay(popWideParams.sexualActivityDelay);

	//Set this person to be trace if toTrace is true
	if(toTrace)
	{
		toReturn->setToBeTraced();
	}

	//if person is of sexually active age, roll and see if they are a CSW
	//DO NOT SET THEM AS SEXUALLY ACTIVE UNTIL AFTER DETERMINING IF THEY ARE A PREVALENT CASE BECAUSE THE CEPAC PERSON IS CREATED HERE!
	if(age >= popWideParams.ageOfMajority)
	{
	    //see if they will be a CSW
	    auto profileID = profile.getProfileID();
	    if(!toReturn->PassedCSWEndAge())
	    {
			toReturn->rollForBecomeSexWorker(parameters_);
	    }

	    Entity::RiskLevel oldRisk = toReturn->getRiskLevel();
	    //reroll their risk group
	    toReturn->rerollRiskGroup(parameters_);

	    if(oldRisk != toReturn->getRiskLevel())
	    {
			OnRiskGroupChanged(toReturn);
	    }

	    //If they are of age, set them to be sexually active here: this is where toReturn->cepacPerson is initialized for non-prevalent cases
	    Entity::HIVStatus oldStatus = toReturn->hivStatus;
	    toReturn->becomeSexuallyActive(parameters_);

	    if(oldStatus != toReturn->hivStatus)
 	    {
			if(toReturn->getDemographicProfile()->get(toReturn->getDemographicProfile()->getProfileID(),
				DemographicProfile::Demographic::SexualActivityStatus) !=
				(std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
			{
				((BucketSexualMixing *) entities->getBucket(
				  toReturn->getDemographicProfile()->getProfileID()))->
				  changeHIVStatus(toReturn,oldStatus, toReturn->hivStatus);
			}
	    }
	}

	return toReturn;
}

void Population::OnRiskGroupChanged(const Entity *entity)
{
    std::for_each(risk_group_changed_.begin(), risk_group_changed_.end(),
        [=](ChangedRiskGroupEventHandler &h) { h(entity); });
}

/*
* Resets the counter for incidient infections by age for infectionstracker
*/
void Population::InitIncidentInfectionsByAge()
{
    InfectionsTracker::EntityTypeArray<AgeRangeSizeContainer> incidentByGenderAndAge;
    AgeRangeSizeContainer totalIncidentInfsAge;

    for(auto ageBucketParams : popWideParams.GetInitialAgeBuckets())
    {
		AgeRange ageRange = {ageBucketParams.GetMinAge(), ageBucketParams.GetMaxAge()};
        totalIncidentInfsAge.push_back({ageRange, 0});
        for(auto gender : enum_iterator<DemographicProfile::Gender>()) {
            incidentByGenderAndAge[(std::size_t)gender].push_back({ageRange, 0});
        }
    }

    populationStatistics.infectionsTracker.initializeIncidentInfectionsByAge(incidentByGenderAndAge,
		totalIncidentInfsAge);
}

/*
 * ApplyIncidentPrevalence
 *
 * Seeds the population with infected individuals
 *
 * Seeding the population happens in one of two ways:
 * by count or by coefficient depending on the input
 * If by count, then the values stored in the SeedDistribution
 * vector are used as whole person counts. In this case,
 * all other seed parameters are ignored.
 *
 * If by coefficient, the the values stored in the SeedDistribution
 * vector are used to calculate the proportion of individuals infected
 * in each risk group and age bucket.
 */
void Population::ApplyIncidentPrevalence(EventParams &parameters_)
{
	if (popWideParams.UseSeedCoefficients()) {
		prevalentInfectionsFromCoefficients();

	} else {
		prevalentInfectionsFromCount();
	}
}

/*
 * prevalentInfectionsFromCoefficients()
 *
 * Creates a vector containing a list of people for seeding and randomly
 * selects ones to infect.
 */
void Population::prevalentInfectionsFromCoefficients()
{
	// vector of people to consider for seeding
	// contains number of people per starta multiplied by the coefficient for that strata
	std::vector<Entity *> seedList;
	std::size_t popInAgeRange = 0;

	for (auto &target : popWideParams.initial_infection_targets_)
	{
	    if (target.second <= 0) continue;

	    auto gender = target.first.get_gender();
	    if (gender == DemographicProfile::Gender::Last)
	    {
		throw std::runtime_error("No gender target for seeding");
	    }

	    for(std::list<Entity *>::iterator _iter = entities->begin(gender);
		_iter != entities->end(gender); _iter++)
	    {
		Entity *person = *(_iter);
		if (target.first.match(person))
		{
		    for (int coeff = 0; coeff < target.second; coeff++)
		    {
			seedList.push_back(person);
		    }
		    popInAgeRange++;
		}
	    }
	}

	if (popInAgeRange == 0 || seedList.size() == 0) {
		throw std::runtime_error("No people in demographics to seed. "
		    "Check input parameters.");
	}

	double seedPrevalence = popWideParams.GetSeedPrevalence();
	auto popToSeed = Utility::round<unsigned long>(popInAgeRange * seedPrevalence);

	infectSeedPopulation(seedList, popToSeed);
}

/*
 * infectSeedPopulation()
 *
 * Randomly selects a person from the list to infect.
 */
int Population::infectSeedPopulation(std::vector<Entity *> seedList, std::size_t seedPopulation)
{
    int count = 0;
    while (count < seedPopulation) {
	int index = (int)parameters_.randomNums.randInt(0,
	    (uint32_t)seedList.size() - 1);
	Entity *p = seedList.at(index);

	if (p->isInfected())
	    continue;

	applyPrevalentInfection(p);
	count++;
    }

    return count;
}


/*
 * prevalentInfectionsFromCount()
 */
void Population::prevalentInfectionsFromCount()
{
    for (auto &target : popWideParams.initial_infection_targets_)
    {
        if (target.second <= 0) continue;
    	int coeff = target.second;

	auto gender = target.first.get_gender();
	if (gender == DemographicProfile::Gender::Last)
	{
	    throw std::runtime_error("No gender target for seeding");
	}

	for(std::list<Entity *>::iterator _iter = entities->begin(gender);
	    _iter != entities->end(gender); _iter++)
	{
	    Entity *person = *(_iter);
	    bool isPrevalent = false;

	    if (target.first.match(person))
	    {
		coeff--;
		isPrevalent = true;
	    }

	    if (isPrevalent)
	    {
	        applyPrevalentInfection(person);
		if (coeff == 0)
		  break;
	    }
	}
    }
}

/*
 * applyPrevalentInfection()
 *
 * Forces a prevalent infection on a @p
 */
void Population::applyPrevalentInfection(Entity *p)
{
    Entity::HIVStatus oldStatus = p->hivStatus;

    if(parameters_.tracePrevalentCases) {
	p->setToBeTraced();
    }

    bool chronicInfection = rollForChronicInfection(parameters_.randomNums);
    p->seedInfection(Constants::InitialInfection, parameters_, chronicInfection);
    RecordInfection(p, nullptr, parameters_.currTime);

    if(oldStatus != p->hivStatus) {
	((BucketSexualMixing *) entities->getBucket(
	    p->getDemographicProfile()->getProfileID()))->
	    changeHIVStatus(p, oldStatus, p->hivStatus);
    }

    if(parameters_.useRollout) {
	rolloutUntreatedPool.push_back(p);
    }

    if (p->getDemographicProfile()->get(
	    p->getDemographicProfile()->getProfileID(),
	    DemographicProfile::Demographic::SexualActivityStatus) !=
	(std::size_t)DemographicProfile::SexualActivityStatus::NotActive) {
	((BucketSexualMixing *)entities->getBucket(
	    p->getDemographicProfile()->
	    getProfileID()))->increaseInfected(p);
    }
}

bool Population::rollForChronicInfection(RandomNumberGenerator &_randomNums)
{
    double infectionRate = popWideParams.GetChanceChronicInfection();
    return (_randomNums.chance(infectionRate));
}

void Population::RecordInfection(const Entity *infectee, const Entity *infector, Time time)
{
    if(individual_summaries_.size() >= NumIndividualSummaries)
    {
        return;
    }

    EntitySummary summary;

    summary.person_id = (int)infectee->getID();
    summary.generation_number = infectee->getGenerationOfInfection(false);
    summary.infected_by = infector == nullptr ? -1 : infector->getID();
    summary.infection_number = (int)individual_summaries_.size();
    summary.profile = *infectee->getDemographicProfile();
    summary.time_infected = time;
    summary.time_of_death = Time::from_months(-1);
    summary.age_at_infection = infectee->getAge();
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
void Population::ApplyRolloutContext(EventParams &parameters_, Time time)
{
	for(auto rolloutContext : parameters_.rolloutSimContexts)
	{
		if(rolloutContext->timeToApply == time)
		{
			//Switch the cepac file depending on the population the new file is applied to
			switch(rolloutContext->popOfInterest)
			{
			case 0: // Update all untreated people with the new context
			{
				parameters_.untreatedContext = rolloutContext->rolloutSimContext.get();

				//Apply to all current untreated patients
				std::list<Entity *>::iterator personIter;
				for(personIter = rolloutUntreatedPool.begin(); personIter != rolloutUntreatedPool.end();
				    personIter++)
				{
					(*personIter)->setSimContext(parameters_.untreatedContext);
				}

				break;
			}
			case 1:  // Update all treated people with the new context
			{
				parameters_.treatedContext = rolloutContext->rolloutSimContext.get();

				//Apply to all current treated patients
				std::list<Entity *>::iterator personIter;
				for(personIter = rolloutTreatedPool.begin(); personIter != rolloutTreatedPool.end(); personIter++)
				{
					(*personIter)->setSimContext(parameters_.treatedContext);
				}

				break;
			}
			case 2: // Only new people added to the treated pool will context this context
				parameters_.treatedContext = rolloutContext->rolloutSimContext.get();
				break;
			case 3: // Only newly infected people (added to the untreated pool) will context this context
				parameters_.untreatedContext = rolloutContext->rolloutSimContext.get();
				break;
			default:
				break;
			}
		}
	}
}

void Population::DetermineRankings(const RolloutEligibility &criteria)
{
	std::unordered_set<Entity *> rankedPeople;

	//loop through the eligibility rankings
	for(int currentRank = 1; currentRank <= 5; ++currentRank)
	{
		bool checkOiHist = currentRank == criteria.oiHistRank;
		bool checkCd4 = currentRank == criteria.cd4Rank;
		bool checkCd4OiHist = currentRank == criteria.cd4OiHistRank;
		bool checkHvl = currentRank == criteria.hvlRank;
		bool checkCd4Hvl = currentRank == criteria.cd4HvlRank;
		rankedForTreatment[currentRank - 1].clear();

		bool checkHIVIdentified = criteria.isIdentified;

		if(checkOiHist || checkCd4 || checkCd4OiHist || checkHvl || checkCd4Hvl)
		{
			auto untIter = rolloutUntreatedPool.begin();

			//loop through people in the untreated pool to check for their eligibility
			while(untIter != rolloutUntreatedPool.end())
			{
				bool isEligible = false;
				Entity *untPerson = *untIter;

				if(rankedPeople.find(untPerson) != rankedPeople.end() || untPerson->HasTargetedCepacContext())
				{
					untIter++;
					continue;
				}

				if(checkOiHist)
				{
					int numMatchingOIs = 0;

					for(int oiNum = 0; oiNum < Constants::NumberOfOIs; oiNum++)
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

					for(int oiNum = 0; oiNum < Constants::NumberOfOIs; oiNum++)
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
					Entity::HVLStrata currHvl = untPerson->currentTrueHvl;

					if((int)currHvl >= criteria.hvlBounds.lower && (int)currHvl <= criteria.hvlBounds.upper)
					{
						isEligible = true;
					}
				}

				if(checkCd4Hvl)
				{
					Entity::HVLStrata currHvl = untPerson->currentTrueHvl;
					double currCd4 = untPerson->cd4;

					if(currCd4 >= criteria.cd4HvlCd4Bounds.lower && currCd4 <= criteria.cd4HvlCd4Bounds.upper
					        && (int)currHvl >= (int)criteria.cd4HvlHvlBounds.lower && (int)currHvl <= (int)criteria.cd4HvlHvlBounds.upper)
					{
						isEligible = true;
					}
				}

				if(isEligible)
				{
					if (!checkHIVIdentified || untPerson->isIdentified())
					{
						rankedForTreatment[currentRank - 1].push_back(untPerson);
						rankedPeople.insert(untPerson);
					}
				}

				untIter++;
			}
		}
	}
}

void Population::StartTreatment(Entity *person, SimContext *treatedContext)
{
	std::list<Entity *>::iterator untreatedIterator;
	untreatedIterator = std::find(rolloutUntreatedPool.begin(), rolloutUntreatedPool.end(), person);
	assert(untreatedIterator != rolloutUntreatedPool.end());
	rolloutUntreatedPool.erase(untreatedIterator);
	rolloutTreatedPool.push_back(person);
	person->setSimContext(treatedContext);
}

double InterpolateProportion(const std::map<int, double> &yearly_proportions, Time month, Time monthOf1990)
{
	if(month >= monthOf1990)
	{
		auto relative_time = Time(1990, 0) + (month - monthOf1990);
		int relative_year = relative_time.get_year();

		if(relative_year >= yearly_proportions.begin()->first)
		{
			auto last = *(--yearly_proportions.end());
			if(relative_year < last.first)
			{
				double currentYearTargetProportion = yearly_proportions.at(relative_year);
				double nextYearTargetProportion = yearly_proportions.at(relative_year + 1);
				//TODO: there's a better way to do this
				double x = ((int)(month - monthOf1990).in_months() % 12) / 12.0;
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

/*
 * Treatment slots are calculated in one of two ways depending if
 * dynamic treatment scaling (DTS) is enabled or disabled. DTS is meant to
 * adjust for the lag in the treatment cascade in CEPAC.
 *
 * When DTS is enabled, we artificially increase the number of treatment slots
 * by a "correction factor". This correction factor is the ratio of the treatment
 * pool size in CDM over the number of people being treated in CEPAC (which will
 * always be greater than one). In this way we force the actual treatment proportion
 * closer to the value given as a parameter.
 *
 * When DTS is disabled, the number of treatment slots is the proportion of the
 * total population  eligible for rollout minus the number of slots already taken.
 */
int Population::UpdateTreatmentSlots(double rolloutProportion)
{
    int eligiblePopulation;
    switch (parameters_.rolloutProportionDenominator) {
    case (RolloutDenominator::POPULATION):
	    eligiblePopulation = GetSize();
	    break;
    case (RolloutDenominator::ELIGIBLE):
	    eligiblePopulation = (int)(rolloutTreatedPool.size() +
		rolloutUntreatedPool.size());
	    break;
    default:
	    eligiblePopulation = (int)(rolloutTreatedPool.size() +
		rolloutUntreatedPool.size());
	    break;
    }

    int numAccessingTreatment = (int)rolloutTreatedPool.size();
    double targetTreatmentSlots = eligiblePopulation * rolloutProportion;

    int numSlots;
    if(parameters_.enableDynamicTreatmentScaling) {
	int position = (int)(parameters_.currTime - parameters_.monthOf1990).in_months() %
	    parameters_.dynamicFeedbackPeriod;

	if(position == 0) {
	    treatmentCorrectionFactor_ = 1;
	    // Count the number of people reported to be on ART in CEPAC
	    auto numTreated = std::count_if(rolloutTreatedPool.begin(), rolloutTreatedPool.end(),
					    [](Entity *p) { return p->isOnArt(); });

	    if(numTreated > 0) {
		treatmentCorrectionFactor_ = numAccessingTreatment /
		    static_cast<double>(numTreated);
	    }
	}
	numSlots = static_cast<int>(targetTreatmentSlots * treatmentCorrectionFactor_) -
	    numAccessingTreatment;

    } else {
	numSlots = static_cast<int>(targetTreatmentSlots) - numAccessingTreatment;
    }

    return numSlots;
}

void Population::ApplyARTRollout(EventParams &parameters_)
{
	double rolloutProportion = InterpolateProportion(parameters_.targetYearlyRolloutProportions,
		parameters_.currTime, parameters_.monthOf1990);
	int newSlots = UpdateTreatmentSlots(rolloutProportion);
	populationStatistics.recordTreatmentSlots(newSlots);

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
		auto relative_time = Time(1990, 0) + (parameters_.currTime - parameters_.monthOf1990);
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

        populationStatistics.UpdateIncidenceCalculations();

		if(relative_time.get_month() == 11)
		{
			populationStatistics.printShiftedOutcomes(_outStream, relative_time);
			populationStatistics.resetYear(relative_time + TimeSpan::Year);
		}
	}
    else if(parameters_.monthOf1990 == (parameters_.currTime + TimeSpan::Month))
    {
        populationStatistics.resetYear(Time(1990, 0));
    }
}

bool Population::PassesPartnershipCalibration(EventParams &parameters_)
{
	//calculate partnership prevalence values
    unsigned long numInPartnership[(std::size_t)SexualPartnership::Type::Last][(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numInConcurrent[(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numActsMonth[(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numActsMonthRisk[(std::size_t)DemographicProfile::Gender::Last][(std::size_t)Entity::RiskLevel::Last];
    unsigned long numSexuallyActive[(std::size_t)DemographicProfile::Gender::Last];
    unsigned long numSexuallyActiveRisk[(std::size_t)DemographicProfile::Gender::Last][(std::size_t)Entity::RiskLevel::Last];
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
	secondRow << "Steady Partnership Prevalence" << Constants::Tab << "Casual Partnership Prevalence" << Constants::Tab <<
	          "CSW Partnership Prevalence" << Constants::Tab << "Prop in Concurrent" << Constants::Tab <<
	          "Num Acts (per person per month)" << Constants::Tab << "Casual Partnership Prev Ratio (FtM)" << Constants::Tab <<
	          "Prop in Concurrent Ratio (FtM)" << Constants::Tab << "Avg Num Acts Ratio (LR to HR Females)" << Constants::Tab <<
	          "Steady Partnership Prev" << Constants::Tab << "Casual Partnership Prevalence" << Constants::Tab <<
	          "CSW Partnership Prevalence" << Constants::Tab << "Prop In Concurrent" << Constants::Tab << "Num Acts" << Constants::Tab
	          << "Casual Partnership Prev Ratio (FtM)" << Constants::Tab << "Prop in Concurrent Ratio (FtM)" << Constants::Tab <<
	          "Avg Num Acts Ratio (LR to HR Females)" << Constants::Tab;

    for(int i = 0; i < (int)SexualPartnership::Type::Last; i++)
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

		for(std::size_t j = 0; j < (std::size_t)Entity::RiskLevel::Last; j++)
		{
			numActsMonthRisk[i][j] = 0;
			numSexuallyActiveRisk[i][j] = 0;
		}
	}

    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		std::list<Entity *>::iterator p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
            if((*p_Iter)->getDemographicProfileVal(DemographicProfile::Demographic::SexualActivityStatus) != (std::size_t)DemographicProfile::SexualActivityStatus::Active)
			{
				p_Iter++;
				continue;
			}

			Entity::RiskLevel risk = (*p_Iter)->getRiskLevel();
            numSexuallyActive[(std::size_t)gender]++;
            numSexuallyActiveRisk[(std::size_t)gender][(std::size_t)risk]++;
            numActsMonth[(std::size_t)gender] += (*p_Iter)->getNumActsThisMonth();
            numActsMonthRisk[(std::size_t)gender][(std::size_t)risk] += (*p_Iter)->getNumActsThisMonth();

			for(int i = 0; i < (int)SexualPartnership::Type::Last; i++)
			{
				if((*p_Iter)->getMonthOfLatestPartnershipDissolution((SexualPartnership::Type) i) > max(parameters_.currTime - TimeSpan::Year, Time::Zero))
				{
                    numInPartnership[i][(std::size_t)gender]++;
				}
			}

			if((*p_Iter)->getTimeOfLatestConcurrent() > max(parameters_.currTime - TimeSpan::Year, Time::Zero))
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
	double steadyPrev = 0.0, casualPrev = 0.0, CSWPrev = 0.0, propInConcurrent = 0.0, numActsAvg = 0.0;

	if(parameters_.calibrationInputs.steadyPrevPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::Tab;
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
		firstRow << "Male SA Pop" << Constants::Tab;
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
		firstRow << "Entire SA Pop" << Constants::Tab;
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
		firstRow << "Male SA Pop" << Constants::Tab;
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
		firstRow << "Entire SA Pop" << Constants::Tab;
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
		firstRow << "Male SA Pop" << Constants::Tab;
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
		firstRow << "Entire SA Pop" << Constants::Tab;
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
		firstRow << "Male SA Pop" << Constants::Tab;
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
		firstRow << "Entire SA Pop" << Constants::Tab;
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
		firstRow << "Male SA Pop" << Constants::Tab;
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
	firstRow << Constants::Tab;

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
	firstRow << Constants::Tab;

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

    unsigned long numSAFemaleHR = numSexuallyActiveRisk[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)Entity::RiskLevel::HIGH];
    unsigned long numSAFemaleLR = numSexuallyActiveRisk[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)Entity::RiskLevel::LOW];
	double numActsFemaleLRtoHRRatio = -1;
	firstRow << Constants::Tab;

	if(numSAFemaleHR != 0 && numSAFemaleLR != 0)
	{
        double avgNumActsFemaleHR = numActsMonthRisk[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)Entity::RiskLevel::HIGH] / (double)numSAFemaleHR;
        double avgNumActsFemaleLR = numActsMonthRisk[(std::size_t)DemographicProfile::Gender::Female][(std::size_t)Entity::RiskLevel::LOW] / (double)numSAFemaleLR;

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
	thirdRow << steadyPrev << Constants::Tab << casualPrev << Constants::Tab << CSWPrev << Constants::Tab <<
	         propInConcurrent << Constants::Tab << numActsAvg << Constants::Tab << casualPartPrevRatio << Constants::Tab <<
	         propConcurrentRatio << Constants::Tab << numActsFemaleLRtoHRRatio << Constants::Tab;
	thirdRow << (int)passesSteadyPrev << Constants::Tab << (int)passesCasualPrev << Constants::Tab <<
	         (int)passesCSWPrev << Constants::Tab << (int)passesPropInConcurrent << Constants::Tab <<
	         (int)passesNumActs << Constants::Tab << (int)passesCasualPartPrevRatio << Constants::Tab <<
	         (int)passesPropConcRatio << Constants::Tab << (int)passesNumActsLRtoHRRatio << Constants::Tab;

	if(parameters_.trace_files[EventParams::TraceFile::Type::CalibrationStatistics].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::CalibrationStatistics] << firstRow.str() << std::endl << secondRow.str() << std::endl <<
		        thirdRow.str() << std::endl;
	}

	return passesCalib;
}

unsigned long Population::CreatePartnerships(EventParams &parameters_, Male *_initiator,
        std::list<Entity *>::iterator * /*_p_Iter*/, SexualPartnership::Type _partnershipType, bool _forceNumPartnersOne)
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
        DemographicProfile::ProfileID profileID = _initiator->ChoosePartnerDemographic(parameters_.randomNums, _partnershipType);
        BucketSexualMixing *bucket = (BucketSexualMixing *)entities->getBucket(profileID);
        assert(bucket != nullptr);

        std::list<Entity *> attemptedPartners;
        bool foundPartner = false;
        Entity *chosenPartner = nullptr;
        bool printTracePartner = false;

        int maxRejections = _initiator->GetMaxPartnershipRejections();

        //Why 10? Magic numbers are evil! GA
        for(int i = 0; i < 10 /* should be maxRejections */; i++)
        {
	    // choose a partner -- they will be removed from the pool and added back later
            Entity *partner = bucket->drawMember(parameters_.randomNums, _initiator, _partnershipType, true);

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
            else if(partner != _initiator && popWideParams.ageOfMajority + partner->GetSexualActivityDelay() <= partner->getAge())
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

            for(std::list<Entity *>::iterator it = attemptedPartners.begin(); it != attemptedPartners.end(); it++)
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
            parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " age " << _initiator->getAge().in_months() << ", "
                << ((chosenPartner->getRiskLevel() == Entity::RiskLevel::HIGH) ? "HIGH" : "LOW") << " risk) forms " <<
                (SexualPartnership::TypeStrings.at(_partnershipType)) << " with female " << chosenPartner->getID() << " (";
            chosenPartner->getDemographicProfile()->print(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].file, "");
			parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << " age " << chosenPartner->getAge().in_months()
                << ", " << ((chosenPartner->getRiskLevel() == Entity::RiskLevel::HIGH) ? "HIGH" : "LOW") << " risk)";
        }

        //the pointer to this partnership will be stored within initiator.
        new SexualPartnership(_initiator, chosenPartner, parameters_, _partnershipType);
        chosenPartner->IncrementTimesSelected();

        //add all persons back to entity pool
        for(std::list<Entity *>::iterator it = attemptedPartners.begin(); it != attemptedPartners.end(); it++)
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
    }

	//TODO: Return difference between numFormed and original value of numPartners as well as numFormed... create field inside initiator
	return numFormed;
}

void Population::ProcessDeath(EventParams &parameters_, Entity *_p, bool calculateLE)
{
	assert((_p != nullptr));
	assert((!_p->isAlive()));

    dead_people_this_month_.insert(_p);

	//Calculate life expectancy info
	if(calculateLE)
	{
		assert((populationStatistics.selectedLEStats != nullptr));
        assert(_p->getAge() >= Age::Zero);
        assert(_p->getAge() <= Time(Entity::maxYrForDeathStats, 0));
        populationStatistics.selectedLEStats->deathsByAge[_p->getAge().year_as_index()]++;
	}

	//holds any former steady partners that are widowed after a partner's death
	// we may need to put the partners back into the singles pool
	std::list<SexualPartnership *> formerPartnerships;

	//we have to take care of what happens to any ongoing partnerships
	for(int type = 0; type < (int)SexualPartnership::Type::Last; ++type)
	{
		_p->getPartnershipsToEnd(parameters_.currTime, SexualPartnership::Type(type), formerPartnerships, true);
	}

	DissolveSexualPartnerships(parameters_, _p, formerPartnerships);
    currDeathCauses[(std::size_t)_p->deathStatus]++;
	populationStatistics.processDeath(_p, parameters_);

	if(parameters_.useRollout) {
	    //Remove people from the treated/untreated pool if they die
	    std::list<Entity *>::iterator poolIterator;
	    poolIterator = std::find(rolloutUntreatedPool.begin(),
				     rolloutUntreatedPool.end(), _p);

	    if(poolIterator != rolloutUntreatedPool.end()) {
		rolloutUntreatedPool.erase(poolIterator);
	    } else {
		poolIterator = std::find(rolloutTreatedPool.begin(),
					 rolloutTreatedPool.end(), _p);

		if(poolIterator != rolloutTreatedPool.end())
		    rolloutTreatedPool.erase(poolIterator);
	    }
	}

	delete _p;
}

std::size_t Population::CalcPrevalentPopulation(Time time)
{
    assert(time >= Time::Zero);

    //total infected in the while population
    std::size_t totalInfected = 0;

	//holds number of prevalent infections
	std::array<InfectionsTracker::DemographicArray, InfectionsTracker::NUMBER_GENERATIONS_TO_TRACE> prevalenceByBucket;
	InfectionsTracker::EntityTypeArray<InfectionsTracker::RiskEmploymentArray> prevalenceByEntityTypeRiskEmployment;
    InfectionsTracker::EntityTypeArray<AgeRangeSizeContainer> prevalenceByEntityTypeAge;

	//initialize prevalent infections by age
    for(auto ageBucketParams : popWideParams.GetInitialAgeBuckets())
	{
		AgeRange range = {ageBucketParams.GetMinAge(), ageBucketParams.GetMaxAge()};
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::size_t entity_type = (std::size_t)gender;
			prevalenceByEntityTypeAge[entity_type].push_back({range, 0});
		}
	}

	//initialize prevalence tallies to 0
	for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
	{
		for(auto gender : enum_iterator<DemographicProfile::Gender>())
		{
			std::size_t entity_type = (std::size_t)gender;
            for(int k = 0; k < (int)DemographicProfile::Employment::Last; k++)
            {
                prevalenceByEntityTypeRiskEmployment[entity_type][i][k] = 0;
            }
        }
	}

	DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;
	while(currProfileID <= DemographicProfile::MAX)
	{
		for(int i = 0; i < populationStatistics.infectionsTracker.NUMBER_GENERATIONS_TO_TRACE; i++)
		{
			prevalenceByBucket[i][currProfileID] = 0;
		}

		currProfileID++;
	}

    auto get_age_bucket = [this](Age age)
    {
        std::size_t i = 0;

        for(auto ageBucketParams : popWideParams.initialAgeBuckets)
        {
            if(age >= ageBucketParams.GetMinAge() && age <= ageBucketParams.GetMaxAge())
            {
                break;
            }

            i++;
        }

        return i;
    };

    entities->forEach([&](Entity *e)
    {
        if(e->isInfected())
        {
			auto gender = (std::size_t)e->getDemographicProfileVal<DemographicProfile::Gender>();
			prevalenceByBucket[e->getGenerationOfInfection()][e->getDemographicProfile()->getProfileID()]++;
            prevalenceByEntityTypeAge[gender][get_age_bucket(e->age)].second++;
            prevalenceByEntityTypeRiskEmployment[gender][(std::size_t)e->getRiskLevel()]
			    [(std::size_t)e->getDemographicProfileVal<DemographicProfile::Employment>()]++;
        }
    });

    //save the prevalent infections by bucket in the PopulationStatistics
	populationStatistics.infectionsTracker.setPrevalentInfections(prevalenceByBucket, prevalenceByEntityTypeAge, prevalenceByEntityTypeRiskEmployment);

    return totalInfected;
}

AgeBucketPrevalenceInfo &Population::GetAgeBucket(Entity *p)
{
    auto age = p->getAge();

	for(unsigned int ageBucket = 0; ageBucket < popWideParams.initialAgeBuckets.size(); ageBucket++)
	{
		//holds the parameters for the current age bucket
		auto &ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucket);

		if(age >= ageBucketParams.GetMinAge() && age <= ageBucketParams.GetMaxAge())
		{
			return ageBucketParams;
		}
	}

	throw std::runtime_error("bucket not found");
	//return popWideParams.initialAgeBuckets.back();
}

int Population::GetAgeBucketIndex(Entity *p)
{
    auto age = p->getAge();
	unsigned int ageBucket;

	for(ageBucket = 0; ageBucket < popWideParams.initialAgeBuckets.size(); ageBucket++)
	{
		//holds the parameters for the current age bucket
		auto &ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucket);

		if(age >= ageBucketParams.GetMinAge() && age <= ageBucketParams.GetMaxAge())
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

std::size_t Population::GetSize(DemographicProfile::ProfileID profileID)
{
    if(currSizeEntityType.find(profileID) != currSizeEntityType.end())
    {
        return currSizeEntityType.at(profileID);
    }
    return 0;
}

std::size_t Population::GetSASize(DemographicProfile::ProfileID profileID, Entity::RiskLevel _risk)
{
    return currSASizeEntityTypeRisk[profileID][(std::size_t)_risk];
}

std::size_t Population::GetCSWSize(DemographicProfile::ProfileID profileID, Entity::RiskLevel _risk)
{
    return currSizeEntityTypeRiskCSW[profileID][(std::size_t)_risk];
}

void Population::PrintPartnerships(EventParams &parameters_, Time _time, std::ostream &_outStream)
{
	assert(_time >= Time::Zero);
	std::string orientationLabels[] = { "MSW", "MSM", "MSMW" };
	std::string employmentLabels[] = { "Non-CSW", "CSW" };
	std::string riskLabels[] = { "LR", "HR" };
	std::string relationshipLabels[] = { "Non-Single", "Single" };
	std::string partnershipLabelsHetero[] = { "Steady", "Regular", "Casual", "CSW" };
	std::string riskLabels2[] = { "HR", "Mix", "LR" };

	if(_time == Time::Zero)
	{
		std::ostringstream firstRow;
		std::ostringstream secondRow;
		std::ostringstream thirdRow;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Month" << Constants::Tab;

        //Partnership Headers
        for (auto partnership_type : enum_iterator<SexualPartnership::Type>())
        {
            firstRow << "Individuals by Partnership";
            secondRow << SexualPartnership::TypeStrings.at(partnership_type);

            for (int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
            {
                for (int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
                {
                    for (auto entity_type :  { "MSW", "MSMW", "MSM", "female" })
                    {
                        for (int m = (int)Entity::RiskLevel::HIGH; m >= 0; m--)
                        {
                            firstRow << Constants::Tab;
                            secondRow << Constants::Tab;
                            thirdRow << entity_type << Constants::Space << relationshipLabels[k] << Constants::Space << employmentLabels[l] <<
                              Constants::Space << riskLabels[m] << Constants::Tab;
                        }
                    }
                }
            }
		}

		//Concurrent Partnerships
		firstRow << "Individuals by Concurrent Partnerships";

		// Concurrent by <entity-type> <risk> Non-CSW
		for (std::string entity_type : { "MSW", "MSM", "MSMW", "female" })
		{
			for (std::string risk : { "High", "Low" })
			{
				for (std::string csw : { "Non-CSW", "CSW" })
				{
					if (csw == "CSW" && entity_type == "MSW") continue;

					firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
					secondRow << entity_type << " " << risk << " Risk " << csw << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
						Constants::Tab;;
					thirdRow << "Concurrent" << Constants::Tab << "2 Partners" << Constants::Tab << "3 Partners" << Constants::Tab <<
						"4 Partners" << Constants::Tab << "5+ Partners" << Constants::Tab;
				}
			}
		}

		//Partnerships by partnership type
		firstRow << "Partnerships by Partnership Type";

		for (std::string partnership_type : { "Male+Female" , "MSMW+Female", "MSMW+MSM", "MSM+MSM" })
		{
			secondRow << partnership_type;

            for (auto partnership_type : enum_iterator<SexualPartnership::Type>())
			{
				for (int j = 0; j < 3; j++)
				{
					std::string label = SexualPartnership::TypeStrings.at(partnership_type);
					firstRow << Constants::Tab;
					secondRow << Constants::Tab;
					thirdRow << label << Constants::Space << riskLabels2[j] << Constants::Tab;
				}
			}
		}

		for (auto entity_type : { "MSMW", "MSM", "Female" })
		{
			secondRow << entity_type << " Selection Statistics" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
			thirdRow << "Min" << Constants::Tab << "Max" << Constants::Tab << "Mean" << Constants::Tab << "Std. Dev." << Constants::Tab << "Median" << Constants::Tab << "Mode" << Constants::Tab;
		}

		//write out string buffers to trace file
		_outStream << firstRow.str() << std::endl;
		_outStream << secondRow.str() << std::endl;
		_outStream << thirdRow.str() << std::endl;
	}

	//Month
	if(_time == Time::Zero)
	{
		_outStream << "Init" << Constants::Tab;
	}
	else
	{
		_outStream << _time.in_months() << Constants::Tab;
	}

	std::unordered_map<std::string, descriptive_stats_container<std::size_t>> times_selected_stats;
	using EmploymentRiskArray = std::array<std::array<std::size_t, (std::size_t)Entity::RiskLevel::Last>, (std::size_t)DemographicProfile::Employment::Last>;
	using RelationshipEmploymentRiskArray = std::array<EmploymentRiskArray, (std::size_t)DemographicProfile::RelationshipStatus::Last>;
	std::array<std::unordered_map<std::string, RelationshipEmploymentRiskArray>, (std::size_t)SexualPartnership::Type::Last> num_in_partnership;
	using ConcurrentCountArray = std::array<EmploymentRiskArray, 5>;
	std::unordered_map<std::string, ConcurrentCountArray> num_in_concurrent;
	std::unordered_map<std::string, std::array<std::array<std::size_t, (std::size_t)SexualPartnership::Type::Last>, 3>> double_num_partnerships;

	entities->forEach([&](Entity *e)
	{
		int concurrent = 0;
		std::size_t num_partners = 0;

		auto gender_index = (std::size_t)e->getDemographicProfileVal<DemographicProfile::Gender>();
		auto orientation_index = (std::size_t)e->getDemographicProfileVal<DemographicProfile::SexualOrientation>();

		auto entity_type = "female";
		if (e->isMale())
		{
			switch (orientation_index) {
			case (std::size_t)DemographicProfile::SexualOrientation::Msw:
				entity_type = "MSW";
				break;
			case (std::size_t)DemographicProfile::SexualOrientation::Msm:
				entity_type = "MSM";
				break;
			case(std::size_t)DemographicProfile::SexualOrientation::Msmw:
				entity_type = "MSMW";
				break;
			default:
				throw std::runtime_error("Unknown sexual orientation");
			}
		}

		auto risk_index = (std::size_t)e->getRiskLevel();
		auto relationship_status_index = (std::size_t)e->getDemographicProfileVal<DemographicProfile::RelationshipStatus>();
		auto employment_index = (std::size_t)e->getDemographicProfileVal<DemographicProfile::Employment>();

		for (auto partnership_type : enum_iterator<SexualPartnership::Type>())
		{
			auto num_partners_type = static_cast<std::size_t>(e->getNumPartners(partnership_type));
			num_partners += num_partners_type;

			if ((int)partnership_type < 4)
			{
				concurrent <<= 1;
			}

			if (num_partners_type > 0)
			{
				if ((int)partnership_type < 4)
				{
					concurrent++;
				}
				num_in_partnership[(std::size_t)partnership_type][entity_type][relationship_status_index][employment_index][risk_index]++;
			}

			auto j = e->getRiskLevel() == Entity::RiskLevel::HIGH ? 0 : 2;

			double_num_partnerships[entity_type][1][(std::size_t)partnership_type] += static_cast<std::size_t>(e->getNumPartners(partnership_type, false));
			double_num_partnerships[entity_type][j][(std::size_t)partnership_type] += static_cast<std::size_t>(e->getNumPartners(partnership_type, true));
		}

		if (num_partners >= 2)
		{
			auto index = std::min((std::size_t)5, num_partners) - 1;
			num_in_concurrent[entity_type][index][employment_index][risk_index]++;
		}

		concurrent = 15 - concurrent;
		assert(concurrent >= 0 && concurrent < Constants::NumberConcurrencyDefs);

		if (parameters_.concurrencyDef[concurrent].useDefinition
		    && num_partners >= static_cast<std::size_t>(parameters_.concurrencyDef[concurrent].minPartnershipsNeeded))
		{
			num_in_concurrent[entity_type][0][employment_index][risk_index]++;
		}

		if (entity_type != "MSW")
		{
			times_selected_stats[entity_type].insert(e->GetTimesSelected());
		}
	});


    for (auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {

        for (int l = 0; l < (int)DemographicProfile::Employment::Last; l++)
        {
            for (int k = 0; k < (int)DemographicProfile::RelationshipStatus::Last; k++)
            {
                for (auto entity_type :  { "MSW", "MSMW", "MSM", "female" })
                {
                    for (int m = (int)Entity::RiskLevel::HIGH; m >= 0; m--)
                    {
                        _outStream << num_in_partnership[(std::size_t)partnership_type][entity_type][k][l][m] << Constants::Tab;
                    }
                }
            }
        }
	}

	for (std::string entity_type : { "MSW", "MSM", "MSMW", "female" })
	{
		for (int risk = (int)Entity::RiskLevel::HIGH; risk >= 0; risk--)
		{
			for (int csw = 0; csw < (int)DemographicProfile::Employment::Last; csw++)
			{
				if (csw == (int)DemographicProfile::Employment::Csw && entity_type == "MSW") continue;
				for (int i = 0; i < 5; i++)
				{
					_outStream << num_in_concurrent[entity_type][i][csw][risk] << Constants::Tab;
				}
			}
		}
	}

	for (std::string partnership_type : { "MSW+Female" , "MSMW+Female", "MSMW+MSM", "MSM+MSM" })
	{
		std::string partner1;
		std::string partner2;

		if (partnership_type == "Male+Female")
		{
			partner1 = "msw";
			partner2 = "female";
		}
		else if (partnership_type == "MSMW+Female")
		{
			partner1 = "msmw";
			partner2 = "female";
		}
		else if (partnership_type == "MSMW+MSM")
		{
			partner1 = "msmw";
			partner2 = "msm";
		}
		else if (partnership_type == "MSM+MSM")
		{
			partner1 = "msm";
			partner2 = "msm";
		}

		for (auto partnership_type : enum_iterator<SexualPartnership::Type>())
		{
			for (int j = 0; j < 3; j++)
			{
				if (double_num_partnerships[partner1][j][(std::size_t)partnership_type] == 0
					|| double_num_partnerships[partner2][j][(std::size_t)partnership_type] == 0)
				{
					_outStream << 0 << Constants::Tab;
					continue;
				}

				auto count = (double_num_partnerships[partner1][j][(std::size_t)partnership_type]
					+ double_num_partnerships[partner2][j][(std::size_t)partnership_type]) / 2;
				_outStream << count << Constants::Tab;
			}
		}
	}

	for (auto entity_type : { "msmw", "msm", "female" })
	{
		if (times_selected_stats[entity_type].get_num_samples() > 0)
		{
			_outStream << times_selected_stats[entity_type].get_min() << Constants::Tab;
			_outStream << times_selected_stats[entity_type].get_max() << Constants::Tab;
			_outStream << times_selected_stats[entity_type].get_mean() << Constants::Tab;
			_outStream << times_selected_stats[entity_type].get_stddev() << Constants::Tab;
			_outStream << times_selected_stats[entity_type].get_median() << Constants::Tab;
			_outStream << times_selected_stats[entity_type].get_mode() << Constants::Tab;
		}
		else
		{
			_outStream << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		}
	}

	_outStream << std::endl;

}

void Population::PrintClinical(EventParams &/*parameters_*/, Time _time, std::ostream &_outStream)
{
	assert(_time >= Time::Zero);

	if(_time == Time::Zero)
	{
		std::ostringstream firstRow;
		std::ostringstream secondRow;
		std::ostringstream thirdRow;
		firstRow << Constants::Tab;
		secondRow << Constants::Tab;
		thirdRow << "Month" << Constants::Tab;
		//HIV Status among all SA pop
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among Sexually Active Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
		//CSW
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among CSW SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
		//High Risk
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among High Risk Non-CSW SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
		//Low Risk
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among Low Risk Non-CSW SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
		//Male
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among Male SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
        //Msw High Risk
        firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab;
        secondRow << "HIV Status Among High Risk Male:Msw SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
        thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
            "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
            << "Unobserved Latestage" << Constants::Tab;
        //Msw Low Risk
        firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab;
        secondRow << "HIV Status Among Low Risk Male:Msw SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
        thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
            "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
            << "Unobserved Latestage" << Constants::Tab;
        //Msmw High Risk
        firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab;
        secondRow << "HIV Status Among High Risk Male:Msmw SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
        thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
            "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
            << "Unobserved Latestage" << Constants::Tab;
        //Msmw Low Risk
        firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab;
        secondRow << "HIV Status Among Low Risk Male:Msmw SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
        thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
            "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
            << "Unobserved Latestage" << Constants::Tab;
        //Msm High Risk
        firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab;
        secondRow << "HIV Status Among High Risk Male:Msm SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
        thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
            "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
            << "Unobserved Latestage" << Constants::Tab;
        //Msm Low Risk
        firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab;
        secondRow << "HIV Status Among Low Risk Male:Msm SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
            Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
        thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
            "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
            << "Unobserved Latestage" << Constants::Tab;
		//Female
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among Female SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
		//Female High Risk
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among High Risk Female SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
		//Female Low Risk
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
		         Constants::Tab;
		secondRow << "HIV Status Among Low Risk Female SA Population" << Constants::Tab << Constants::Tab << Constants::Tab <<
		          Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "HIV-" << Constants::Tab << "Observed Acute" << Constants::Tab << "Unobserved Acute" << Constants::Tab <<
		         "Observed Chronic" << Constants::Tab << "Unobserved Chronic" << Constants::Tab << "Observed Latestage" << Constants::Tab
		         << "Unobserved Latestage" << Constants::Tab;
		//CD4 At Transmission
		firstRow << Constants::Tab << Constants::Tab << Constants::Tab;
		secondRow << "CD4 At Transmission" << Constants::Tab << Constants::Tab << Constants::Tab;
		thirdRow << "Num Transmissions" << Constants::Tab << "Mean CD4" << Constants::Tab << "SD CD4" << Constants::Tab;
		_outStream << firstRow.str() << std::endl;
		_outStream << secondRow.str() << std::endl;
		_outStream << thirdRow.str() << std::endl;
	}

	//Month
	if(_time == Time::Zero)
	{
		_outStream << "Init" << Constants::Tab;
	}
	else
	{
		_outStream << _time.in_months() << Constants::Tab;
	}

    std::unordered_map<std::string, std::array<std::array<std::array<unsigned long, (std::size_t)Entity::HIVStatus::Last>, (std::size_t)DemographicProfile::Employment::Last>, (std::size_t)Entity::RiskLevel::Last>> numWithHIVStatus;

    for(auto entity_type : {"MSW", "MSM", "MSMW", "female"})
    {
        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(int j = 0; j < (int)DemographicProfile::Employment::Last; j++)
            {
                for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
                {
                    numWithHIVStatus[entity_type][i][j][m] = 0;
                }
            }
        }
    }

    entities->forEach([&](Entity *e)
    {
        numWithHIVStatus[e->getEntityType()][(std::size_t)e->getRiskLevel()][(std::size_t)e->getDemographicProfileVal<DemographicProfile::Employment>()][(std::size_t)e->getHIVStatus()]++;
    });

	//HIV Status of entire SA population
	unsigned long hivStatusSA[(std::size_t)Entity::HIVStatus::Last];

	for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
	{
		hivStatusSA[m] = 0;
	}

    for(auto entity_type : {"MSW", "MSM", "MSMW", "female"})
    {
        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(int j = 0; j < (int)DemographicProfile::Employment::Last; j++)
            {
                for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
                {
                    hivStatusSA[m] += numWithHIVStatus[entity_type][i][j][m];
                }
            }
        }
    }

	for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
	{
		_outStream << hivStatusSA[m] << Constants::Tab;
	}

	//HIV Status of CSW SA population
	unsigned long hivStatusCSW[(std::size_t)Entity::HIVStatus::Last];

	for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
	{
		hivStatusCSW[m] = 0;
	}

    for (auto entity_type : {"MSW", "MSM", "MSMW", "female"})
    {
        for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
        {
            for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
            {
                hivStatusCSW[m] += numWithHIVStatus[entity_type][i][(std::size_t)DemographicProfile::Employment::Csw][m];
            }
        }
    }

	for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
	{
		_outStream << hivStatusCSW[m] << Constants::Tab;
	}

	//HIV Status of Risk Non CSW SA population
	for(int i = (int)Entity::RiskLevel::HIGH; i >= 0; i--)
	{
		unsigned long hivStatusRisk[(std::size_t)Entity::HIVStatus::Last];

		for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
		{
			hivStatusRisk[m] = 0;
		}

		for(auto entity_type : {"MSW", "MSM", "MSMW", "female"})
        {
		    for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
			{
                hivStatusRisk[m] += numWithHIVStatus[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw][m];
			}
		}

		for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
		{
			_outStream << hivStatusRisk[m] << Constants::Tab;
		}
	}

    //HIV Status of Risk Non CSW SA Male population
    unsigned long hivStatusMale[(std::size_t)Entity::HIVStatus::Last];

    for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
    {
        hivStatusMale[m] = 0;
    }

    for(int i = (int)Entity::RiskLevel::HIGH; i >= 0; i--)
    {
        for(auto entity_type : {"MSW", "MSM", "MSMW"})
        {
            for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
            {
                hivStatusMale[m] += numWithHIVStatus[entity_type][i][(std::size_t)DemographicProfile::Employment::NonCsw][m];
            }
        }
    }

    for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
    {
        _outStream << hivStatusMale[m] << Constants::Tab;
    }

	//HIV Status of SA population by Gender and Risk
	for (auto entity_type : {"MSW", "MSM", "MSMW", "female"})
	{
		unsigned long hivStatusGender[(std::size_t)Entity::HIVStatus::Last];
		unsigned long hivStatusGenderRisk[(std::size_t)Entity::RiskLevel::Last][(std::size_t)Entity::HIVStatus::Last];

		for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
		{
			hivStatusGender[m] = 0;

			for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
			{
				hivStatusGenderRisk[i][m] = 0;
			}
		}

		for(std::size_t i = 0; i < (std::size_t)Entity::RiskLevel::Last; i++)
		{
            for(int j = 0; j < (int)DemographicProfile::Employment::Last; j++)
			{
			    for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
				{
					hivStatusGender[m] += numWithHIVStatus[entity_type][i][j][m];
					hivStatusGenderRisk[i][m] += numWithHIVStatus[entity_type][i][j][m];
				}
			}
		}

        if(std::string(entity_type) == "female")
        {
            for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
            {
                _outStream << hivStatusGender[m] << Constants::Tab;
            }
        }

		for(int i = (int)Entity::RiskLevel::HIGH; i >= 0; i--)
		{
		    for(std::size_t m = 0; m < (std::size_t)Entity::HIVStatus::Last; m++)
			{
				_outStream << hivStatusGenderRisk[i][m] << Constants::Tab;
			}
		}
	}

	populationStatistics.infectionsTracker.recordCD4AtTransmission(_outStream);
	_outStream << std::endl;
}

void Population::PrintPopulationHeaders(Time _time, std::ostream &_outStream)
{
    //total # of age ranges to print out
    std::vector<AgeRange> age_ranges = GetAgeRanges();

    std::ostringstream firstRow;
    std::ostringstream secondRow;

    //incidence-related headers
    firstRow << Constants::Tab;
    secondRow << "Month" << Constants::Tab;
    firstRow << Constants::Tab;
    secondRow << "Pop Size" << Constants::Tab;
    firstRow << Constants::Tab;
    secondRow << "SA Pop Size" << Constants::Tab;
    firstRow << "Deaths" << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab << Constants::Tab <<
	Constants::Tab << Constants::Tab;
    secondRow << "Any OI" << Constants::Tab << "Chronic AIDS" << Constants::Tab << "Non-AIDS" << Constants::Tab <<
	"ART Toxicity" << Constants::Tab << "Proph Toxicity" << Constants::Tab << "Other" << Constants::Tab << "Total" <<
	Constants::Tab;
    firstRow << "Gender" << Constants::Tab;
    secondRow << "Females" << Constants::Tab;
	firstRow << Constants::Tab;
    secondRow << "All Males" << Constants::Tab;
    // By Orientation
    firstRow << "Male By Orientation" << Constants::Tab << Constants::Tab
             << Constants::Tab;
	for (std::string label : { "MSW", "MSMW", "MSM" })
    {
        secondRow << label << Constants::Tab;
    }
    // By Race and Ethnicity
    firstRow << "Race and Ethnicity" << Constants::Tab
             << Constants::Tab << Constants::Tab << Constants::Tab;
	for (std::string label : { "Black:NonHispanic", "Black:Hispanic", "White:NonHispanic", "White:Hispanic"})
    {
        secondRow << label << Constants::Tab;
    }

    //write out headers for population by age
    firstRow << "Non-SA Pop (All Ages)" << Constants::Tab;
    secondRow << "All ages" << Constants::Tab;

    for(std::size_t i = 0; i < age_ranges.size(); i++)
	{
		if(i == 0) {
			firstRow << "SA Pop (Age Months)";
		}

		firstRow << Constants::Tab;
		secondRow << age_ranges.at(i) << Constants::Tab;
    }

    firstRow << "Male (By Age)" << Constants::Tab;
    secondRow << "Non-SA" << Constants::Tab;

    for(std::size_t i = 0; i < age_ranges.size(); i++)
	{
		if(i == 0) {
			firstRow << "SA Pop (Age Months)";
		}

		firstRow << Constants::Tab;
		secondRow << age_ranges.at(i) << Constants::Tab;
    }

    firstRow << "Female (By Age)" << Constants::Tab;
    secondRow << "Non-SA" << Constants::Tab;

    for(std::size_t i = 0; i < age_ranges.size(); i++)
	{
		if(i == 0) {
			firstRow << "SA Pop (Age Months)";
	}

		firstRow << Constants::Tab;
		secondRow << age_ranges.at(i) << Constants::Tab;
    }

    firstRow << "Number circumcised" << Constants::Tab << Constants::Tab;
    secondRow << "NA" << Constants::Tab << "SA" << Constants::Tab;

    // By Risk
	DemographicProfile SAProfile;
    std::vector<DemographicProfile::ProfileID> SAProfileIDs;

    SAProfile.set(DemographicProfile::Demographic::SexualActivityStatus,
		(std::size_t)DemographicProfile::SexualActivityStatus::Active);
    SAProfile.selectProfileIDs(SAProfileIDs, &demographicProfileIDs);
    //output size by risk
	for (auto profile : SAProfileIDs)
	{
		firstRow << *DemographicProfile::toString(profile);
		for(auto risk : enum_iterator<Entity::RiskLevel>())
		{
			firstRow << Constants::Tab;
			secondRow << Entity::RiskStrings[(std::size_t)risk] << Constants::Tab;
		}
    }

	_outStream << firstRow.str() << std::endl;
    _outStream << secondRow.str() << std::endl;
}

void Population::PrintPopulation(EventParams &/*_paramters*/, Time _time, std::ostream &_outStream)
{
    //Month
    if(_time == Time::Zero)
	{
		PrintPopulationHeaders(_time, _outStream);
		_outStream << "init" << Constants::Tab;
    }
	else
	{
		_outStream << _time.in_months() << Constants::Tab;
    }

    //output population size and SA pop size
    _outStream << GetSize() << Constants::Tab;
    _outStream << GetSize() - GetNASize() << Constants::Tab;

	//output deaths by causes
    _outStream << currDeathCauses[(std::size_t)Entity::DeathStatus::DTH_OI]
        << Constants::Tab
        << currDeathCauses[(std::size_t)Entity::DeathStatus::DTH_CHRAIDS]
        << Constants::Tab
        << currDeathCauses[(std::size_t)Entity::DeathStatus::DTH_NONAIDS]
        << Constants::Tab
        << currDeathCauses[(std::size_t)Entity::DeathStatus::DTH_TOX_ART]
        << Constants::Tab
        << currDeathCauses[(std::size_t)Entity::DeathStatus::DTH_TOX_PROPH]
        << Constants::Tab
        << currDeathCauses[(std::size_t)Entity::DeathStatus::DTH_OTHER]
        << Constants::Tab;

    std::size_t totalDeaths = 0;
    for(int i = 1; i < (int)Entity::DeathStatus::Last; i++)
    {
		totalDeaths += currDeathCauses[i];
    }
    _outStream << totalDeaths << Constants::Tab;

    //output size of male and female populations
    _outStream << GetSize(DemographicProfile::Gender::Female) << Constants::Tab;
    _outStream << GetSize(DemographicProfile::Gender::Male) << Constants::Tab;

    // output size by orientation
    std::array<std::size_t, (std::size_t)DemographicProfile::SexualOrientation::Last> orientationTotals;
    orientationTotals.fill(0);
	for (auto orientation : enum_iterator<DemographicProfile::SexualOrientation>())
	{
        DemographicProfile profile;
        std::vector<DemographicProfile::ProfileID> profileIDs;
        profile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);

        profile.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)orientation);
        profile.selectProfileIDs(profileIDs, &demographicProfileIDs);
        for (auto profileID : profileIDs)
        {
            orientationTotals[(std::size_t)orientation] += entities->size(profileID);
        }
    }
    for (auto total : orientationTotals)
	{
        _outStream << total << Constants::Tab;
    }

    // by race and ethnicity
    // output size by orientation
    const std::size_t raceEthnicCount = (std::size_t)DemographicProfile::Race::Last *
      (std::size_t)DemographicProfile::Ethnicity::Last;
    std::array<std::size_t, raceEthnicCount> raceEthnicTotals;
    raceEthnicTotals.fill(0);

    int index = 0;
	for (auto race : enum_iterator<DemographicProfile::Race>())
	{
        for (auto ethnicity : enum_iterator<DemographicProfile::Ethnicity>())
        {
            DemographicProfile profile;
            std::vector<DemographicProfile::ProfileID> profileIDs;

            profile.set(DemographicProfile::Demographic::Race, (std::size_t)race);
            profile.set(DemographicProfile::Demographic::Ethnicity, (std::size_t)ethnicity);

            profile.selectProfileIDs(profileIDs, &demographicProfileIDs);
            for (auto profileID : profileIDs)
            {
                raceEthnicTotals[index] += entities->size(profileID);
            }
            index++;
        }
    }
    for (auto total : raceEthnicTotals)
	{
        _outStream << total << Constants::Tab;
    }

    // tally the size of non-sexually actives by gender
	std::size_t total_na = 0;
    std::size_t total_na_male = 0;
    std::size_t total_na_female = 0;

    DemographicProfile GenderProfile;
    std::vector<DemographicProfile::ProfileID> GenderProfileIDs;

    //First tally the men
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
	GenderProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
    GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);
    for(std::size_t i = 0; i < GenderProfileIDs.size(); i++)
    {
        total_na_male += entities->size(GenderProfileIDs[i]);
    }

	// Next tally the women
    GenderProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
	GenderProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
	GenderProfileIDs.clear();
    GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);
    for(std::size_t i = 0; i < GenderProfileIDs.size(); i++)
    {
        total_na_female += entities->size(GenderProfileIDs[i]);
    }
	total_na = total_na_male + total_na_female;

	// tally the size of sexually actives by age and gender
	AgeRangeSizeContainer size_by_age_range;
    AgeRangeSizeContainer size_by_age_range_male;
    AgeRangeSizeContainer size_by_age_range_female;
	for (auto age_range : GetAgeRanges())
	{
		size_by_age_range.push_back({age_range,0});
		size_by_age_range_male.push_back({age_range,0});
		size_by_age_range_female.push_back({age_range,0});
	}

	for(auto age_range_iter : currSizeByEntityTypeAgeRange)
	{
		DemographicProfile::ProfileID profile = age_range_iter.first;
		AgeRangeSizeContainer age_range_list = age_range_iter.second;

		std::size_t i = 0;
		for (auto age_range : age_range_list)
		{
			size_by_age_range[i].second += age_range.second;

			auto maleProfile = ((std::size_t)DemographicProfile::get(profile,
				DemographicProfile::Demographic::Gender) ==
				(std::size_t)DemographicProfile::Gender::Male);
			if (maleProfile)
			{
				size_by_age_range_male[i].second += age_range.second;
			}
			else
			{
				size_by_age_range_female[i].second += age_range.second;
			}
			i++;
		}
    }

    // print out both genders size by age
	_outStream << total_na << Constants::Tab;
    for(auto &ageRangeSize : size_by_age_range)
    {
        _outStream << ageRangeSize.second << Constants::Tab;
    }

    // print out male size by age
    _outStream << total_na_male << Constants::Tab;
    for(auto &ageRangeSize : size_by_age_range_male)
    {
        _outStream << ageRangeSize.second << Constants::Tab;
    }

    // print out female size by age
    _outStream << total_na_female << Constants::Tab;
    for(auto &ageRangeSize : size_by_age_range_female)
    {
        _outStream << ageRangeSize.second << Constants::Tab;
    }

    _outStream << num_circumcised_na << Constants::Tab;
    _outStream << num_circumcised_sa << Constants::Tab;

    //output size by risk
    DemographicProfile SAProfile;
    std::vector<DemographicProfile::ProfileID> SAProfileIDs;
    SAProfile.set(DemographicProfile::Demographic::SexualActivityStatus,
		(std::size_t)DemographicProfile::SexualActivityStatus::Active);
    SAProfile.selectProfileIDs(SAProfileIDs, &demographicProfileIDs);

    //output size by risk
	for (auto profile : SAProfileIDs)
	{
		for(auto risk : enum_iterator<Entity::RiskLevel>())
		{
			_outStream << GetSASize(profile, risk) << Constants::Tab;
		}
    }

    _outStream << std::endl;
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
	std::list<Entity *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
    for(auto gender : enum_iterator<DemographicProfile::Gender>())
	{
		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			int numPartnersInHistory = (*p_Iter)->getNumPartnersInHistory();

			if(numPartnersInHistory >= PopulationStatisticsOld::SinglePartAcqStats::NUM_PARTNER_BINS)
			{
				numPartnersInHistory = PopulationStatisticsOld::SinglePartAcqStats::NUM_PARTNER_BINS - 1;
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

void Population::Initialize(const PopulationParameters &parameters)
{
    popWideParams = parameters;

    std::map<SexualPartnership::Type, double> assort;
    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        if(!popWideParams.GetMaleParameters().hasSexualBehavior(partnership_type)) continue;
        assort[partnership_type] = popWideParams.GetMaleParameters().
		  getSexualBehavior(partnership_type).getRiskAssortativeness();
    }

    //create EntityPool - this will contain all Entities
    entities.reset(new EntityPool(popWideParams, GetId(), assort));

	// initialize the list of demographic profile ids used in the simulation
	demographicProfileIDs = entities->getProfileIDs();

	for (auto profileID : demographicProfileIDs)
	{
		for (auto ageRange : GetAgeRanges())
		{
			currSizeByEntityTypeAgeRange[profileID].push_back({ageRange,0});
		}
	}

    //initialize infection trace generator print detailed info about certain ProfileID's
    // in this case, all ProfileID's w/ non-nullptr BucketDemographicProfiles
    DemographicProfile::ProfileID currProfileID = DemographicProfile::MIN;
    while(currProfileID <= DemographicProfile::MAX)
    {
        if((entities->getBucket(currProfileID)))
		{
			// add active sexual activity profiles to infections tracker
            if (DemographicProfile::get(currProfileID, DemographicProfile::Demographic::SexualActivityStatus)
				!= (std::size_t)DemographicProfile::SexualActivityStatus::NotActive)
			{
				//initialize infection trace generator print detailed info about certain ProfileID's
				GetPopulationStatistics().infectionsTracker.addToDetailedTrace(currProfileID);
			}
		}

        currProfileID++;
    }
    //initialize infection trace generator
    GetPopulationStatistics().infectionsTracker.Initialize();

    //initialize structures that hold people who can initiate and 'agree' to relationships.
    InitPartnershipBuckets();

    if(parameters_.useRollout)
    {
        ApplyRolloutContext(parameters_, Time::Zero);
    }
    GetPopulationStatistics().artTracker.SetAgeRanges(popWideParams.GetAgeRanges());

	GenerateInitialEntities();

    //count the size of the population and store value
    UpdateSize();
}

} // namespace transm
