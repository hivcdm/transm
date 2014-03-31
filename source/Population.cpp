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
#include "entities/Female.h"
#include "entities/Male.h"
#include "entities/behaviors/SexualBehaviorParams.h"
#include "statistics/InfectionsTracker.h"
#include "statistics/CostsTracker.h"
#include "util/Util.h"
#include "util/rand/RandomNums.h"

/***
Data needed :
events stratified by age and CD4
***/
unsigned int Population::idCounter = 0;

/**
Creates an initial population of folks
**/
Population::Population(EventParams &_eventParams, const Inputs &inputs, long _maxTime) :
	rankedForTreatment(5),
	populationID(Population::idCounter++)
{
	if(_eventParams.useRollout)
	{
		applyRolloutContext(_eventParams, 0);
	}

	//create the PopStats
	popStats = new PopStats(_maxTime, inputs.GetLifeExpectancyRecordTimes(), 
		inputs.GetLifeExpectancyMedianConfidenceInterval(), inputs.GetPartnerAcquisitionRecordTimes());

	//save population parameters
	popWideParams.init(inputs, populationID, _eventParams);
	//create EntityPool - this will contain all Entities
	entities = new EntityPool(popWideParams.SAEntAgeMths, populationID, popWideParams.assort);
	//initialize infection trace generator print detailed info about certain ProfileID's
	// in this case, all ProfileID's w/ non-nullptr DmgProfileBuckets
	DmgProfile::ProfileID currProfileID = DmgProfile::MIN;

	while(currProfileID <= DmgProfile::MAX)
	{
		//if it is being used in this Population, then append to _profileIDs
		if((entities->getBucket(currProfileID) != nullptr) &&
		        (DmgProfile::get(currProfileID, DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA))
		{
			popStats->infectionsTracker.addToDetailedTrace(currProfileID);
		}

		currProfileID++;
	} //while(currProfileID <= DmgProfile::MAX) {

	//initialize structures that hold people who can initiate and 'agree' to relationships.
	initPartnershipBuckets();
	_eventParams.displayOut("About to create the populations\n");
	/** Create the people in the population **/
	long totalNumMales = Util::round<long>(popWideParams.initSize * popWideParams.proportionMale);
	long totalNumFemales = std::max<long>(popWideParams.initSize - totalNumMales, 0);
	//the params.xml file should have detailed the prevalent characteristics of each age bucket
	//  we will go through each age bucket and create the part of the prevalent population that falls within the bucket
	std::vector<AgeRange> ageRanges;

	for(auto ageBucketParams : popWideParams.initialAgeBuckets)
	{
		auto numMalesInCurrentBucket = Util::round<std::size_t>(totalNumMales * ageBucketParams->proportionOfPopulation[DmgProfile::MALE]);
		auto numFemalesInCurrentBucket = Util::round<std::size_t>(totalNumFemales * ageBucketParams->proportionOfPopulation[DmgProfile::FEMALE]);

		//calc how many people are in the current age range
		auto currentBucketSize = numMalesInCurrentBucket + numFemalesInCurrentBucket;

		//Number of persons of each gender to be traced in detailed output file
		auto numToTrace = static_cast<std::size_t>(_eventParams.numToTrace);

		for(std::size_t count = 0; count < currentBucketSize; count++)
		{
			//Determine whether or not person should be traced in SinglePersonTrace
			bool tracePerson = (count < numToTrace || (count >= numMalesInCurrentBucket && (count - numMalesInCurrentBucket) < numToTrace));

			//create a person, males first and females second
			auto gender = (count < numMalesInCurrentBucket) ? DmgProfile::MALE : DmgProfile::FEMALE;
			auto person = generatePerson(_eventParams, gender, ageBucketParams, tracePerson);

			//add the created person to the EntityPool
			entities->addPersonToAll(person);
		}//end for (int count

		AgeRange ageRange = {ageBucketParams->minAgeMth, ageBucketParams->maxAgeMth};
		AgeRangeSizePair ageRangeSize = std::make_pair(ageRange, currentBucketSize);

		//Add a tuple to the currSizeByAgeRange vector along with the initial size of the age range
		ageRanges.push_back(ageRange);
		currSizeByAgeRange.push_back(ageRangeSize);
		currSizeByAgeRangeMale.push_back(std::make_pair(ageRange, numMalesInCurrentBucket));
		currSizeByAgeRangeFemale.push_back(std::make_pair(ageRange, numFemalesInCurrentBucket));
	}

	popStats->artTracker.SetAgeRanges(ageRanges);

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson])
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << endl << "Now creating initial partnerships... " << endl;
	}

	/** create prevalent Regular Partnerships (time = 0) before creating prevalent marriages **/
	//the demographics that we are pulling the eligibles from
	DmgProfile selector;
	selector.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::SA);
	selector.set(DmgProfile::GENDER, DmgProfile::MALE);
	selector.set(DmgProfile::SEXUAL_ORIENTATION, DmgProfile::HETERO);
	selector.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::SINGLE);
	selector.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);
	std::vector<DmgProfile::ProfileID> bucketIDs;
	//fix code below, i've put placeholders for multiple singles buckets, but right now we only use 1 of each gender
	//errhode: Is this taken care of with the whole agebucket inside SexualMixingBucket thing?
	assert(Constants::TODO_LO_PRI);
	selector.selectProfileIDs(bucketIDs, nullptr);
	DmgProfileBucket *singleMales = entities->getBucket(bucketIDs.at(0));
	selector.set(DmgProfile::GENDER, DmgProfile::FEMALE);
	bucketIDs.clear();
	selector.selectProfileIDs(bucketIDs, nullptr);
	DmgProfileBucket *singleFemales = entities->getBucket(bucketIDs.at(0));
	/** create prevalent formSteadyPartnerships (time = 0) **/
	//the demographics that we are pulling the eligibles from -- same as for regular;
	//can just use the previous singleMales and singleFemales buckets
	//number of couples -- % married of adult population by DmgProfile::SAStatus / 2
	int numCouples = Util::round<long>(popWideParams.initproportionMarried * (singleMales->size() + singleFemales->size()) * 0.5);
	_eventParams.displayOut("Creating ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numCouples).c_str());
	_eventParams.displayOut(" initial steady partnerships\n");

	while(numCouples > 0)
	{
		//break if there are no more people to marry...
		if(!singleFemales->size() || !singleMales->size())
		{
			break;
		}

		//choose a random male from the pool
		Male *m = (Male *)singleMales->drawMember(_eventParams.randomNums, SexualPartnership::Type::Steady, Constants::DONT_REMOVE);

		if(m == nullptr)
		{
			_eventParams.displayOut("Not enough single males\n");
			break;
		}

		//try to form partnership, will add Male back to the pool if partnership was formed
		createPartnerships(_eventParams, m, nullptr, SexualPartnership::Type::Steady, true);
		numCouples--;
	} //while(numCouples > 0) {

	//number of regular couples -- % married of adult population by DmgProfile::SAStatus / 2
	//Note that some people may end up in multiple relationships -- this should come out in the wash (?)
	numCouples = Util::round<int>(popWideParams.initproportionRegular * (singleMales->size() + singleFemales->size()) * 0.5);
	_eventParams.displayOut("Creating ");
	_eventParams.displayOut(boost::lexical_cast<std::string>(numCouples).c_str());
	_eventParams.displayOut(" initial regular partnerships\n");

	while(numCouples > 0)
	{
		//break if there are no more people to pair off...
		//this shouldn't be a problem unless we start with no men or no women as we are not shifting the pairs to non_single status
		if(!singleFemales->size() || !singleMales->size())
		{
			break;
		}

		//choose a random male from the pool
		Male *m = (Male *)singleMales->drawMember(_eventParams.randomNums, SexualPartnership::Type::Regular, Constants::DONT_REMOVE);

		if(m == nullptr)
		{
			_eventParams.displayOut("Not enough single males!\n");
			break;
		}

		//form partnership, will add Male back to the pool if partnership was formed
		createPartnerships(_eventParams, m, nullptr, SexualPartnership::Type::Regular, true);
		numCouples--;
	} //while(numCouples > 0) {

	//count the size of the population and store value
	updateSize();
	_eventParams.displayOut("Population created\n");
	graph = new GraphVizGraphElements();
}

/**
This is a bit hackish and hardcoded
The method determines who are the partnership initiators and who are available to them
//first we determine who can initiate
//second we determine who can be accosted - this differs by partnershipType
**/
void Population::initPartnershipBuckets()
{
	//this is used to select ProfileID's of eligible initiators
	DmgProfile selector;
	selector.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::SA);
	selector.set(DmgProfile::GENDER, DmgProfile::MALE);
	selector.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);
	//all SA, non-CSW males can form partnerships of any type
	vector<DmgProfile::ProfileID> eligibleInitiators;
	selector.selectProfileIDs(eligibleInitiators, nullptr);
	//men can form all types of partnerships
	vector<SexualPartnership::Type> availPartnershipTypes;

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

	DmgProfile currProfileSelector;
	vector<DmgProfile::ProfileID> selectedIDs;

	//iterate through all partnership types. the available Buckets are different by partnership
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		//get the parameters for current relationship type
		const SexualBehaviorParams *partneringParams = popWideParams.maleParams->getSexualBehaviorParams(SexualPartnership::Type(type));

		//get available demographicProfiles that are available for this partnership
		for(unsigned int j = 0; j < partneringParams->getNumAvailableBuckets(); ++j)
		{
			selectedIDs.clear();
			//contains profile ID's that were selected from
			currProfileSelector.set(partneringParams->getAvailableBucket(j).dmgProfileSelector);
			currProfileSelector.selectProfileIDs(selectedIDs, nullptr);
			assert(selectedIDs.size() == 1);	//we don't want any wild cards in the DmgProfile string.
			assert(Constants::TODO_DEF);		//eventually, we should change this.

			//check to see whether we have a repeat Bucket.
			for(size_t i = 0; i < potentialPartnerBuckets[(int)type].size(); ++i)
			{
				if(potentialPartnerBuckets[(int)type].at(i)->getProfileID() == selectedIDs.at(0))
				{
					cerr << "For available buckets for partnership type '" << SexualPartnership::TypeStrings.at(SexualPartnership::Type(type));
					cerr << "', " << *DmgProfile::toString(selectedIDs.at(0)) <<
					     " is listed multiple times either via repeat or wildcard overlaps";
					Util::exitWithPrompt(-1);
				} //if(potentialPartnerBuckets[type].at(i)->getProfileID() == selectedIDs.at(0)) {
			} //for(int i = 0; i < potentialPartnerBuckets[type].size(); ++i) {

			BucketSexualMixing *bucket = (BucketSexualMixing *)entities->getBucket(selectedIDs.at(0));

			if(bucket == nullptr)
			{
				cerr << "This Demographic Profile " << *DmgProfile::toString(selectedIDs.at(0)) <<
				     " has not been instantiated and so cannot be used" << endl;
				Util::exitWithPrompt(-1);
			}
			else
			{
				potentialPartnerBuckets[(int)type].push_back(bucket);
				eligibleBucketWeights[(int)type].push_back(partneringParams->getAvailableBucket(j).weight);
			}
		}

		//make sure that the weights sum to 1
		Util::normalize(eligibleBucketWeights[(int)type]);
	} //for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type) {
}

Population::~Population(void)
{
	//delete the EntityPool container
	delete entities;
	//delete popStats -- but after they get passed on to the TransmissionSummaryStats
	delete popStats;
	delete Male::getPopParams(populationID);
	delete Female::getPopParams(populationID);
	delete graph;
}

//-----------------< Event-related methods -----------------------------//


void Population::births(EventParams &_eventParams)
{
	//number of people to be born this month
	unsigned long numBorn = Util::round<unsigned long>(currSize * popWideParams.birthRate);
	unsigned long numMales = static_cast<unsigned long>(popWideParams.proportionMale * numBorn);
	DmgProfile::Gender gender;
	Person *p = nullptr;

	//create currSize * birthRate New people
	for(unsigned long i = 0; i < numBorn; ++i)
	{
		//determine gender
		gender = (i < numMales) ? DmgProfile::MALE : DmgProfile::FEMALE;
		bool toTrace = _eventParams.currTime >= _eventParams.monthTraceNewborns ? _eventParams.numNewbornsTraced <
		               _eventParams.numNewbornsToTrace : false;

		if(toTrace)
		{
			_eventParams.numNewbornsTraced++;
		}

		p = generatePerson(_eventParams, gender, nullptr, toTrace);

		if(_eventParams.debugLevel > DEBUG1 && _eventParams.outputTrace[EventParams::TraceFileType::Events])
		{
			p->print(_eventParams.traceStreams[EventParams::TraceFileType::Events], Constants::TABTAB);
		}

		//add the newborn to the EntityPool
		//Use addPersonToAll here (initial entrance into population)
		entities->addPersonToAll(p);
	}

	if(_eventParams.debugLevel > DEBUG0)
	{
		printMethodResults(_eventParams, "Births", "People Born", numBorn, "total born", Constants::SHOW_INFECTED);
	}
}

//Updates the age buckets for use with life expectancy
void Population::updateAgeBucketsLE()
{
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
	for(int gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++)
	{
		DmgProfile::Gender gender = DmgProfile::MALE;

		if(gend == DmgProfile::FEMALE)
		{
			gender = DmgProfile::FEMALE;
		}

		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Person *p = (*p_Iter);
			assert(p != nullptr);
			//calculate life expectancy
			assert((popStats->selectedLEStats != nullptr));
			assert(p->getAge(YEAR) >= 0);
			assert(p->getAge(YEAR) < Person::maxYrForDeathStats);
			popStats->selectedLEStats->popByAge[p->getAge(YEAR)]++;
			p_Iter++;
		}//while (p_Iter != entities->end(gender))
	}//	for (DmgProfile::Gender gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++){
}

/**
update age (and SAStatus b/c SAStatus depends on age), health,
**/
void Population::updatePhysicalState(EventParams &_eventParams, bool calculateLE, bool newLEPeriod)
{
	int totalDied = 0;		//keeps track of deaths this timestep
	//holds a pointer to the current bucket we are looking at
	DmgProfileBucket *currBucket = nullptr;
	//helps us iterate through all DemographicProfileBuckets
	DmgProfile::ProfileID currProfileID = DmgProfile::MIN;
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;

	//Reset the class to calculate LE
	if(newLEPeriod)
	{
		popStats->selectedLEStats = new PopStats::SingleLEStats;
	}

	//check if there is another bucket of entities to check
	//if there is a bucket, then iterate through people in bucket
	while(currProfileID <= DmgProfile::MAX)
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
			if((*p_Iter)->rollForDeath(_eventParams.randomNums))
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
	for(DmgProfile::Gender gender = DmgProfile::MALE; gender != DmgProfile::ENDGender; ++gender)
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
				processDeath(_eventParams, p, calculateLE);

				if(_eventParams.useRollout)
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
			p->updateHealthStatus(_eventParams, &popStats->artTracker, &popStats->costsTracker);

			if(oldStatus != p->hivStatus)
			{
				if(p->getDmgProfile()->get(p->getDmgProfile()->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA)
				{
					((BucketSexualMixing *) entities->getBucket(p->getDmgProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
					        p->hivStatus);
				}
			}

			//see whether this person has died.
			//if this person was a couple, then will push living members to personsToAdd
			// to be reinserted into the EntityPool once we have iterated through all buckets
			if(p->rollForDeath(_eventParams.randomNums))
			{
				bool wasProcessed = false;
				p_Iter = entities->removePersonFromAll(p_Iter);
				wasProcessed = true;
				processDeath(_eventParams, p, calculateLE);
				totalDied++;

				if(_eventParams.useRollout)
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
			if((p->getDmgProfileVal(DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::SA)
			        && (p->getAge(MONTH) >= popWideParams.SAEntAgeMths))
			{
				// set them as SA and potentially CSWs
				if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && p->trace())
				{
					if(p->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
					{
						_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Male ";
					}
					else
					{
						_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Female ";
					}

					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << p->getID() << " becomes sexually active" << endl;
				}//if (_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && p->trace())

				Person::HIVStatus oldStatus = p->hivStatus;
				p->becomeSexuallyActive(_eventParams);

				if(oldStatus != p->hivStatus)
				{
					if(p->getDmgProfile()->get(p->getDmgProfile()->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA)
					{
						((BucketSexualMixing *) entities->getBucket(p->getDmgProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
						        p->hivStatus);
					}
				}

				if(p->getAge(MONTH) < popWideParams.CSWEndAgeMth[p->getDmgProfileVal(DmgProfile::GENDER)])
				{
					p->rollForBecomeSexWorker(_eventParams, false);
				}

				Person::RiskLevel oldRisk = p->getRiskLevel();
				//reroll risk group
				p->rerollRiskGroup(_eventParams);
				//refresh risk group in dmg bucket and refresh DmgProfileBucket
				entities->refreshDmgProfileBucket(p, &p_Iter, oldRisk != p->getRiskLevel());
			}

			//Check for age to stop becoming CSW
			if(p->getDmgProfileVal(DmgProfile::EMPLOYMENT) == DmgProfile::CSW
			        && p->getAge(MONTH) >= popWideParams.CSWEndAgeMth[p->getDmgProfileVal(DmgProfile::GENDER)])
			{
				p->quitSexWork(_eventParams);
				entities->refreshDmgProfileBucket(p, &p_Iter);
			}
            
			if(_eventParams.useRollout && _eventParams.treatedContext && p->isInfected())
			{
				if(p->isOnArt())
				{
					popStats->recordTreatmentEligiblity(p); // if they're on treatment, they should be counted as eligible even if the treatment has worked
					popStats->recordTreatment(p);
				}
				else if(p->isEligibleForTreatment(_eventParams.treatedContext->getTreatmentInputs()->startART[0]))
				{
					popStats->recordTreatmentEligiblity(p);
				}
			}

			popStats->costsTracker.RecordLifeMonth(p->getQualityOfLife(), p->getCepacDiscountFactor(), p->getHIVStatus());

			p_Iter++;
		}//while (p_Iter != entities->end(gender))
	}//	for (DmgProfile::Gender gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++){

	if(_eventParams.debugLevel > DEBUG0)
	{
		printMethodResults(_eventParams, "UpdatePhysicalState", "People Died", totalDied, "total died", Constants::SHOW_INFECTED);
	}
}

/*  who initiates flings? it seems that males do for now

This method is mostly designed for speed as this takes up the bulk of processing
We hopefully only iterate through each initiator once.
*/
void Population::updatePartnerships(EventParams &_eventParams)
{
	//holds the tallies for any New partnerships that were made and ended this month
	int newPartnershipCount[(int)SexualPartnership::Type::ENDType];
	//Number of attemptedPartnerships may be higher than the actual partnerships formed if there weren't enough females/males tried to repartner with current partners
	int attemptedPartnershipCount[(int)SexualPartnership::Type::ENDType];
	int	endedPartnershipCount[(int)SexualPartnership::Type::ENDType];

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
	for(p_Iter = entities->begin(DmgProfile::MALE); p_Iter != entities->end(DmgProfile::MALE); p_Iter++)
	{
		Person *person = (*p_Iter);
		Male *initiator = (Male *)person; //We're dealing with this dude
		(*p_Iter)->resetNumActs();
		std::list<SexualPartnership *> partnershipsToEnd;	//list of all partnerships due to end

		//Decide who needs to split up
		for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
		{
			//get partnerships of 'type' whose durations have elapsed, i.e. time to split
			endedPartnershipCount[type] += initiator->getPartnershipsToEnd(_eventParams.currTime, SexualPartnership::Type(type), partnershipsToEnd, false);
		}//foreach SexualPartnership::type

		//Now, split them up... man, it would suck for their kids (if they had any)
		dissolveSexualPartnerships(_eventParams, initiator, partnershipsToEnd);

		//if this initiator is now single, then make sure they are in singles pool
		if(!initiator->inCorrectDmgProfileBucket())
		{
			entities->refreshDmgProfileBucket(initiator, &p_Iter);
		}
	} //for (p_Iter = entities->begin(DmgProfile::MALE); p_Iter != entities->end(DmgProfile::MALE); p_Iter++) {

	//Ending the first pass (dissolving partnerships)

	//reset num acts for females
	for(p_Iter = entities->begin(DmgProfile::FEMALE); p_Iter != entities->end(DmgProfile::FEMALE); p_Iter++)
	{
		(*p_Iter)->resetNumActs();
	}

	//Second pass: Form new partnerships and have sex
	for(p_Iter = entities->begin(DmgProfile::MALE); p_Iter != entities->end(DmgProfile::MALE); p_Iter++)
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
			int numFormed = createPartnerships(_eventParams, initiator, &p_Iter, type);
			//if(numFormed > 0) {
			newPartnershipCount[(int)type] += numFormed;
			//} //if(numFormed > 0) {
			attemptedPartnershipCount[(int)type] += numFormed + initiator->getLatestUnformedPartnerships(type);
		} //for(int i =0; i < bucketIter->second.size(); i ++) {

		//for existing partnerships, have sexual activity
		//Have all the sexual activity with current partners (includes new partners)
		for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
		{
			std::list<Person *> newlyInfected;
			//sexual activity among any existing partnerships that have a duration associated with them
			Person *infectedMe = initiator->allPartnerSexualActivity(_eventParams, SexualPartnership::Type(type), newlyInfected,
			                     &(popStats->infectionsTracker));
			//TODO: Get a condom use count here!
			//record all incident infections
			std::list<Person *>::iterator newlyInfectedIter = newlyInfected.begin();

			while(newlyInfectedIter != newlyInfected.end())
			{
				Person *wasUninfected = *newlyInfectedIter;

				if(wasUninfected->getDmgProfile()->get(wasUninfected->getDmgProfile()->getProfileID(),
				                                       DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA)
				{
					((BucketSexualMixing *) entities->getBucket(wasUninfected->getDmgProfile()->getProfileID()))->changeHIVStatus(
					    wasUninfected, Person::NEGATIVE, wasUninfected->hivStatus);
				}

				((BucketSexualMixing *) entities->getBucket(wasUninfected->getDmgProfile()->getProfileID()))->increaseInfected(
				    wasUninfected);
				//initiator only gets infected once...
				Person *wasInfected = (*newlyInfectedIter == initiator) ? infectedMe : initiator;

				//Adds person to the untreated pool if using rollout
				if(_eventParams.useRollout)
				{
					rolloutUntreatedPool.push_back(wasUninfected);
				}

				if(_eventParams.outputTrace[EventParams::TraceFileType::Events])
				{
					popStats->recordIncidentInfection(_eventParams, _eventParams.currTime,
						SexualPartnership::Type(type),
					                                        wasInfected,
					                                        wasUninfected,
					                                        (_eventParams.debugLevel > DEBUG1),
					                                        _eventParams.traceStreams[EventParams::TraceFileType::Events]);
				}

				newlyInfectedIter++;
			}//while(newlyInfectedIter != newlyInfected.end())
		} //for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type) {

		//Add the cost of condom usage
		auto totalCondomCostUndiscounted = initiator->getCondomsUsedThisMonth() * popWideParams.condomCost;
		popStats->costsTracker.RecordCondomUse(totalCondomCostUndiscounted, totalCondomCostUndiscounted * initiator->getCepacDiscountFactor());
	} //for (p_Iter = entities->begin(DmgProfile::MALE); p_Iter != entities->end(DmgProfile::MALE); p_Iter++)

	//Ends the second pass through (i.e. the sex acts pass through)

	//print out results to traces
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		if(_eventParams.debugLevel > DEBUG0)
		{
			//print out how many partnerships were attempted
			std::ostringstream attemptedLabel;
			attemptedLabel << SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)) << " Attempted";
			printMethodResults(_eventParams, "updatePartnerships(...)", attemptedLabel.str(), attemptedPartnershipCount[type],
				"Attempted" + SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)), Constants::SHOW_INFECTED);
			//print out how many partnerships were formed
			std::ostringstream formedLabel;
			formedLabel << SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)) << " Formed";
			printMethodResults(_eventParams, "updatePartnerships(...)", formedLabel.str(), newPartnershipCount[type],
				"New " + SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)), Constants::SHOW_INFECTED);

			//if these partnerships have a duration beyond the month, print out how many were ended
			if(popWideParams.partnershipsHaveDuration[DmgProfile::MALE][type])
			{
				std::ostringstream endedLabel;
				endedLabel << SexualPartnership::TypeStrings.at(SexualPartnership::Type(type)) << " Ended";
				printMethodResults(_eventParams, "updatePartnerships(...)", endedLabel.str(), endedPartnershipCount[type],
				                         "Number Ended", Constants::SHOW_INFECTED);
			}
		}
	} //for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type) {

	if(_eventParams.calibrationInputs.useCalibration
	        && _eventParams.currTime > (_eventParams.calibrationInputs.monthOfCalibration - 12)
	        && _eventParams.currTime <= _eventParams.calibrationInputs.monthOfCalibration)
	{
		//update concurrency status
		for(int gender = DmgProfile::MALE; gender < DmgProfile::ENDGender; gender++)
		{
			std::list<Person *>::iterator p_Iter = entities->begin((DmgProfile::Gender) gender);

			while(p_Iter != entities->end((DmgProfile::Gender) gender))
			{
				//tally concurrent partners
				//create a number between 0 and 15 representing the combination of partnership types person has
				//e.g. if person has partnerships steady and casual concurrent will equal 8+2=10
				int concurrent = 0;
				int numPartners[(int)SexualPartnership::Type::ENDType];
				int totalNumPartners = 0;

				for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
				{
					numPartners[i] = (*p_Iter)->getNumPartners((SexualPartnership::Type) i);
					concurrent = (concurrent << 1) + (numPartners[i] != 0 ? 1 : 0);
					totalNumPartners += numPartners[i];
				}

				concurrent = 15 - concurrent;
				assert(concurrent <= Constants::NUMBER_CONCURRENCY_DEFS);

				if(_eventParams.concurrencyDef[concurrent]->useDefinition
				        && totalNumPartners >= _eventParams.concurrencyDef[concurrent]->minPartnershipsNeeded)
				{
					//count as concurrent partnership
					(*p_Iter)->setMonthOfLatestConcurrent(_eventParams.currTime);
				}

				p_Iter++;
			}
		}
	}
}

/**
Iterates through current entities in the population and returns a total number of people
***/
long Population::updateSize()
{
	currSize = entities->size();
	//Also update size of non-sexually active
	currNASize = entities->sizeNotSexuallyActive();

	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		currNASizeByGender[i] = entities->sizeNotSexuallyActive((DmgProfile::Gender) i);

		for(int j = 0; j < Person::ENDRiskLevel; j++)
		{
			currSASizeGenderRisk[i][j] = entities->sizeSexuallyActive((DmgProfile::Gender) i, (Person::RiskLevel) j);
		}
	}

	//Size by gender
	DmgProfile GenderProfile;
	//First tally the men
	GenderProfile.set(DmgProfile::GENDER, DmgProfile::MALE);
	vector<DmgProfile::ProfileID> GenderProfileIDs;
	currSizeGender[DmgProfile::MALE] = 0;
	GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);

	for(size_t i = 0; i < GenderProfileIDs.size(); i++)
	{
		currSizeGender[DmgProfile::MALE] += entities->size(GenderProfileIDs[i]);
	}

	//Next tally the women
	GenderProfile.set(DmgProfile::GENDER, DmgProfile::FEMALE);
	GenderProfileIDs.clear();
	currSizeGender[DmgProfile::FEMALE] = 0;
	GenderProfile.selectProfileIDs(GenderProfileIDs, nullptr);

	for(size_t i = 0; i < GenderProfileIDs.size(); i++)
	{
		currSizeGender[DmgProfile::FEMALE] += entities->size(GenderProfileIDs[i]);
	}

	//Count all the CSW's
	DmgProfile CSWProfile;
	CSWProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::CSW);
	vector<DmgProfile::ProfileID> CSWProfileIDs;
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

		for(int j = 0; j < DmgProfile::ENDGender; j++)
		{
			currSizeGenderRiskCSW[j][risk] = 0;
		}

		//loop through all buckets
		DmgProfileBucket *currBucket = nullptr;
		DmgProfile::ProfileID currProfileID = DmgProfile::MIN;

		//iterate through all buckets
		while(currProfileID <= DmgProfile::MAX)
		{
			currBucket = entities->getBucket(currProfileID);

			//if people of this particular profile don't exist in the population, move on.
			if((currBucket == nullptr) || (currBucket->size() == 0))
			{
				currProfileID++;
				continue;
			}

			if(DmgProfile::get(currBucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA)
			{
				//Add all people in bucket of particular risk group
				currSizeRisk[risk] += ((BucketSexualMixing *)(currBucket))->sizeRisk((Person::RiskLevel) risk);

				if(DmgProfile::get(currBucket->getProfileID(), DmgProfile::EMPLOYMENT) == DmgProfile::CSW)
				{
					currSizeRiskCSW[risk] += ((BucketSexualMixing *)(currBucket))->sizeRiskCSW((Person::RiskLevel)risk);
					currSizeGenderRiskCSW[DmgProfile::get(currBucket->getProfileID(),
					                            DmgProfile::GENDER)][risk] += ((BucketSexualMixing *)(currBucket))->sizeRiskCSW((Person::RiskLevel)risk);
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
		ageRangeSizePair.second = entities->sizeSexuallyActiveByAge(ageRangeSizePair.first.lower, ageRangeSizePair.first.upper, DmgProfile::MALE);
	}

	for(auto &ageRangeSizePair : currSizeByAgeRangeFemale)
	{
		ageRangeSizePair.second = entities->sizeSexuallyActiveByAge(ageRangeSizePair.first.lower, ageRangeSizePair.first.upper, DmgProfile::FEMALE);
	}

	return currSize;
}

//Resets Monthly Population Statistics
void Population::resetMonthlyStats()
{
	//reset curr month death stats
	for(int i = 0; i < Person::ENDDeathStatus; i++)
	{
		currDeathCauses[i] = 0;
	}
}

//After all of the population dynamics have run through, run all of the remaining infected persons through CEPAC until they die to get the life expectancy and such in the CEPAC output files.
void Population::updateFinalPhysicalState(EventParams &_eventParams)
{
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
	for(int gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++)
	{
		DmgProfile::Gender gender = DmgProfile::MALE;

		if(gend == DmgProfile::FEMALE)
		{
			gender = DmgProfile::FEMALE;
		}

		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Person *p = (*p_Iter);
			assert(p != nullptr);

			if(p->isAlive())
			{
				//Set their death in the graph node to be the end of time (i.e. now)
				p->getPersonNode()->timeDied = _eventParams.currTime + 1;
				//Add this person to the graph
				graph->persons.push_back(p->getPersonNode());
			}

			//Removing entity because the age is about to change and this will fuck up being able to find the person!
			entities->removeEntity(p);
			p->runCEPACtoDeath(_eventParams.randomNums);
			popStats->processPostMaxTimeDeath(p);
			//Putting them back after age is updated
			entities->addEntity(p);
			p_Iter++;
		}//while (p_Iter != entities->end(gender))
	}//	for (DmgProfile::Gender gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++){
}


//--------------------------< BEGIN helper methods  >-------------------------------------//

void Population::dissolveSexualPartnerships(EventParams &_eventParams, Person *_initiator,
        std::list<SexualPartnership *> &_partnershipsToEnd)
{
	bool initiatorMale = (_initiator->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE);
	//iterate through each partner list
	std::list<SexualPartnership *>::iterator partnerIter = _partnershipsToEnd.begin();

	while(partnerIter != _partnershipsToEnd.end())
	{
		Person *partner = (*partnerIter)->getOtherPartner(_initiator);

		//print the couple that is getting divorced
		if(_eventParams.debugLevel > DEBUG1 && _eventParams.outputTrace[EventParams::TraceFileType::Events])
		{
			(*partnerIter)->printPartners(_eventParams.traceStreams[EventParams::TraceFileType::Events],
			                              "This couple is splitting up: " + Constants::TABTAB);
		}

		if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (_initiator->trace() || partner->trace()))
		{
			if(_initiator->trace())
			{
				if(initiatorMale)
				{
					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "- Male ";
				}
				else
				{
					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "- Female ";
				}

				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << _initiator->getID() 
					<< " ends " << (SexualPartnership::TypeStrings.at((*partnerIter)->getType()));
				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " partnership with " << partner->getID();
			}
			else
			{
				if(initiatorMale)
				{
					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "- Female ";
				}
				else
				{
					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "- Male ";
				}

				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << partner->getID() << " ends " << (SexualPartnership::TypeStrings.at((*partnerIter)->getType()));
				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " partnership with " << _initiator->getID();
			}

			if(!_initiator->isAlive())
			{
				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " -> " << _initiator->getID() << " has died";
			}

			if(!partner->isAlive())
			{
				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " -> " << partner->getID() << " has died";
			}

			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << endl;
		}//if (_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (_initiator->trace() || partner->trace()))

		//if the partnership has any duration, destructor removes the pointer from both members partner lists
		delete(*partnerIter);

		//refresh DmgProfileBucket placement if necessary
		if(!partner->inCorrectDmgProfileBucket())
		{
			//try {
			/* Code that can throw */
			entities->refreshDmgProfileBucket(partner, nullptr);
			/*}
			catch (std::out_of_range& e) {
			std::cout << "Out of range: " << e.what() << "\n";
			}
			catch (std::exception& e) {
			std::cout << "Some other exception: " << e.what() << "\n";
			}*/
		}

		partnerIter++;
	}
}

Person *Population::generatePerson(EventParams &_eventParams, DmgProfile::Gender _gender,
                                   PopulationParams::AgeBucketPrevalenceInfo *_ageBucketParams, bool toTrace)
{
	assert(_gender < DmgProfile::ENDGender);
	Person *toReturn = nullptr;	//pointer to the person that was just generated
	//determine age of current person. If we have no age _ageBucketParams, then this is a newborn.
	//Otherwise, generate an age from a uniform distribution bounded by _ageBucketParams
	int ageMth = (_ageBucketParams == nullptr) ? 0 : _eventParams.randomNums.randInt(_ageBucketParams->minAgeMth,
	             _ageBucketParams->maxAgeMth);

	//create the person
	if(_gender == DmgProfile::MALE)
	{
		toReturn = new Male(_eventParams, ageMth, _eventParams.randomNums.chance(popWideParams.circumcised),
		                    populationID);
	}
	else
	{
		toReturn = new Female(_eventParams, ageMth, populationID);
	}

	//If it was a boy and he was circumcised, add the costs
	if(toReturn->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
	{
		Male *m = (Male *)toReturn;

		if(m->isCircumcised())
		{
			popStats->costsTracker.RecordCircumcision(popWideParams.circumcisionCost, popWideParams.circumcisionCost * m->getCepacDiscountFactor());
		}
	}

	//Set this person to be trace if toTrace is true
	if(toTrace)
	{
		toReturn->setToBeTraced();
	}

	//if person is of sexually active age, roll and see if they are a CSW
	//DO NOT SET THEM AS SEXUALLY ACTIVE UNTIL AFTER DETERMINING IF THEY ARE A PREVALENT CASE BECAUSE THE CEPAC PERSON IS CREATED HERE!
	if(ageMth >= popWideParams.SAEntAgeMths)
	{
		//see if they will be a CSW
		if(ageMth < popWideParams.CSWEndAgeMth[_gender])
		{
			toReturn->rollForBecomeSexWorker(_eventParams, true, popWideParams.initProbCSW[_gender]);
		}

		//reroll their risk group
		toReturn->rerollRiskGroup(_eventParams);
		//If they are of age, set them to be sexually active here: this is where toReturn->cepacPerson is initialized for non-prevalent cases
		Person::HIVStatus oldStatus = toReturn->hivStatus;
		toReturn->becomeSexuallyActive(_eventParams);

		if(oldStatus != toReturn->hivStatus)
		{
			if(toReturn->getDmgProfile()->get(toReturn->getDmgProfile()->getProfileID(),
			                                  DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA)
			{
				((BucketSexualMixing *) entities->getBucket(toReturn->getDmgProfile()->getProfileID()))->changeHIVStatus(toReturn,
				        oldStatus, toReturn->hivStatus);
			}
		}
	}

	if(toTrace)
	{
		if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson])
		{
			toReturn->print(_eventParams.traceStreams[EventParams::TraceFileType::Singleperson], "Tracing the following patient: ");
		}
	}

	return toReturn;
}
/*
* Resets the counter for incidient infections by age for infectionstracker
*/
void Population::initIncidentInfectionsByAge()
{
	AgeRangeSizeContainer incidentInfsAgeMale, incidentInfsAgeFemale, totalIncidentInfsAge;

	for(auto ageBucketParams : popWideParams.initialAgeBuckets)
	{
		AgeRange ageRange = {ageBucketParams->minAgeMth, ageBucketParams->maxAgeMth};
		incidentInfsAgeMale.push_back({ageRange, 0});
		incidentInfsAgeFemale.push_back({ageRange, 0});
		totalIncidentInfsAge.push_back({ageRange, 0});
	}

	popStats->infectionsTracker.initializeIncidentInfectionsByAge(incidentInfsAgeMale, incidentInfsAgeFemale, totalIncidentInfsAge);
}
void Population::applyIncidentPrevalence(EventParams &_eventParams)
{
	_eventParams.displayOut("Applying incident prevalence data\n");
	//counter for number of people in each age bucket who are infected (used to initialize prevalence) (CSW, High risk, Low risk)
	std::vector<std::array<int, 3>> numInfectedByAgeBucketMale(popWideParams.initialAgeBuckets.size());
	std::vector<std::array<int, 3>> numInfectedByAgeBucketFemale(popWideParams.initialAgeBuckets.size());

	//loop through all males and apply prevalence to population
	for(std::list<Person *>::iterator males_iter = entities->begin(DmgProfile::MALE);
	        males_iter != entities->end(DmgProfile::MALE); males_iter++)
	{
		Person *p = *(males_iter);
		int ageBucketIndex = getAgeBucketIndex(p);
		auto _ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucketIndex);
		DmgProfile::Gender _gender = DmgProfile::MALE;
		//if this is a prevalent person, see if they're infected. Right now, newborns cannot be infected
		//TODO: Have counter in ageBucketParams for persons infected

		if(_ageBucketParams)
		{
			bool isCSW = p->getDmgProfileVal(DmgProfile::EMPLOYMENT) == DmgProfile::CSW;
			Person::RiskLevel risk = p->getRiskLevel();
			//apply prevalence if we have not yet reached the quoto of infected people for that bucket
			bool isPrevalent = false;

			if(isCSW)
			{
				if(numInfectedByAgeBucketMale.at(ageBucketIndex)[0] < _ageBucketParams->numInfectedCSW[_gender])
				{
					isPrevalent = true;
					numInfectedByAgeBucketMale.at(ageBucketIndex)[0]++;
				}
			}
			else
			{
				if(risk == Person::HIGH)
				{
					if(numInfectedByAgeBucketMale.at(ageBucketIndex)[1] < _ageBucketParams->numInfectedRisk[_gender][risk])
					{
						isPrevalent = true;
						numInfectedByAgeBucketMale.at(ageBucketIndex)[1]++;
					}
				}
				else
				{
					if(numInfectedByAgeBucketMale.at(ageBucketIndex)[2] < _ageBucketParams->numInfectedRisk[_gender][risk])
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

				if(_eventParams.tracePrevalentCases)
				{
					p->setToBeTraced();
				}

				p->becomeInfected(Constants::PREVALENT_INFECTION, _eventParams);

				if(oldStatus != p->hivStatus)
				{
					((BucketSexualMixing *) entities->getBucket(p->getDmgProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
					        p->hivStatus);
				}

				//Adds person to the untreated pool if using rollout
				if(_eventParams.useRollout)
				{
					rolloutUntreatedPool.push_back(p);
				}

				if(p->getDmgProfile()->get(p->getDmgProfile()->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA)
				{
					((BucketSexualMixing *)entities->getBucket(p->getDmgProfile()->getProfileID()))->increaseInfected(p);
				}
			}
		} //if(_ageBucketParams) {
	}//for()

	//loop through all females and apply prevalence to population
	for(std::list<Person *>::iterator females_iter = entities->begin(DmgProfile::FEMALE);
	        females_iter != entities->end(DmgProfile::FEMALE); females_iter++)
	{
		Person *p = *(females_iter);
		int ageBucketIndex = getAgeBucketIndex(p);
		PopulationParams::AgeBucketPrevalenceInfo *_ageBucketParams = popWideParams.initialAgeBuckets.at(
		            ageBucketIndex);
		DmgProfile::Gender _gender = DmgProfile::FEMALE;
		//if this is a prevalent person, see if they're infected. Right now, newborns cannot be infected
		//TODO: Have counter in ageBucketParams for persons infected

		if(_ageBucketParams)
		{
			bool isCSW = p->getDmgProfileVal(DmgProfile::EMPLOYMENT) == DmgProfile::CSW;
			Person::RiskLevel risk = p->getRiskLevel();
			bool isPrevalent = false;

			if(isCSW)
			{
				if(numInfectedByAgeBucketFemale.at(ageBucketIndex)[0] < _ageBucketParams->numInfectedCSW[_gender])
				{
					isPrevalent = true;
					numInfectedByAgeBucketFemale.at(ageBucketIndex)[0]++;
				}
			}
			else
			{
				if(risk == Person::HIGH)
				{
					if(numInfectedByAgeBucketFemale.at(ageBucketIndex)[1] < _ageBucketParams->numInfectedRisk[_gender][risk])
					{
						isPrevalent = true;
						numInfectedByAgeBucketFemale.at(ageBucketIndex)[1]++;
					}
				}
				else
				{
					if(numInfectedByAgeBucketFemale.at(ageBucketIndex)[2] < _ageBucketParams->numInfectedRisk[_gender][risk])
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

				if(_eventParams.tracePrevalentCases)
				{
					p->setToBeTraced();
				}

				p->becomeInfected(Constants::PREVALENT_INFECTION, _eventParams);

				if(oldStatus != p->hivStatus)
				{
					((BucketSexualMixing *) entities->getBucket(p->getDmgProfile()->getProfileID()))->changeHIVStatus(p, oldStatus,
					        p->hivStatus);
				}

				if(_eventParams.useRollout)
				{
					rolloutUntreatedPool.push_back(p);
				}

				if(p->getDmgProfile()->get(p->getDmgProfile()->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::NA)
				{
					((BucketSexualMixing *)entities->getBucket(p->getDmgProfile()->getProfileID()))->increaseInfected(p);
				}
			}
		} //if(_ageBucketParams) {
	}//for()
}

/**
* Sets the untreated and treated cepac files if using ART Rollout
*/
void Population::applyRolloutContext(EventParams &_eventParams, int time)
{
	for(vector <EventParams::RolloutContext *>::iterator rolloutContextIter = _eventParams.rolloutSimContexts.begin();
	        rolloutContextIter != _eventParams.rolloutSimContexts.end(); rolloutContextIter++)
	{
		EventParams::RolloutContext *rolloutContext = *rolloutContextIter;

		if(rolloutContext->timeToApply == time)
		{
			//Switch the cepac file depending on the population the new file is applied to
			switch(rolloutContext->popOfInterest)
			{
			case 0: //All Untreated
				_eventParams.untreatedContext = rolloutContext->rolloutSimContext;
				break;

			case 1:  //All Treated
			{
				_eventParams.treatedContext = rolloutContext->rolloutSimContext;
				//Apply to all current treated patients
				std::list<Person *>::iterator personIter;

				for(personIter = rolloutTreatedPool.begin(); personIter != rolloutTreatedPool.end(); personIter++)
				{
					(*personIter)->setSimContext(_eventParams.treatedContext);
				}

				break;
			}

			case 2: //Untreated Getting new art
				_eventParams.treatedContext = rolloutContext->rolloutSimContext;
				break;

			default:
				break;
			}
		}
	}
}

void Population::determineRankings(const EventParams::RolloutEligibility &criteria)
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
					if(untPerson->cd4 >= criteria.cd4Bounds[Constants::LOWER] && untPerson->cd4 <= criteria.cd4Bounds[Constants::UPPER])
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

					if(untPerson->cd4 >= criteria.cd4OiHistCd4Bounds[Constants::LOWER]
					        && untPerson->cd4 <= criteria.cd4OiHistCd4Bounds[Constants::UPPER] && numMatchingOIs >= 1)
					{
						isEligible = true;
					}
				}

				if(checkHvl)
				{
					Person::HVLStrata currHvl = untPerson->currentTrueHvl;

					if((int)currHvl >= criteria.hvlBounds[Constants::LOWER] && (int)currHvl <= criteria.hvlBounds[Constants::UPPER])
					{
						isEligible = true;
					}
				}

				if(checkCd4Hvl)
				{
					Person::HVLStrata currHvl = untPerson->currentTrueHvl;
					double currCd4 = untPerson->cd4;

					if(currCd4 >= criteria.cd4HvlCd4Bounds[Constants::LOWER] && currCd4 <= criteria.cd4HvlCd4Bounds[Constants::UPPER]
					        && currHvl >= criteria.cd4HvlHvlBounds[Constants::LOWER] && currHvl <= criteria.cd4HvlHvlBounds[Constants::UPPER])
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

void Population::startTreatment(Person *person, SimContext *treatedContext)
{
	std::list<Person *>::iterator untreatedIterator;
	untreatedIterator = std::find(rolloutUntreatedPool.begin(), rolloutUntreatedPool.end(), person);
	assert(untreatedIterator != rolloutUntreatedPool.end());
	rolloutUntreatedPool.erase(untreatedIterator);
	rolloutTreatedPool.push_back(person);
	person->setSimContext(treatedContext);
}

void Population::applyARTRollout(EventParams &_eventParams)
{
	int numTreated = rolloutTreatedPool.size();
	double currentRolloutProportion = _eventParams.interpolateMonthlyRolloutProportion();
	int totalSlots = static_cast<int>(getSize() * currentRolloutProportion);
	int newSlots = totalSlots - numTreated;

	if(totalSlots > 0)
	{
		determineRankings(_eventParams.rolloutEligibility);
	}

	if(newSlots > 0)
	{
		for(auto &current_ranking_bucket : rankedForTreatment)
		{
			while(newSlots > 0 && !current_ranking_bucket.empty())
			{
				int randomPersonIndex = _eventParams.randomNums.randInt(0, current_ranking_bucket.size() - 1);
				startTreatment(current_ranking_bucket[randomPersonIndex], _eventParams.treatedContext);

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
			popStats->recordTreatmentAccessEligiblity(person);
		}
	}

	for(auto &person : rolloutTreatedPool)
	{
		// double counting shouldn't be a problem, they're either in rolloutTreatedPool or rankedForTreatment but not both
		popStats->recordTreatmentAccessEligiblity(person);
		popStats->recordTreatmentAccess(person);
	}
}

void Population::recordShiftedOutcomes(EventParams &_eventParams, std::ostream &_outStream)
{
	if(_eventParams.monthOf1990 <= _eventParams.currTime)
	{
		int year = 1990 + (_eventParams.currTime - _eventParams.monthOf1990) / 12;
		int month = (_eventParams.currTime - _eventParams.monthOf1990) % 12;
		int numTests = _eventParams.cepacRunStats->getHIVScreening()->numAcceptTest;
		std::vector<int> numTestsByResult(SimContext::TEST_RESULT_NUM, 0);

		for(int i = 0; i < SimContext::TEST_RESULT_NUM; i++)
		{
			numTestsByResult[i] += _eventParams.cepacRunStats->getHIVScreening()->numTestResultsPrevalentType[i];
			numTestsByResult[i] += _eventParams.cepacRunStats->getHIVScreening()->numTestResultsIncidentType[i];
			numTestsByResult[i] += _eventParams.cepacRunStats->getHIVScreening()->numTestResultsHIVNegativeType[i];
		}

		if(numTests)
		{
			popStats->recordTestStats(numTests, numTestsByResult);
		}

		if(month == 11)
		{
			popStats->printShiftedOutcomes(_outStream, year);
			popStats->resetYear(year + 1);
		}
	}
}

bool Population::passesPartnershipCalibration(EventParams &_eventParams)
{
	//calculate partnership prevalence values
	unsigned long numInPartnership[(int)SexualPartnership::Type::ENDType][DmgProfile::ENDGender];
	unsigned long numInConcurrent[DmgProfile::ENDGender];
	unsigned long numActsMonth[DmgProfile::ENDGender];
	unsigned long numActsMonthRisk[DmgProfile::ENDGender][Person::ENDRiskLevel];
	unsigned long numSexuallyActive[DmgProfile::ENDGender];
	unsigned long numSexuallyActiveRisk[DmgProfile::ENDGender][Person::ENDRiskLevel];
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
		for(int j = 0; j < DmgProfile::ENDGender; j++)
		{
			numInPartnership[i][j] = 0;
		}
	}

	for(int i = 0; i < DmgProfile::ENDGender; i++)
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

	for(int gender = DmgProfile::MALE; gender < DmgProfile::ENDGender; gender++)
	{
		std::list<Person *>::iterator p_Iter = entities->begin((DmgProfile::Gender) gender);

		while(p_Iter != entities->end((DmgProfile::Gender) gender))
		{
			if((*p_Iter)->getDmgProfileVal(DmgProfile::SEXUAL_ACTIVITY_STATUS) != DmgProfile::SA)
			{
				p_Iter++;
				continue;
			}

			Person::RiskLevel risk = (*p_Iter)->getRiskLevel();
			numSexuallyActive[gender]++;
			numSexuallyActiveRisk[gender][risk]++;
			numActsMonth[gender] += (*p_Iter)->getNumActsThisMonth();
			numActsMonthRisk[gender][risk] += (*p_Iter)->getNumActsThisMonth();

			for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
			{
				if((*p_Iter)->getMonthOfLatestPartnershipDissolution((SexualPartnership::Type) i) > max<int>((
				            _eventParams.currTime - 12), 0))
				{
					numInPartnership[i][gender]++;
				}
			}

			if((*p_Iter)->getMonthOfLatestConcurrent() > max<int>((_eventParams.currTime - 12), 0))
			{
				numInConcurrent[gender]++;
			}

			p_Iter++;
		}
	}

	//check if population meets bounds for partnership prevalance
	int numInSteady, numInCasual, numInCSW, concurrentNum, numActs;
	int maleSA = numSexuallyActive[DmgProfile::MALE];
	int femaleSA = numSexuallyActive[DmgProfile::FEMALE];
	int totalSA = maleSA + femaleSA;
	double steadyPrev, casualPrev, CSWPrev, propInConcurrent, numActsAvg;

	if(_eventParams.calibrationInputs.steadyPrevPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
		numInSteady = numInPartnership[(int)SexualPartnership::Type::Steady][DmgProfile::MALE] +
			numInPartnership[(int)SexualPartnership::Type::Steady][DmgProfile::FEMALE]; //entire SA pop

		if(totalSA != 0)
		{
			steadyPrev = numInSteady / (double)totalSA;

			if(steadyPrev < _eventParams.calibrationInputs.steadyPrevBounds[Constants::LOWER]
			        || steadyPrev > _eventParams.calibrationInputs.steadyPrevBounds[Constants::UPPER])
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
		numInSteady = numInPartnership[(int)SexualPartnership::Type::Steady][DmgProfile::MALE]; //only male SA

		if(maleSA != 0)
		{
			steadyPrev = numInSteady / (double)maleSA;

			if(steadyPrev < _eventParams.calibrationInputs.steadyPrevBounds[Constants::LOWER]
			        || steadyPrev > _eventParams.calibrationInputs.steadyPrevBounds[Constants::UPPER])
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

	if(_eventParams.calibrationInputs.casualPrevPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
		numInCasual = numInPartnership[(int)SexualPartnership::Type::Casual][DmgProfile::MALE] +
			numInPartnership[(int)SexualPartnership::Type::Casual][DmgProfile::FEMALE]; //entire SA pop

		if(totalSA != 0)
		{
			casualPrev = numInCasual / (double)totalSA;

			if(casualPrev < _eventParams.calibrationInputs.casualPrevBounds[Constants::LOWER]
			        || casualPrev > _eventParams.calibrationInputs.casualPrevBounds[Constants::UPPER])
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
		numInCasual = numInPartnership[(int)SexualPartnership::Type::Casual][DmgProfile::MALE]; //only male SA

		if(maleSA != 0)
		{
			casualPrev = numInCasual / (double)maleSA;

			if(casualPrev < _eventParams.calibrationInputs.casualPrevBounds[Constants::LOWER]
			        || casualPrev > _eventParams.calibrationInputs.casualPrevBounds[Constants::UPPER])
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

	if(_eventParams.calibrationInputs.CSWPrevPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
		numInCSW = numInPartnership[(int)SexualPartnership::Type::Csw][DmgProfile::MALE] +
			numInPartnership[(int)SexualPartnership::Type::Csw][DmgProfile::FEMALE]; //entire SA pop

		if(totalSA != 0)
		{
			CSWPrev = numInCSW / (double)totalSA;

			if(CSWPrev < _eventParams.calibrationInputs.CSWPrevBounds[Constants::LOWER]
			        || CSWPrev > _eventParams.calibrationInputs.CSWPrevBounds[Constants::UPPER])
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
		numInCSW = numInPartnership[(int)SexualPartnership::Type::Csw][DmgProfile::MALE]; //only male SA

		if(maleSA != 0)
		{
			CSWPrev = numInCSW / (double)maleSA;

			if(CSWPrev < _eventParams.calibrationInputs.CSWPrevBounds[Constants::LOWER]
			        || CSWPrev > _eventParams.calibrationInputs.CSWPrevBounds[Constants::UPPER])
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

	if(_eventParams.calibrationInputs.propInConcurrentPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
		concurrentNum = numInConcurrent[DmgProfile::MALE] + numInConcurrent[DmgProfile::FEMALE]; //entire SA pop

		if(totalSA != 0)
		{
			propInConcurrent = concurrentNum / (double)totalSA;

			if(propInConcurrent < _eventParams.calibrationInputs.propInConcurrentBounds[Constants::LOWER]
			        || propInConcurrent > _eventParams.calibrationInputs.propInConcurrentBounds[Constants::UPPER])
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
		concurrentNum = numInConcurrent[DmgProfile::MALE]; //only male SA

		if(maleSA != 0)
		{
			propInConcurrent = concurrentNum / (double)maleSA;

			if(propInConcurrent < _eventParams.calibrationInputs.propInConcurrentBounds[Constants::LOWER]
			        || propInConcurrent > _eventParams.calibrationInputs.propInConcurrentBounds[Constants::UPPER])
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

	if(_eventParams.calibrationInputs.numActsPopulation == 0)
	{
		firstRow << "Entire SA Pop" << Constants::TAB;
		numActs = numActsMonth[DmgProfile::MALE] + numActsMonth[DmgProfile::FEMALE]; //entire SA pop

		if(totalSA != 0)
		{
			numActsAvg = numActs / (double)totalSA;

			if(numActsAvg < _eventParams.calibrationInputs.numActsBounds[Constants::LOWER]
			        || numActsAvg > _eventParams.calibrationInputs.numActsBounds[Constants::UPPER])
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
		numActs = numActsMonth[DmgProfile::MALE]; //only male SA

		if(maleSA != 0)
		{
			numActsAvg = numActs / (double)maleSA;

			if(numActsAvg < _eventParams.calibrationInputs.numActsBounds[Constants::LOWER]
			        || numActsAvg > _eventParams.calibrationInputs.numActsBounds[Constants::UPPER])
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
	unsigned long numInCasualMale = numInPartnership[(int)SexualPartnership::Type::Casual][DmgProfile::MALE];
	unsigned long numInCasualFemale = numInPartnership[(int)SexualPartnership::Type::Casual][DmgProfile::FEMALE];
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

		if(casualPrevFemale > (casualPrevMale * _eventParams.calibrationInputs.femaleCasualPrevRatio))
		{
			passesCalib = false;
			passesCasualPartPrevRatio = false;
		}
	}

	unsigned long numInConcurrentMale = numInConcurrent[DmgProfile::MALE];
	unsigned long numInConcurrentFemale = numInConcurrent[DmgProfile::FEMALE];
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

		if(propConcurrentFemale > (propConcurrentMale * _eventParams.calibrationInputs.femalePropInConcurrentRatio))
		{
			passesCalib = false;
			passesPropConcRatio = false;
		}
	}

	unsigned long numSAFemaleHR = numSexuallyActiveRisk[DmgProfile::FEMALE][Person::HIGH];
	unsigned long numSAFemaleLR = numSexuallyActiveRisk[DmgProfile::FEMALE][Person::LOW];
	double numActsFemaleLRtoHRRatio = -1;
	firstRow << Constants::TAB;

	if(numSAFemaleHR != 0 && numSAFemaleLR != 0)
	{
		double avgNumActsFemaleHR = numActsMonthRisk[DmgProfile::FEMALE][Person::HIGH] / (double)numSAFemaleHR;
		double avgNumActsFemaleLR = numActsMonthRisk[DmgProfile::FEMALE][Person::LOW] / (double)numSAFemaleLR;

		if(avgNumActsFemaleHR != 0)
		{
			numActsFemaleLRtoHRRatio = avgNumActsFemaleLR / avgNumActsFemaleHR;
		}

		if(avgNumActsFemaleLR > (avgNumActsFemaleHR * _eventParams.calibrationInputs.femaleNumActsLRtoHRRatio))
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

	if(_eventParams.outputTrace[EventParams::TraceFileType::CalibrationStatistics])
	{
		_eventParams.traceStreams[EventParams::TraceFileType::CalibrationStatistics] << firstRow.str() << endl << secondRow.str() << endl <<
		        thirdRow.str() << endl;
	}

	return passesCalib;
}

unsigned long Population::createPartnerships(EventParams &_eventParams, Person *_initiator,
        std::list<Person *>::iterator * /*_p_Iter*/, SexualPartnership::Type _partnershipType, bool _forceNumPartnersOne)
{
	assert((_initiator != nullptr));
	assert((_initiator->isAlive()));
	//Boolean for determining whether we print this creation to singlePersonTrace
	bool printToTrace = false;

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && _initiator->trace())
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
		numPartners = _initiator->rollForNumPartners(_eventParams.randomNums, _partnershipType);
	}

	if(printToTrace && numPartners > 0)
	{
		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "+ Male " << _initiator->getID() << " attempts to form " <<
		        numPartners << " " << (SexualPartnership::TypeStrings.at(_partnershipType)) << " partnerships:" << endl;
	}

	while(numPartners > 0)
	{
		//Decrement numPartners
		numPartners--;
		//pick the bucket that we will attempt to choose from
		DmgProfileBucket *bucket = potentialPartnerBuckets[(int)_partnershipType].at(_eventParams.randomNums.chooseIndex(
			Population::eligibleBucketWeights[(int)_partnershipType]));
		assert(bucket != nullptr);
		std::list<Person *> attemptedPartners;
		bool foundPartner = false;
		Person *chosenPartner = nullptr;
		bool printTracePartner = false;

		//the partner that this man will have a relationship with
		//remove the partner from the pool will be added back later
		for(int i = 0; i < 10; i++)
		{
			Person *partner = bucket->drawMember(_eventParams.randomNums, _initiator, _partnershipType, Constants::REMOVE);

			if(partner == nullptr)
			{
				if(printToTrace)
				{
					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "  +x Male " << _initiator->getID() <<
					        " attempted to draw from empty bucket" << endl;
					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "   x Partnership not formed!" << endl;
				}

				//This partnership will not be formed: increase the number of unformed partnerships
				_initiator->increaseUnformedPartnershipTallies(_partnershipType);
				break;
			}

			if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && partner->trace())
			{
				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "+ Female " << partner->getID() << " is chosen for a " << 
				        (SexualPartnership::TypeStrings.at(_partnershipType)) << " partnership:" << endl;
				printTracePartner = true;
			}

			assert(partner != nullptr);
			assert(partner->isAlive());
			attemptedPartners.push_back(partner);

			//if we tried to draw someone we are already seeing, then redraw until we pick someone new
			if(_initiator->isPartneredWith(partner))
			{
				if(_eventParams.debugLevel > DEBUG1)
				{
					_initiator->print(cerr, "");
					cerr << _initiator->getID() << " Attempted repeat partnership with " << partner->getID() << ", " <<
					     (SexualPartnership::TypeStrings.at(_partnershipType)) << endl;
					_initiator->printCurrentPartners(cerr, "");
				}

				if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (_initiator->trace() || partner->trace()))
				{
					if(_initiator->trace())
					{
						if(_initiator->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
						{
							_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "  +x Male ";
						}
						else
						{
							_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "  +x Female ";
						}

						_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << _initiator->getID() << " attempted repeat partnership with " <<
						        partner->getID() << endl;
					}
					else
					{
						if(partner->getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::FEMALE)
						{
							_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "  +x Female ";
						}
						else
						{
							_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "  +x Male ";
						}

						_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << partner->getID() << " was selected *again* by " <<
						        _initiator->getID() << endl;
					}

					_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "   x Repeat partnership not formed!" << endl;
				}
			}
			else if(partner != _initiator)
			{
				foundPartner = true;
				chosenPartner = partner;
				break;
			}
		}

		if(!foundPartner)
		{
			//This partnership will not be formed: increase the number of unformed partnerships
			_initiator->increaseUnformedPartnershipTallies(_partnershipType);

			for(std::list<Person *>::iterator it = attemptedPartners.begin(); it != attemptedPartners.end(); it++)
			{
				//If woman was removed from entity pool, put her back!
				if((*it)->getCurrBucketProfileID() == DmgProfile::END)
				{
					entities->addEntity((*it));
				}
			}

			continue;
		}

		if(printToTrace || printTracePartner)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "  + Male " << _initiator->getID() << " (";
			_initiator->getDmgProfile()->print(_eventParams.traceStreams[EventParams::TraceFileType::Singleperson], "");
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " age " << _initiator->getAge(YEAR) << ") forms " << 
			        (SexualPartnership::TypeStrings.at(_partnershipType)) << " with female " << chosenPartner->getID() << " (";
			chosenPartner->getDmgProfile()->print(_eventParams.traceStreams[EventParams::TraceFileType::Singleperson], "");
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " age " << chosenPartner->getAge(
			            YEAR) << ", " << chosenPartner->getSexualActivity() << " marbles, " << ((chosenPartner->getRiskLevel() == Person::HIGH)
			                    ? "HIGH" : "LOW") << " risk)";
		}

		//the pointer to this partnership will be stored within initiator.
		SexualPartnership *sp = new SexualPartnership(_initiator, chosenPartner, _eventParams, _partnershipType);

		//add all persons back to entity pool
		for(std::list<Person *>::iterator it = attemptedPartners.begin(); it != attemptedPartners.end(); it++)
		{
			//If woman was removed from entity pool, put her back!
			if((*it)->getCurrBucketProfileID() == DmgProfile::END)
			{
				entities->addEntity((*it));
			}
		}

		if(!_initiator->inCorrectDmgProfileBucket())
		{
			entities->refreshDmgProfileBucket(_initiator, nullptr);
		}

		assert(chosenPartner->inCorrectDmgProfileBucket());
		assert(_initiator->inCorrectDmgProfileBucket());
		/** Add the new partnership to the initiator's list of edges in the graph */
		GraphVizGraphElements::personNode *graphNode = _initiator->getPersonNode();
		graphNode->addRelationship(chosenPartner->getID(), sp->getTimeOfFormation(), sp->getTimeOfDissolution(), sp->getType());
		//increment if partnership was formed
		numFormed++;
	} //for(int partner = 0; partner < numPartners; partner++) {

	//TODO: Return difference between numFormed and original value of numPartners as well as numFormed... create field inside initiator
	return numFormed;
}

void Population::processDeath(EventParams &_eventParams, Person *_p, bool calculateLE)
{
	assert((_p != nullptr));
	assert((!_p->isAlive()));

	//Calculate life expectancy info
	if(calculateLE)
	{
		assert((popStats->selectedLEStats != nullptr));
		assert(_p->getAge(YEAR) >= 0);
		assert(_p->getAge(YEAR) <= Person::maxYrForDeathStats);
		popStats->selectedLEStats->deathsByAge[_p->getAge(YEAR)]++;
	}

	//print out this info to the trace
	if(_eventParams.debugLevel > DEBUG1 && _eventParams.outputTrace[EventParams::TraceFileType::Events])
	{
		_p->print(_eventParams.traceStreams[EventParams::TraceFileType::Events], "Someone died: " + Constants::TAB);
	}

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && _p->trace())
	{
		_p->print(_eventParams.traceStreams[EventParams::TraceFileType::Singleperson], ">> Today we mourn: ");
	}

	//holds any former steady partners that are widowed after a partner's death
	// we may need to put the partners back into the singles pool
	std::list<SexualPartnership *> formerPartnerships;

	//we have to take care of what happens to any ongoing partnerships
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		_p->getPartnershipsToEnd(_eventParams.currTime, SexualPartnership::Type(type), formerPartnerships, true);
	} //for(SexualPartnership::Type type = SexualPartnership::Type(0); type < SexualPartnership::Type::ENDType; ++type) {

	dissolveSexualPartnerships(_eventParams, _p, formerPartnerships);
	currDeathCauses[_p->deathStatus]++;
	popStats->processDeath(_p, _eventParams);

	if(_eventParams.genGraphViz)
	{
		/** If we're using graphViz, update the time of death in the graph node add the persons node to the population graph */
		_p->getPersonNode()->timeDied = _eventParams.currTime;
		graph->persons.push_back(_p->getPersonNode());
		/** Need to keep person around to print out the black (dead) persons */
		_p->deletePersonWithoutDeleting();
	}
	else
	{
		//the deconstructor will take _p out of all non-steady relationships
		delete _p;
	}
}
//--------------------------< END helper methods  >-------------------------------------//



//-----------< BEGIN getters,setters, and print functions >--------------------//


long Population::calcPrevalentPopulation(long _time)
{
	assert(_time >= 0);
	int totalInfected = 0;		//total infected in the while population
	//holds a pointer to the current bucket we are looking at
	DmgProfileBucket *currBucket = nullptr;
	//holds number of prevalent infections
	unsigned long prevalenceByBucket[DmgProfile::TotalNumBuckets][InfectionsTracker::NUMBER_GENERATIONS_TO_TRACE];
	unsigned long prevalenceByRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];

	AgeRangeSizeContainer prevalenceByAgeMale, prevalenceByAgeFemale;

	//initialize prevalent infections by age
	for(auto ageBucketParams : popWideParams.initialAgeBuckets)
	{
		AgeRange range = {ageBucketParams->minAgeMth, ageBucketParams->maxAgeMth};
		prevalenceByAgeMale.push_back({range, 0});
		prevalenceByAgeFemale.push_back({range, 0});
	}

	//initialize prevalence tallies to 0
	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
		for(int j = 0; j < DmgProfile::ENDGender; j++)
		{
			for(int k = 0; k < DmgProfile::ENDEmployment; k++)
			{
				prevalenceByRiskGenderEmployment[i][j][k] = 0;
			}
		}
	}

	DmgProfile::ProfileID currProfileID = DmgProfile::MIN;

	while(currProfileID <= DmgProfile::MAX)
	{
		for(int i = 0; i < popStats->infectionsTracker.NUMBER_GENERATIONS_TO_TRACE; i++)
		{
			prevalenceByBucket[currProfileID][i] = 0;
		}

		currProfileID++;
	}

	//check if there is another bucket of entities to check
	//if there is a bucket, then iterate through people in bucket
	currProfileID = DmgProfile::MIN;

	while(currProfileID <= DmgProfile::MAX)
	{
		currBucket = entities->getBucket(currProfileID);

		//if people of this particular profile don't exist in the population, move on.
		if((currBucket == nullptr) || (currBucket->size() == 0))
		{
			currProfileID++;
			continue;
		}

		for(int generation = Constants::PREVALENT_INFECTION;
		        generation < popStats->infectionsTracker.NUMBER_GENERATIONS_TO_TRACE; generation++)
		{
			//count number of infected in bucket
			prevalenceByBucket[currProfileID][generation] = currBucket->getNumInfected(generation);
			//count total # of infected people
			totalInfected += prevalenceByBucket[currProfileID][generation];
		}

		//add the sizes of sexually active buckets
		if(DmgProfile::get(currBucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::SA)
		{
			//Update size by risk
			for(int i = 0; i < Person::ENDRiskLevel; i++)
			{
				prevalenceByRiskGenderEmployment[i][DmgProfile::get(currBucket->getProfileID(),
				                                    DmgProfile::GENDER)][DmgProfile::get(currBucket->getProfileID(),
				                                            DmgProfile::EMPLOYMENT)] += ((BucketSexualMixing *)currBucket)->getNumInfected((Person::RiskLevel) i);
			}

			//Update size by age range
			bool isMale = DmgProfile::get(currBucket->getProfileID(), DmgProfile::GENDER) == DmgProfile::MALE;
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

	//save the prevalent infections by bucket in the PopStats
	popStats->infectionsTracker.setPrevalentInfections(_time, prevalenceByBucket, prevalenceByAgeMale, prevalenceByAgeFemale, prevalenceByRiskGenderEmployment);

	return totalInfected;
}

PopulationParams::AgeBucketPrevalenceInfo *Population::getAgeBucket(Person *p)
{
	int age = p->getAge(MONTH);
	PopulationParams::AgeBucketPrevalenceInfo *ageBucketParams = nullptr;

	for(unsigned int ageBucket = 0; ageBucket < popWideParams.initialAgeBuckets.size(); ageBucket++)
	{
		//holds the parameters for the current age bucket
		ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucket);

		if(age >= ageBucketParams->minAgeMth && age <= ageBucketParams->maxAgeMth)
		{
			break;
		}
	}

	return ageBucketParams;
}

int Population::getAgeBucketIndex(Person *p)
{
	int age = p->getAge(MONTH);
	PopulationParams::AgeBucketPrevalenceInfo *ageBucketParams;
	unsigned int ageBucket;

	for(ageBucket = 0; ageBucket < popWideParams.initialAgeBuckets.size(); ageBucket++)
	{
		//holds the parameters for the current age bucket
		ageBucketParams = popWideParams.initialAgeBuckets.at(ageBucket);

		if(age >= ageBucketParams->minAgeMth && age <= ageBucketParams->maxAgeMth)
		{
			break;
		}
	}

	return ageBucket;
}

long Population::getSize()
{
	return currSize;
}

long Population::getNASize()
{
	return currNASize;
}

long Population::getSize(DmgProfile::Gender gender)
{
	return currSizeGender[gender];
}

long Population::getSASize(DmgProfile::Gender _gender, Person::RiskLevel _risk)
{
	return currSASizeGenderRisk[_gender][_risk];
}

long Population::getCSWSize(DmgProfile::Gender _gender, Person::RiskLevel _risk)
{
	return currSizeGenderRiskCSW[_gender][_risk];
}

void Population::printMethodResults(EventParams &_eventParams, std::string _methodName, std::string _eventLabel,
                                    long _totalAffected, std::string _totalAffectedLabel, bool _showInfections)
{
	unsigned long totalInfected = 0;
	unsigned long totalPopSize = 0;
	unsigned long totalInSteady = 0;
	unsigned long totalInRegular = 0;
	unsigned long totalSexuallyActive = 0;
	_showInfections = false;

	if(_eventParams.debugLevel > DEBUG0 && _eventParams.outputTrace[EventParams::TraceFileType::Events])
	{
		//if we are at time 0, then print out headers
		if((_eventParams.currTime == 0) && (_eventParams.debugLevel == DEBUG1))
		{
			//print out headers for DEBUG level 1 in the DmgProfileBucket size trace
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "Time\t" << "EventLabel\tNumAffected\t";
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "Currently Infected" << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "Current Population Size" << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "Sexually Active Population" << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "Steady Partnership Population" << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "Regular Partnership Population" << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << Constants::TAB;
			entities->printBucketLabels(_eventParams.traceStreams[EventParams::TraceFileType::Events], _showInfections);
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << endl;
		} //if( _eventParams.currTime == 0 ) {

		//print out the sizes of the buckets to a string stream
		std::ostringstream bucketTotalsStr;
		entities->printBucketSizes(bucketTotalsStr, Constants::TAB, _showInfections, totalInfected, totalPopSize,
		                                 totalSexuallyActive, totalInSteady, totalInRegular, (_eventParams.debugLevel > DEBUG1));

		//if debug level > 1, then print trace format in verbose form and include labels
		if(_eventParams.debugLevel > DEBUG1)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "T: " << _eventParams.currTime << " -- " << _methodName << ": " <<
			        endl;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << Constants::TAB << _eventLabel;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << _totalAffectedLabel << "= " << _totalAffected << endl;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << "Current Person Pool Sizes:\t";
		}
		else
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << _eventParams.currTime << Constants::TAB << _eventLabel <<
			        Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << _totalAffected << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << totalInfected << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << totalPopSize << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << totalSexuallyActive << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << totalInSteady << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << totalInRegular << Constants::TAB;
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << Constants::TAB;
		}//if( _d > DEBUG1) {

		_eventParams.traceStreams[EventParams::TraceFileType::Events] << bucketTotalsStr.str();

		if(_eventParams.debugLevel > DEBUG1)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << endl;
		}

		//print out all people in the population
		if(_eventParams.debugLevel > DEBUG2)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Events] << endl;
			entities->print(_eventParams.traceStreams[EventParams::TraceFileType::Events]);
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Events] << endl;
	} //if( _eventParams.debugLevel > DEBUG0)
}

void Population::printPartnerships(EventParams &_eventParams, long _time, std::ostream &_outStream)
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

		for(int l = 0; l < DmgProfile::ENDEmployment; l++)
		{
			for(int j = 0; j < DmgProfile::ENDGender; j++)
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

		for(int l = 0; l < DmgProfile::ENDEmployment; l++)
		{
			for(int k = 0; k < DmgProfile::ENDRelationshipStatus; k++)
			{
				for(int j = 0; j < DmgProfile::ENDGender; j++)
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

		for(int l = 0; l < DmgProfile::ENDEmployment; l++)
		{
			for(int k = 0; k < DmgProfile::ENDRelationshipStatus; k++)
			{
				for(int j = 0; j < DmgProfile::ENDGender; j++)
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

		for(int l = 0; l < DmgProfile::ENDEmployment; l++)
		{
			for(int k = 0; k < DmgProfile::ENDRelationshipStatus; k++)
			{
				for(int j = 0; j < DmgProfile::ENDGender; j++)
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
		_outStream << firstRow.str() << endl;
		_outStream << secondRow.str() << endl;
		_outStream << thirdRow.str() << endl;
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
		numInPartnership[(int)SexualPartnership::Type::ENDType][DmgProfile::ENDGender][DmgProfile::ENDRelationshipStatus][DmgProfile::ENDEmployment][Person::ENDRiskLevel];
	unsigned long numInConcurrent[DmgProfile::ENDGender][DmgProfile::ENDEmployment][Person::ENDRiskLevel];
	unsigned long numInMultiple[DmgProfile::ENDGender][DmgProfile::ENDEmployment][Person::ENDRiskLevel][4];
	unsigned long doubleNumPartnerships[(int)SexualPartnership::Type::ENDType][3];

	for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
	{
		for(int j = 0; j < DmgProfile::ENDGender; j++)
		{
			for(int k = 0; k < DmgProfile::ENDRelationshipStatus; k++)
			{
				for(int l = 0; l < DmgProfile::ENDEmployment; l++)
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

	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		for(int j = 0; j < DmgProfile::ENDEmployment; j++)
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

	for(int gender = DmgProfile::MALE; gender < DmgProfile::ENDGender; gender++)
	{
		std::list<Person *>::iterator p_Iter = entities->begin((DmgProfile::Gender)gender);

		while(p_Iter != entities->end((DmgProfile::Gender) gender))
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
				numInMultiple[gender][(*p_Iter)->getDmgProfileVal(DmgProfile::EMPLOYMENT)][(*p_Iter)->getRiskLevel()][totalNumPartners -
				        2]++;
			}
			else if(totalNumPartners >= 5)
			{
				numInMultiple[gender][(*p_Iter)->getDmgProfileVal(DmgProfile::EMPLOYMENT)][(*p_Iter)->getRiskLevel()][3]++;
			}

			for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
			{
				if(numPartners[i] != 0)
				{
					numInPartnership[i][gender][(*p_Iter)->getDmgProfileVal(DmgProfile::RELATIONSHIP_STATUS)][(*p_Iter)->getDmgProfileVal(
					            DmgProfile::EMPLOYMENT)][(*p_Iter)->getRiskLevel()]++;
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

			if(_eventParams.concurrencyDef[concurrent]->useDefinition
			        && totalNumPartners >= _eventParams.concurrencyDef[concurrent]->minPartnershipsNeeded)
			{
				//count as concurrent partnership
				numInConcurrent[gender][(*p_Iter)->getDmgProfileVal(DmgProfile::EMPLOYMENT)][(*p_Iter)->getRiskLevel()]++;
			}

			p_Iter++;
		}
	}

	//steady
	for(int l = 0; l < DmgProfile::ENDEmployment; l++)
	{
		for(int j = 0; j < DmgProfile::ENDGender; j++)
		{
			for(int m = Person::HIGH; m >= 0; m--)
			{
				_outStream << numInPartnership[(int)SexualPartnership::Type::Steady][j][DmgProfile::NON_SINGLE][l][m] << Constants::TAB;
			}
		}
	}

	//regular
	for(int l = 0; l < DmgProfile::ENDEmployment; l++)
	{
		for(int k = 0; k < DmgProfile::ENDRelationshipStatus; k++)
		{
			for(int j = 0; j < DmgProfile::ENDGender; j++)
			{
				for(int m = Person::HIGH; m >= 0; m--)
				{
					_outStream << numInPartnership[(int)SexualPartnership::Type::Regular][j][k][l][m] << Constants::TAB;
				}
			}
		}
	}

	//casual
	for(int l = 0; l < DmgProfile::ENDEmployment; l++)
	{
		for(int k = 0; k < DmgProfile::ENDRelationshipStatus; k++)
		{
			for(int j = 0; j < DmgProfile::ENDGender; j++)
			{
				for(int m = Person::HIGH; m >= 0; m--)
				{
					_outStream << numInPartnership[(int)SexualPartnership::Type::Casual][j][k][l][m] << Constants::TAB;
				}
			}
		}
	}

	//CSW
	for(int l = 0; l < DmgProfile::ENDEmployment; l++)
	{
		for(int k = 0; k < DmgProfile::ENDRelationshipStatus; k++)
		{
			for(int j = 0; j < DmgProfile::ENDGender; j++)
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

	for(int i = 0; i < DmgProfile::ENDGender; i++)
	{
		for(int k = 0; k < Person::ENDRiskLevel; k++)
		{
			numInConcurrentCSW[k] += numInConcurrent[i][DmgProfile::CSW][k];

			for(int m = 0; m < 4; m++)
			{
				numInMultipleCSW[k][m] += numInMultiple[i][DmgProfile::CSW][k][m];
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
	_outStream << numInConcurrent[DmgProfile::MALE][DmgProfile::NON_CSW][Person::HIGH] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
		_outStream << numInMultiple[DmgProfile::MALE][DmgProfile::NON_CSW][Person::HIGH][m] << Constants::TAB;
	}

	//High Risk Females Non CSW
	_outStream << numInConcurrent[DmgProfile::FEMALE][DmgProfile::NON_CSW][Person::HIGH] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
		_outStream << numInMultiple[DmgProfile::FEMALE][DmgProfile::NON_CSW][Person::HIGH][m] << Constants::TAB;
	}

	//Low Risk Males Non CSW
	_outStream << numInConcurrent[DmgProfile::MALE][DmgProfile::NON_CSW][Person::LOW] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
		_outStream << numInMultiple[DmgProfile::MALE][DmgProfile::NON_CSW][Person::LOW][m] << Constants::TAB;
	}

	//Low Risk Females Non CSW
	_outStream << numInConcurrent[DmgProfile::FEMALE][DmgProfile::NON_CSW][Person::LOW] << Constants::TAB;

	for(int m = 0; m < 4; m++)
	{
		_outStream << numInMultiple[DmgProfile::FEMALE][DmgProfile::NON_CSW][Person::LOW][m] << Constants::TAB;
	}

	//Partnerships by type
	for(int i = 0; i < 4; i++)
	{
		for(int j = 0; j < 3; j++)
		{
			_outStream << doubleNumPartnerships[i][j] / 2 << Constants::TAB;
		}
	}

	_outStream << endl;
}

void Population::printClinical(EventParams &/*_eventParams*/, long _time, std::ostream &_outStream)
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
		_outStream << firstRow.str() << endl;
		_outStream << secondRow.str() << endl;
		_outStream << thirdRow.str() << endl;
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
	numWithHIVStatus[Person::ENDRiskLevel][DmgProfile::ENDEmployment][DmgProfile::ENDGender][Person::ENDHIVStatus];

	for(int i = 0; i < Person::ENDRiskLevel; i++)
	{
		for(int j = 0; j < DmgProfile::ENDEmployment; j++)
		{
			for(int k = 0; k < DmgProfile::ENDGender; k++)
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
	DmgProfileBucket *currBucket = nullptr;
	DmgProfile::ProfileID currProfileID = DmgProfile::MIN;

	while(currProfileID <= DmgProfile::MAX)
	{
		currBucket = entities->getBucket(currProfileID);

		//if people of this particular profile don't exist in the population, move on.
		if((currBucket == nullptr) || (currBucket->size() == 0))
		{
			currProfileID++;
			continue;
		}

		//accumulate all SA buckets
		if(DmgProfile::get(currBucket->getProfileID(), DmgProfile::SEXUAL_ACTIVITY_STATUS) == DmgProfile::SA)
		{
			for(int i = 0; i < Person::ENDRiskLevel; i++)
			{
				for(int m = 0; m < Person::ENDHIVStatus; m++)
				{
					numWithHIVStatus[i][DmgProfile::get(currBucket->getProfileID(),
					                                    DmgProfile::EMPLOYMENT)][DmgProfile::get(currBucket->getProfileID(),
					                                            DmgProfile::GENDER)][m] += ((BucketSexualMixing *)currBucket)->sizeRiskHIVStatus((Person::RiskLevel) i,
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
		for(int j = 0; j < DmgProfile::ENDEmployment; j++)
		{
			for(int k = 0; k < DmgProfile::ENDGender; k++)
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
		for(int k = 0; k < DmgProfile::ENDGender; k++)
		{
			for(int m = 0; m < Person::ENDHIVStatus; m++)
			{
				hivStatusCSW[m] += numWithHIVStatus[i][DmgProfile::CSW][k][m];
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

		for(int k = 0; k < DmgProfile::ENDGender; k++)
		{
			for(int m = 0; m < Person::ENDHIVStatus; m++)
			{
				hivStatusRisk[m] += numWithHIVStatus[i][DmgProfile::NON_CSW][k][m];
			}
		}

		for(int m = 0; m < Person::ENDHIVStatus; m++)
		{
			_outStream << hivStatusRisk[m] << Constants::TAB;
		}
	}

	//HIV Status of SA population by Gender and Risk
	for(int k = 0; k < DmgProfile::ENDGender; k++)
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
			for(int j = 0; j < DmgProfile::ENDEmployment; j++)
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

	popStats->infectionsTracker.recordCD4AtTransmission(_outStream);
	_outStream << endl;
}

void Population::printPopulation(EventParams &/*_eventParams*/, long _time, std::ostream &_outStream)
{
	assert(_time >= 0);
	//total # of age ranges to print out
	auto &currSizeByAgeRange = getSizeByAgeRange();
	int numAgeRanges = currSizeByAgeRange.size();

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
		_outStream << firstRow.str() << endl;
		_outStream << secondRow.str() << endl;
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
	_outStream << getSize() << Constants::TAB;
	_outStream << getSize() - getNASize() << Constants::TAB;
	//output deaths by causes
	_outStream << currDeathCauses[Person::DTH_OI] << Constants::TAB << currDeathCauses[Person::DTH_CHRAIDS] <<
	           Constants::TAB << currDeathCauses[Person::DTH_NONAIDS] << Constants::TAB <<
	           currDeathCauses[Person::DTH_TOX_ART] << Constants::TAB << currDeathCauses[Person::DTH_TOX_PROPH] <<
	           Constants::TAB << currDeathCauses[Person::DTH_OTHER] << Constants::TAB;
	long totalDeaths = 0;

	for(int i = 1; i < Person::ENDDeathStatus; i++)
	{
		totalDeaths += currDeathCauses[i];
	}

	_outStream << totalDeaths << Constants::TAB;
	//output size of male and female populations
	_outStream << getSize(DmgProfile::MALE) << Constants::TAB;
	_outStream << getSize(DmgProfile::FEMALE) << Constants::TAB;
	//output size by risk
	_outStream << currSizeRiskCSW[Person::HIGH] << Constants::TAB << currSizeRiskCSW[Person::LOW] <<
	           Constants::TAB;

	for(int i = Person::HIGH; i >= 0; i--)
	{
		for(int j = 0; j < DmgProfile::ENDGender; j++)
		{
			_outStream << getSASize((DmgProfile::Gender) j,
			                              (Person::RiskLevel) i) - currSizeGenderRiskCSW[j][i] << Constants::TAB;
		}
	}

	//output size by age
	_outStream << getNASize() << Constants::TAB;

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

	_outStream << endl;
}

void Population::saveState(std::ostream &_outStream, long currTime)
{
	//Used to iterate through persons
	std::list<Person *>::iterator p_Iter;
	bool isFirst = true;

	//Double loop: first iterate through the men, then the women
	for(int gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++)
	{
		DmgProfile::Gender gender = DmgProfile::MALE;

		if(gend == DmgProfile::FEMALE)
		{
			gender = DmgProfile::FEMALE;
		}

		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			Person *p = (*p_Iter);
			assert(p != nullptr);

			if(!isFirst)
			{
				_outStream << "," << endl << endl;
			}

			isFirst = false;
			_outStream << "{";
			//save the state of each person
			p->saveState(_outStream, currTime);
			_outStream << "}";
			p_Iter++;
		}//while (p_Iter != entities->end(gender))
	}//	for (DmgProfile::Gender gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++){
}

void Population::printARTRolloutOutcomes(EventParams &_eventParams, std::ostream &_outStream)
{
	popStats->artTracker.printArtRolloutOutcomes(_eventParams.currTime, _outStream, this);
}

/**
this is called at specified time points to record the partner history frequency
**/
void Population::recordPartAcqFreq()
{
	assert(popStats->selectedPartAcqStats != nullptr);
	std::list<Person *>::iterator p_Iter;

	//Double loop: first iterate through the men, then the women
	for(int gend = DmgProfile::MALE; gend < DmgProfile::ENDGender; gend++)
	{
		DmgProfile::Gender gender = DmgProfile::MALE;

		if(gend == DmgProfile::FEMALE)
		{
			gender = DmgProfile::FEMALE;
		}

		p_Iter = entities->begin(gender);

		while(p_Iter != entities->end(gender))
		{
			int numPartnersInHistory = (*p_Iter)->getNumPartnersInHistory();

			if(numPartnersInHistory >= PopStats::SinglePartAcqStats::NUM_PARTNER_BINS)
			{
				numPartnersInHistory = PopStats::SinglePartAcqStats::NUM_PARTNER_BINS - 1;
			}

			if(numPartnersInHistory < 0)
			{
				numPartnersInHistory = 0;
			}

			popStats->selectedPartAcqStats->partnerFreq[numPartnersInHistory]++;
			p_Iter++;
		}
	}
}

//-----------< END getters,setters, and print functions >--------------------//

