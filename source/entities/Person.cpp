#include "Person.h"
#include "Male.h"
#include "Female.h"
#include "classifiers/SexualPartnership.h"
#include "../core/Constants.h"
#include "../data/EventParams.h"
#include "../util/Utility.h"
#include "../util/rand/RandomNumberGenerator.h"
#include "../statistics/InfectionsTracker.h"
#include "../statistics/ArtRolloutTracker.h"
#include "../statistics/CostsTracker.h"

class EntityPool;

long Person::idCounter = 0;
int Person::numTracesSoFar = 0;

//This is pretty much only used by the NA folks who are NA at the end of the model and need to have their LMs added to total
//TODO: But maybe they shouldn't?
std::vector<double> Person::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Last];

const std::vector<std::string> Person::StatsStr =
{
	"TOTAL_LM",
	"HIV_NEG_LM_INSIM",
	"HIV_POS_POSTINFECT_LM_INSIM",
	"EXPOSURES_BEFORE_INF",
	"NUM_INFECTED",
	"AGE_AT_INFECTION_MTH",
	"TIME_OF_INFECTION_MTH",
	"GENERATION_OF_INFECTION"
};

EnumCls<Person::Stats> Person::StatsEnum(Person::StatsStr);

//----------------< Start Methods for Person >-------------------//
//All implemented methods are in alphabetical order execept for the constructors/destructors (at bottom of Person section)
//The virtual methods are implemeted by Male and Female and are the very bottom fo the file


void Person::ageOneTimeUnit()
{
	age++;
}

Person::CD4Strata Person::getCd4Stratum() const
{
	switch(cepacPatient->getDiseaseState()->currTrueCD4Strata)
	{
	case SimContext::CD4_VLO:
		return CD4_ZERO;

	case SimContext::CD4__LO:
		return CD4_ONE;

	case SimContext::CD4_MLO:
		return CD4_TWO;

	case SimContext::CD4_MHI:
		return CD4_THREE;

	case SimContext::CD4__HI:
		return CD4_FOUR;

	case SimContext::CD4_VHI:
		return CD4_FIVE;

	default:
		return ENDCD4Strata;
	}
}


    template<>
    DemographicProfile::Gender Person::getDemographicProfileVal() const { return (DemographicProfile::Gender)getDemographicProfileVal(DemographicProfile::Demographic::Gender); }

    template<>
    DemographicProfile::SexualActivityStatus Person::getDemographicProfileVal() const 
    { 
        return (DemographicProfile::SexualActivityStatus)getDemographicProfileVal(DemographicProfile::Demographic::SexualActivityStatus); 
    }

    template<>
    DemographicProfile::Employment Person::getDemographicProfileVal() const
    {
        return (DemographicProfile::Employment)getDemographicProfileVal(DemographicProfile::Demographic::Employment);
    }

bool Person::isEligibleForTreatment(const SimContext::TreatmentInputs::ARTStartPolicy &artStartPolicy)
{
	// Evaluate the CD4 only criteria
	double trueCD4 = cepacPatient->getDiseaseState()->currTrueCD4;
	if((trueCD4 >= artStartPolicy.CD4BoundsOnly[SimContext::LOWER_BOUND]) &&
		(trueCD4 <= artStartPolicy.CD4BoundsOnly[SimContext::UPPER_BOUND]))
	{
		return true;
	}

	// Evaluate the HVL strata only criteria
	SimContext::HVL_STRATA trueHVL = cepacPatient->getDiseaseState()->currTrueHVLStrata;
	if((trueHVL >= artStartPolicy.HVLBoundsOnly[SimContext::LOWER_BOUND]) &&
		(trueHVL <= artStartPolicy.HVLBoundsOnly[SimContext::UPPER_BOUND]))
	{
		return true;
	}

	// Evaluate the CD4 and HVL combined criteria
	if((trueCD4 >= artStartPolicy.CD4BoundsWithHVL[SimContext::LOWER_BOUND]) &&
		(trueCD4 <= artStartPolicy.CD4BoundsWithHVL[SimContext::UPPER_BOUND]) &&
		(trueHVL >= artStartPolicy.HVLBoundsWithCD4[SimContext::LOWER_BOUND]) &&
		(trueHVL <= artStartPolicy.HVLBoundsWithCD4[SimContext::UPPER_BOUND])) {
		return true;
	}

	// Evaluate the acute OIs since last ART only criteria
	int numOIs = 0;
	for(int i = 0; i < SimContext::OI_NUM; i++)
	{
		if(artStartPolicy.OIHistory[i])
		{
			numOIs += oiHistory[i];
		}
	}

	if(numOIs >= artStartPolicy.numOIs)
	{
		return true;
	}

	// Evaluate the acute OIs in cepacPatient's history and CD4 count criteria
	if((trueCD4 != SimContext::NOT_APPL) &&
		(trueCD4 >= artStartPolicy.CD4BoundsWithOIs[SimContext::LOWER_BOUND]) &&
		(trueCD4 <= artStartPolicy.CD4BoundsWithOIs[SimContext::UPPER_BOUND]))
	{
		for(int i = 0; i < SimContext::OI_NUM; i++)
		{
			if(artStartPolicy.OIHistoryWithCD4[i] && oiHistory[i])
			{
				return true;
			}
		}
	}

	return false;
}

Person *Person::allPartnerSexualActivity(EventParams &_eventParams, SexualPartnership::Type _partnershipType,
        list<Person *> &_newlyInfected, InfectionsTracker *infTrack)
{
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	//iterate through all partnerships of SexualActivity::Type _partnershipType and have them engage in sexual activity
	list<SexualPartnership *>::iterator iter = partners[(int)_partnershipType].begin();
	list<SexualPartnership *>::iterator iterEnd = partners[(int)_partnershipType].end();
	//becomes non-nullptr only when this person gets infected. We are saving the partner who infected this person
	Person *infectedMe = nullptr;

	while(iter != iterEnd)
	{
		//initiate sexual activity only if you are partner1
		if((*iter)->getPartner1() == this)
		{
			Person *infected = (*iter)->monthlySexualActivity(_eventParams, infTrack);

			//if you or your partners got infected, the infected joins the _newlyInfected list
			if(infected != nullptr)
			{
				_newlyInfected.push_back(infected);

				//if you got infected, then you have to save the person who infected you for record keeping
				if(infected == this)
				{
					infectedMe = (*iter)->getOtherPartner(this);
				}
			} //if(infected != nullptr) {
		}

		iter++;
	} //while(iter != iterEnd) {

	return infectedMe;
}

bool Person::availableForPartnership(SexualPartnership::Type _partnershipType) const
{
	if(_partnershipType == SexualPartnership::Type::Steady)
	{
		return (partners[(int)SexualPartnership::Type::Steady].empty());
	}
	else
	{
		return true;
	}
}

void Person::addPartnership(SexualPartnership *_partnership)
{
	assert(_partnership != nullptr);
	assert((_partnership->getPartner1() != nullptr));
	assert(_partnership->getPartner1()->isAlive());
	assert((_partnership->getPartner2() != nullptr));
	assert((_partnership->getPartner2()->isAlive()));
	partners[(int)_partnership->getType()].push_back(_partnership);
	numPartnersInHistory[(int)_partnership->getType()]++;

	monthOfLatestPartnershipDissolution[(int)_partnership->getType()] =
		max(monthOfLatestPartnershipDissolution[(int)_partnership->getType()], _partnership->getDissolutionTime());

	//if a STEADY partnership was added && we are SINGLE, the we need to change or RELATIONSHIP_STATUS
	if((_partnership->getType() == SexualPartnership::Type::Steady) &&
		(!partners[(int)SexualPartnership::Type::Steady].empty()) &&
        (getDemographicProfileVal(DemographicProfile::Demographic::RelationshipStatus) == (std::size_t)DemographicProfile::RelationshipStatus::Single))
	{
        dmgProfile.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::NonSingle);
	}
}

void Person::becomeInfected(int _generationOfInfection, EventParams &_eventParams)
{
    //hvl needs to be set even for people who are about to go through CEPAC so that isInfected() correctly returns true
	hvl = HVL_PRIMARY;
    //CD4 doesn't affect much in the transmission model yet... will be updated with CEPAC
	cd4 = -1;
	ageInfected = age;
	generationOfInfection = _generationOfInfection;

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
	{
        if(getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "@ Male ";
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "@ Female ";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << getID() << " has ";

		if(_generationOfInfection == 0)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "a prevalent case of HIV";
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "an incident case of HIV";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "!" << std::endl;

        if(_generationOfInfection == 0)
        {
            print(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].file, "Demographic info for person:");
        }
	}

	stats.setStat(Person::STAT_TIME_OF_INFECTION_MTH, _eventParams.currTime);
    stats.setStat(Person::STAT_AGE_AT_INFECTION_MTH, getAge(TimeGranularity::Month));
	stats.setStat(Person::STAT_GENERATION_OF_INFECTION, _generationOfInfection);

	//if a CEPAC person exists (i.e. they were created earlier and thus this is an incident case), set them to infected
	if(wentThroughCEPAC)
	{
		cepacPatient->forceNewInfection();
		cd4 = cepacPatient->getDiseaseState()->currTrueCD4;
		//update HVL state
		SimContext::HVL_STRATA hvlStrata = cepacPatient->getDiseaseState()->currTrueHVLStrata;

		if(hvlStrata == SimContext::HVL_VLO)
		{
			hvl = HVL_ZERO;    //0-20
		}
		else if(hvlStrata == SimContext::HVL__LO)
		{
			hvl = HVL_ONE;    //21-500
		}
		else if(hvlStrata == SimContext::HVL_MLO)
		{
			hvl = HVL_TWO;    //501-3000
		}
		else if(hvlStrata == SimContext::HVL_MED)
		{
			hvl = HVL_THREE;    //3001-10000
		}
		else if(hvlStrata == SimContext::HVL_MHI)
		{
			hvl = HVL_FOUR;    //10001-30000
		}
		else if(hvlStrata == SimContext::HVL__HI)
		{
			hvl = HVL_FIVE;    //30001-100000
		}
		else if(hvlStrata == SimContext::HVL_VHI)
		{
			hvl = HVL_SIX;    //100000+
		}
		else
		{
		    throw std::runtime_error("Invalid CEPAC API infection state: " + std::string((SimContext::HVL_STRATA_STRS[hvlStrata])));
		}

		currentTrueHvl = hvl;

		if(cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN)
		{
			hvl = HVL_PRIMARY;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
				hivStatus = OBSERVED_ACUTE;
			}
			else
			{
				hivStatus = UNOBSERVED_ACUTE;
			}
		}
		//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
		else if((!(cepacPatient->getARTState()->hasNextRegimenAvailable) &&
		         (!(cepacPatient->getARTState()->isOnART) || cepacPatient->getARTState()->hasObservedFailure)) &&
		        cepacPatient->getDiseaseState()->currTrueCD4 <= 50)
		{
			hvl = HVL_LATESTAGE;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
				hivStatus = OBSERVED_LATESTAGE;
			}
			else
			{
				hivStatus = UNOBSERVED_LATESTAGE;
			}
		}
		else
		{
			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
				hivStatus = OBSERVED_CHRONIC;
			}
			else
			{
				hivStatus = UNOBSERVED_CHRONIC;
			}
		}

		//Update OI History
		for(int i = 0; i < Constants::NUMBER_OF_OIS; i++)
		{
			oiHistory[i] = cepacPatient->getDiseaseState()->hasTrueOIHistory[i];
		}
	}
	else
	{
		//Infection of prevalent cases happens when CEPAC person is initialized
		initialCEPACpatient(_eventParams);
	}
}

void Person::initialCEPACpatient(EventParams &_eventParams)
{
	//Only initialize the person if they haven't already been initialized!  (Prevalent cases will get called to initialize twice!)
	if(!wentThroughCEPAC)
	{
		wentThroughCEPAC = true;
		//determine if prevalent or incident case
		//Prevalent cases will be set to be infected prior to initialization
		//"Incident" cases are not yet infected and will be initialized later.
		bool setAsIncidentCase = !isInfected();
		//initial CEPAC patient for this person
		SimContext::GENDER_TYPE cepacGender = SimContext::GENDER_FEMALE;

        if(getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
		{
			cepacGender = SimContext::GENDER_MALE;
		}

		//MULTIRUN
		/*if (_eventParams.currTime > 20){
			cepacPatient = new Patient(_eventParams.cepacSimContext2, _eventParams.cepacRunStats, _eventParams.cepacTracer,
					true, getAge(MONTH), cepacGender, (_generationOfInfection > 0));
		} else {*/
		//TODO: Switch to multiple input sheets!
		SimContext *simContextToUse;

		if(_eventParams.useRollout)
		{
			//When patients are initialized they are added to the untreated pool
			simContextToUse = _eventParams.untreatedContext;
		}
		else
		{
			simContextToUse = _eventParams.cepacSimContexts[getCEPACSimContextIndex(_eventParams)];
		}

		cepacPatient = new Patient(simContextToUse, _eventParams.cepacRunStats, _eventParams.cepacTracer,
            true, getAge(TimeGranularity::Month), cepacGender, setAsIncidentCase, _eventParams.currTime);

		//}
		//Only update hvl and cd4 if the patient is infected
		//update HVL and CD4  and infection status for this Person if they are infected
		if(isInfected())
		{
			cd4 = cepacPatient->getDiseaseState()->currTrueCD4;
			//update HVL state
			SimContext::HVL_STRATA hvlStrata = cepacPatient->getDiseaseState()->currTrueHVLStrata;

			if(hvlStrata == SimContext::HVL_VLO)
			{
				hvl = HVL_ZERO;    //0-20
			}
			else if(hvlStrata == SimContext::HVL__LO)
			{
				hvl = HVL_ONE;    //21-500
			}
			else if(hvlStrata == SimContext::HVL_MLO)
			{
				hvl = HVL_TWO;    //501-3000
			}
			else if(hvlStrata == SimContext::HVL_MED)
			{
				hvl = HVL_THREE;    //3001-10000
			}
			else if(hvlStrata == SimContext::HVL_MHI)
			{
				hvl = HVL_FOUR;    //10001-30000
			}
			else if(hvlStrata == SimContext::HVL__HI)
			{
				hvl = HVL_FIVE;    //30001-100000
			}
			else if(hvlStrata == SimContext::HVL_VHI)
			{
				hvl = HVL_SIX;    //100000+
			}
			else
			{
			    throw std::runtime_error("Invalid CEPAC API infection state: " + std::string((SimContext::HVL_STRATA_STRS[hvlStrata])));
			}

			currentTrueHvl = hvl;

			if(cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN)
			{
				hvl = HVL_PRIMARY;

				if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				{
					hivStatus = OBSERVED_ACUTE;
				}
				else
				{
					hivStatus = UNOBSERVED_ACUTE;
				}
			}
			//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
			else if((!(cepacPatient->getARTState()->hasNextRegimenAvailable) &&
			         (!(cepacPatient->getARTState()->isOnART) || cepacPatient->getARTState()->hasObservedFailure)) &&
			        cepacPatient->getDiseaseState()->currTrueCD4 <= 50)
			{
				hvl = HVL_LATESTAGE;

				if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				{
					hivStatus = OBSERVED_LATESTAGE;
				}
				else
				{
					hivStatus = UNOBSERVED_LATESTAGE;
				}
			}
			else
			{
				if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				{
					hivStatus = OBSERVED_CHRONIC;
				}
				else
				{
					hivStatus = UNOBSERVED_CHRONIC;
				}
			}

			//Update OI History
			for(int i = 0; i < Constants::NUMBER_OF_OIS; i++)
			{
				oiHistory[i] = cepacPatient->getDiseaseState()->hasTrueOIHistory[i];
			}
		}//if (isInfected)

		Person::numTracesSoFar++;
	}
}

int Person::getGenerationOfInfection(bool cap_at_5) const
{
	//TODO: Make this a constant!
	if(cap_at_5 && generationOfInfection > 5)
	{
		return 5;
	}
	else
	{
		return generationOfInfection;
	}
}

int Person::getNumPartners(SexualPartnership::Type _type)
{
	return (int)partners[(int)_type].size();
}

int Person::getNumPartners(SexualPartnership::Type _type, bool sameRisk)
{
	int numPartners = 0;

	for(std::list<SexualPartnership *>::iterator partnerIter = partners[(int)_type].begin();
		partnerIter != partners[(int)_type].end(); partnerIter++)
	{
		Person *partner;

		if((*partnerIter)->getPartner1() == this)
		{
			partner = (*partnerIter)->getPartner2();
		}
		else
		{
			partner = (*partnerIter)->getPartner1();
		}

		bool isSameRisk = risk == partner->getRiskLevel();

		if(isSameRisk == sameRisk)
		{
			numPartners += 1;
		}
	}

	return numPartners;
}

int Person::getNumPartnersInHistory(SexualPartnership::Type _type)
{
	return numPartnersInHistory[(int)_type];
}
int Person::getNumPartnersInHistory()
{
	int total = 0;

	for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
	{
		total += numPartnersInHistory[i];
	}

	return total;
}
int Person::getMonthOfLatestPartnershipDissolution(SexualPartnership::Type _type)
{
	return monthOfLatestPartnershipDissolution[(int)_type];
}
int Person::getMonthOfLatestConcurrent()
{
	return monthOfLatestConcurrent;
}
void Person::setMonthOfLatestConcurrent(int _month)
{
	monthOfLatestConcurrent = _month;
}
void Person::becomeSexuallyActive(EventParams &_eventParams)
{
    dmgProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::Active);
	//CEPAC person needs to be initialized
	initialCEPACpatient(_eventParams);

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
	{
        if(getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Male ";
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Female ";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << getID() << " becomes sexually active" << std::endl;
	}
}

/*
Person *Person::fling(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams,
                      InfectionsTracker *infTrack)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	int numActs = rollNumEventsPerPartner(_p, _eventParams.randomNums, _partnershipType);
	return sexualActivity(_p, numActs, _partnershipType, _eventParams, infTrack);
}
*/

int Person::getAge(TimeGranularity _granularity) const
{
    if(_granularity == TimeGranularity::Month)
	{
		return age;
	}
	else
	{
        return Utility::convertTime(TimeGranularity::Month, _granularity, age);
	}
}


DemographicProfile::ProfileID Person::getCurrBucketProfileID()
{
	return currentBucketID;
}

const DemographicProfile *Person::getDemographicProfile() const
{
	return &dmgProfile;
}

BaseEnumCls::Enum Person::getDemographicProfileVal(DemographicProfile::Demographic _demographic) const
{
	return dmgProfile.get(_demographic);
}

unsigned long Person::getID() const
{
	return id;
}

long Person::getPartnershipsToEnd(long _currTime, SexualPartnership::Type _partnershipType,
                                  list<SexualPartnership *> &_partnershipsToEnd, bool _fromDeath)
{
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	assert((_currTime >= 0) || _fromDeath);

	if(partners[(int)_partnershipType].size() == 0)
	{
		return 0;
	}

	//iterate through all current partnerships that had any duration to them.
	//The iterator points to class SexualPartnership
	list<SexualPartnership *>::iterator iter = partners[(int)_partnershipType].begin();
	list<SexualPartnership *>::iterator iterEnd = partners[(int)_partnershipType].end();
	long numEnded = 0;

	//go through all partnerships
	while(iter != iterEnd)
	{
		//if it's time for that partnership to end, then put that partnership is the list for deletion
		if((*iter)->checkTimeForSplit(_currTime) || _fromDeath)
		{
			_partnershipsToEnd.push_back(*iter);
			numEnded++;
		}

		iter++;
	} //	while(iter != iterEnd) {

	return numEnded;
}

//Begin Unformed Partnership helper methods
int Person::getTotalUnformedPartnerships(SexualPartnership::Type type)
{
	return unformedPartnershipsTotal[(int)type];
}

int Person::getLatestUnformedPartnerships(SexualPartnership::Type type)
{
	return unformedPartnershipsLatestTime[(int)type];
}

void Person::increaseUnformedPartnershipTallies(SexualPartnership::Type type)
{
	unformedPartnershipsLatestTime[(int)type] += 1;
	unformedPartnershipsTotal[(int)type] += 1;
}

void Person::resetLatestUnformedPartnerships(SexualPartnership::Type type)
{
	unformedPartnershipsLatestTime[(int)type] = 0;
}

//End Unformed Partnership helper methods

unsigned int Person::getPopulationID()
{
	return populationID;
}

//returns traceMe
bool Person::trace()
{
	return traceMe;
}
//sets traceMe to true
void Person::setToBeTraced()
{
	traceMe = true;
}

const Person::StatsRecord *Person::getStats()
{
	return &stats;
}

bool Person::inCorrectBucketDemographicProfile()
{
	return (dmgProfile.getProfileID() == currentBucketID);
}

bool Person::isAlive() const
{
	if(this == nullptr)
	{
		return false;
	}

	return !death;
}

bool Person::isPartneredWith(Person *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());

	for(int partnershipType = 0; partnershipType < (int)SexualPartnership::Type::ENDType; ++partnershipType)
	{
		//iterate through each partnership and check if _p is a member of one of them
		list<SexualPartnership *>::iterator iter = partners[(int)partnershipType].begin();
		list<SexualPartnership *>::iterator endIter = partners[(int)partnershipType].end();

		while(iter != endIter)
		{
			if((*iter)->isMember(_p))
			{
				return true;
			}

			iter++;
		}
	}

	return false;
}

bool Person::hasPartnership(SexualPartnership::Type partnershipType)
{
	if(partners[(int)partnershipType].size() > 0)
	{
		return true;
	}

	return false;
}
void Person::print(ostream &_outStream, const std::string &_prefix) const
{
	_outStream << _prefix << std::endl;
    _outStream << ((getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male) ? "Male" : "Female") << Constants::TAB;
	_outStream << "ID: " << id << Constants::TAB;
	_outStream << "(";
	getDemographicProfile()->print(_outStream, "");
	_outStream << ")";
    _outStream << Constants::TAB << "Age(mos.): " << getAge(TimeGranularity::Month);
	_outStream <<  Constants::TAB << "CD4: " << cd4;
	_outStream << Constants::TAB << "HVL: " << hvl;
	_outStream << Constants::TAB << "Risk: " << ((risk == Person::HIGH) ? "HIGH" : "LOW");
	_outStream << Constants::TAB << "Marbles: " << activityLevel;
	_outStream << std::endl;
}

void Person::printCurrentPartners(ostream &_outStream, const std::string &prefix)
{
    _outStream << prefix << std::endl;
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		std::list<SexualPartnership *>::iterator iter = partners[(int)type].begin();
		std::list<SexualPartnership *>::iterator iterEnd = partners[(int)type].end();

		if(iter != iterEnd)
		{
			_outStream << (SexualPartnership::TypeStrings.at(SexualPartnership::Type(type))) << Constants::COLON << std::endl;
		}

        int i = 0;
		while(iter != iterEnd)
		{
			Person *partner = (*iter)->getOtherPartner(this);
			partner->print(_outStream, "Partner " + std::to_string(i++));
			iter++;
		}
	}
}
/**
*This function saves the state of the patient to file
*Uses Json like notation
*/
void Person::saveState(ostream &_outStream, long currTime)
{
	_outStream << "id:" << id << "," << std::endl; //id
	dmgProfile.saveState(_outStream); //dmg profile
	_outStream << "curBktID:" << currentBucketID << "," << std::endl; //bucket id (contains same information as dmgprofile)
	//save all the sexual partnerships
	_outStream << "partners:[";
	bool firstPartner = true;

	for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)  //loop through partnership types
	{
		for(std::list<SexualPartnership *>::iterator it = partners[i].begin(); it != partners[i].end(); it++) //loop through all partners
		{
			if(!firstPartner)
			{
				_outStream << ",";
			}

			firstPartner = false;
			(*it)->saveState(_outStream, id, currTime);
		}
	}

	_outStream << "]," << std::endl;
	_outStream << "partnerHist:[";
	firstPartner = true;

	for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)  //loop through partnership types
	{
		if(!firstPartner)
		{
			_outStream << ",";
		}

		firstPartner = false;
		_outStream << numPartnersInHistory[i];
	}

	_outStream << "]," << std::endl;
	_outStream << "genInf:" << generationOfInfection << "," << std::endl; //generation of infection
	_outStream << "risk:" << risk << "," << std::endl; //risk Level
	_outStream << "activity:" << activityLevel << "," << std::endl; //activity Level
	_outStream << "age:" << age << "," << std::endl; //age
	_outStream << "initAge:" << initAge << "," << std::endl; //initial age
	_outStream << "dead:" << death << "," << std::endl; //death
	_outStream << "hvl:" << hvl; //hvl in transmission includes primary and late stage
}

bool Person::isInfected()
{
	return (hvl > UNINFECTED);
}

void Person::removePartnership(SexualPartnership *_partnership)
{
	assert(_partnership != nullptr);
	partners[(int)_partnership->getType()].remove(_partnership);

	//if a STEADY partnership was removed and we have no more, then we should be set to SINGLE
	if((_partnership->getType() == SexualPartnership::Type::Steady) &&
		(partners[(int)SexualPartnership::Type::Steady].empty()) &&
        (getDemographicProfileVal(DemographicProfile::Demographic::RelationshipStatus) == (std::size_t)DemographicProfile::RelationshipStatus::NonSingle))
	{
        dmgProfile.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::Single);
	}
}

void Person::rollForBecomeSexWorker(EventParams &_eventParams, bool _isInit, double initialProb)
{
	//see whether this person will become a CSW when they make their sexual debut
	double currGenderChanceBecomeCSW;

	if(_isInit)
	{
		currGenderChanceBecomeCSW = initialProb;
	}
	else
	{
		currGenderChanceBecomeCSW = getChanceBecomeCsw();
	}

	if(_eventParams.randomNums.chance(currGenderChanceBecomeCSW))
	{
        if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
		{
            if(getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
			{
                _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Male ";
			}
			else
			{
                _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Female ";
			}

            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << getID() << " becomes CSW" << std::endl;
		}

        dmgProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::Csw);
	}
}

void Person::quitSexWork(EventParams &_eventParams)
{
    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
	{
        if(getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Male ";
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Female ";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << getID() << " quits being CSW" << std::endl;
	}

    dmgProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
}

//determine whether this person died
bool Person::rollForDeath(RandomNumberGenerator &_randomNums)
{
	assert(death == false);

	//if person is too old, then they automatically die
    if(getAge(TimeGranularity::Month) >= (12 * Person::maxYrForDeathStats))
	{
		death = true;
		deathStatus = DTH_OTHER;
	}
	//if they have gone through CEPAC
	else if(wentThroughCEPAC)
	{
		//CEPAC determines month of death
		//check if cepacPatient is dead
		if(!cepacPatient->isAlive())
		{
			death = true;
			SimContext::DTH_CAUSES causeOfDeath = cepacPatient->getDiseaseState()->causeOfDeath;

			if(causeOfDeath < SimContext::DTH_CHRAIDS)
			{
				deathStatus = DTH_OI;    //oi death
			}
			else if(causeOfDeath == SimContext::DTH_CHRAIDS)
			{
				deathStatus = DTH_CHRAIDS;
			}
			else if(causeOfDeath == SimContext::DTH_NONAIDS)
			{
				deathStatus = DTH_NONAIDS;
			}
			else if(causeOfDeath == SimContext::DTH_TOX_ART)
			{
				deathStatus = DTH_TOX_ART;
			}
			else if(causeOfDeath == SimContext::DTH_TOX_PROPH)
			{
				deathStatus = DTH_TOX_PROPH;
			}
			else
			{
				deathStatus = DTH_OTHER;
			}
		}
	}
	//There may be those remaining that didn't go through CEPAC: the non-sexually actives!
	else
	{
		//this part of the function is for uninfected persons
		assert(Person::probDeathNatCauses[getDemographicProfileVal(DemographicProfile::Demographic::Gender)].size() > 0);

		//if this person is past Person::maxYrForDeathStats, they should not be alive
		//get the correct probability of death for this person's gender and age
        if(getAge(TimeGranularity::Year) >= static_cast<int>(Person::probDeathNatCauses[getDemographicProfileVal(DemographicProfile::Demographic::Gender)].size()))
		{
            cout << "The age is " << getAge(TimeGranularity::Year) << std::endl;
		}

        double deathRate = Person::probDeathNatCauses[getDemographicProfileVal(DemographicProfile::Demographic::Gender)].at(getAge(TimeGranularity::Year));
		death = _randomNums.chance(deathRate);

		if(death)
		{
			deathStatus = DTH_NONAIDS;
		}
	}

	//if they died, collect statistics
	if(death)
	{
        stats.setStat(STAT_TOTAL_LM, getAge(TimeGranularity::Month));
        stats.setStat(STAT_HIV_NEG_LM, getAge(TimeGranularity::Month) - (isInfected() ? stats.getStat(STAT_TIME_OF_INFECTION_MTH) : 0));
		stats.setStat(STAT_HIV_POS_POSTINFECT_LM, stats.getStat(STAT_TOTAL_LM) - stats.getStat(STAT_AGE_AT_INFECTION_MTH));
	}

	return death;
}

void Person::setCurrBucketProfileID(DemographicProfile::ProfileID _profileID)
{
	currentBucketID = _profileID;
}

void Person::setSimContext(SimContext *newSimContext)
{
	cepacPatient->setSimContext(newSimContext);
}

Person *Person::sexualActivity(Person *_p, int _numActs, SexualPartnership::Type _partnershipType,
                               EventParams &_eventParams, InfectionsTracker *infTrack)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	//TODO: CONDOM STUFF!

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
	{
		if(trace())
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "# Male " << getID() << " engages in " << _numActs <<
			        " acts with his " << (SexualPartnership::TypeStrings.at(_partnershipType)) << " " << _p->getID() << std::endl;
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "# Female " << _p->getID() << " engages in " << _numActs <<
			        " acts with her " << (SexualPartnership::TypeStrings.at(_partnershipType)) << " " << getID() << std::endl;
		}
	}

	//increment numacts for this person and partner
	incrementNumActsThisMonth(_numActs);
	_p->incrementNumActsThisMonth(_numActs);

	//people are either both already infected or both uninfected
	if(isInfected() == _p->isInfected())
	{
		return nullptr;
	}

	//if infection occurs, return true
	Person *infected = isInfected() ? this : _p;
	Person *uninfected = isInfected() ? _p : this;
	bool transmissionOccured = false;

	for(int i = 0; i < _numActs; i++)
	{
		/** Regardless of infection, record the exposure */
		infTrack->recordExposure(_eventParams.currTime, infected);
		//force of infection from infected to uninfected
		double foifPerEvent = infected->getFOI(uninfected, _partnershipType, _eventParams);

		//If a condom was used, increase the number of condoms used for each person by numActs
		if(infected->getCondomUsedLastFOICalculation())
		{
			incrementCondomsUsedThisMonth(1);
			_p->incrementCondomsUsedThisMonth(1);
		}

		if(_eventParams.randomNums.chance(foifPerEvent))
		{
			transmissionOccured = true;
		}
	}

	//perform _numActs and see whether someone gets infected
	if(transmissionOccured)
	{
        if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !!# " << infected->getID() << " infected " <<
			        uninfected->getID() << "!" << std::endl;
		}

		//record who infected whom
		if(infected == this)
		{
			stats.incrStat(Person::STAT_NUM_INFECTED, 1);
		}
		else
		{
			_p->stats.incrStat(Person::STAT_NUM_INFECTED, 1);
		}

		//if they get infected, then change status of uninfected to infected and count infection
		//The generation of infected for the newly infected will be 1+ the infected persons generation
		uninfected->becomeInfected(infected->getGenerationOfInfection(false) + 1, _eventParams);
		return uninfected;
	}
	else
	{
        if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !# " << infected->getID() << " exposed but did not infect " <<
			        uninfected->getID() << "!" << std::endl;
		}

		//uninfected person was exposed but not infected
		uninfected->stats.incrStat(Person::STAT_EXPOSURES_BEFORE_INF, _numActs);
	}//if( !_eventParams.randomNums.chance(pow( 1-foifPerEvent, eventsThisMonth)) ) {

	return nullptr;
}

template<typename T>
T Scale(const T &t, double factor)
{
	T r;
	std::transform(t.begin(), t.end(), r.begin(), std::bind1st(std::multiplies<double>(), factor));
	return r;
}

Person::HVLStrata HvlFromCepacHvl(SimContext::HVL_STRATA stratum)
{
    switch(stratum)
    {
    case SimContext::HVL_VLO: return Person::HVL_ZERO;
    case SimContext::HVL__LO: return Person::HVL_ONE;
    case SimContext::HVL_MLO: return Person::HVL_TWO;
    case SimContext::HVL_MED: return Person::HVL_THREE;
    case SimContext::HVL_MHI: return Person::HVL_FOUR;
    case SimContext::HVL__HI: return Person::HVL_FIVE;
    case SimContext::HVL_VHI: return Person::HVL_SIX;
    default: throw std::runtime_error("invalid hvl");
    }
}

std::string to_string(Person::HVLStrata stratum)
{
    switch(stratum)
    {
    case Person::UNINFECTED: return "uninfected";
    case Person::HVL_ZERO: return "0-20";
    case Person::HVL_ONE: return "21-500";
    case Person::HVL_TWO: return "501-3000";
    case Person::HVL_THREE: return "3001-10000";
    case Person::HVL_FOUR: return "10001-30000";
    case Person::HVL_FIVE: return "30001-100000";
    case Person::HVL_SIX: return "100000+";
    case Person::HVL_PRIMARY: return "primary";
    case Person::HVL_LATESTAGE: return "late-stage";
    default: throw std::runtime_error("invalid hvl");
    }
}

std::string to_string(Person::HIVStatus status)
{
    switch(status)
    {
    case Person::NEGATIVE: return "negative";
    case Person::OBSERVED_ACUTE: return "acute (observed)";
    case Person::OBSERVED_CHRONIC: return "chronic (observed)";
    case Person::OBSERVED_LATESTAGE: return "late-stage (observed)";
    case Person::UNOBSERVED_ACUTE: return "acute (unobserved)";
    case Person::UNOBSERVED_CHRONIC: return "chronic (unobserved)";
    case Person::UNOBSERVED_LATESTAGE: return "late-stage (unobserved)";
    default: throw std::runtime_error("invalid hiv status");
    }
}


double Person::updateHealthStatus(EventParams &_eventParams, ArtRolloutTracker *testTracker, CostsTracker *costsTracker)
{
	//if this person has died, then don't update.
	if(!isAlive())
	{
		return 0;
	}

	//if we haven't put this person through CEPAC, then don't bother w/ second part
	if(!wentThroughCEPAC)
	{
		return 0;
	}

	if(!_eventParams.useRollout)
	{
		//Adjust the CEPAC SimContext depending on what time it is
		if(_eventParams.itIsTimeToSwitchSimContext())
		{
			cepacPatient->setSimContext(_eventParams.cepacSimContexts[getCEPACSimContextIndex(_eventParams)]);
		}
	}

	const auto costsBefore = *_eventParams.cepacRunStats->getOverallCosts();
	const auto hivScreeningBefore = *_eventParams.cepacRunStats->getHIVScreening();

    auto treatmentBefore = isOnArt();

	//run this person's patient info one month forward in CEPAC
	cepacPatient->simulateMonth();

    auto treatmentAfter = isOnArt();

    if(treatmentBefore != treatmentAfter && _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
    {
        if(treatmentAfter)
        {
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !!# "
                << getID() << " started treatment." << std::endl;
        }
        else
        {
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !!# "
                << getID() << " stoppped treatment." << std::endl;
        }
    }

	const auto costsAfter = *_eventParams.cepacRunStats->getOverallCosts();
	const auto hivScreeningAfter = *_eventParams.cepacRunStats->getHIVScreening();
	const auto discountFactor = cepacPatient->getGeneralState()->discountFactor;

	//Update this patient's costs
	auto costThisMonthDiscounted = cepacPatient->getGeneralState()->costsDiscounted - CEPACcosts;
	CEPACcosts = cepacPatient->getGeneralState()->costsDiscounted;

	// We don't have access to the original undiscounted costs, so reverse the discounting factor
	auto costThisMonthUndiscounted = costThisMonthDiscounted / cepacPatient->getGeneralState()->discountFactor;

	if(costThisMonthUndiscounted > 0)
	{
		costsTracker->RecordCepacCosts(costThisMonthUndiscounted, costThisMonthDiscounted,
			static_cast<DemographicProfile::Gender>(getDemographicProfileVal(DemographicProfile::Demographic::Gender)), getCd4Stratum(), 
			getHVL(), getHIVStatus());

		std::array<double, SimContext::COST_NUM_TYPES> medicalCostsUndiscounted;
		for(int i = 0; i < SimContext::COST_NUM_TYPES; i++)
		{
			medicalCostsUndiscounted[i] = costsAfter.totalUndiscountedCosts[i] - costsBefore.totalUndiscountedCosts[i];
		}
		costsTracker->RecordMedicalCosts(medicalCostsUndiscounted, Scale(medicalCostsUndiscounted, discountFactor));

		std::array<double, 5> clinicalCostsDiscounted;
		clinicalCostsDiscounted[(size_t)ClinicalCostTypes::CD4Testing] = costsAfter.costsCD4Testing - costsBefore.costsCD4Testing;
		clinicalCostsDiscounted[(size_t)ClinicalCostTypes::HvlTesting] = costsAfter.costsHVLTesting - costsBefore.costsHVLTesting;
		clinicalCostsDiscounted[(size_t)ClinicalCostTypes::ClinicVisits] = costsAfter.costsClinicVisits - costsBefore.costsClinicVisits;
		clinicalCostsDiscounted[(size_t)ClinicalCostTypes::HivScreeningTests] = costsAfter.costsHIVScreeningTests - costsBefore.costsHIVScreeningTests;
		clinicalCostsDiscounted[(size_t)ClinicalCostTypes::HivScreeningMisc] = costsAfter.costsHIVScreeningMisc - costsBefore.costsHIVScreeningMisc;
		costsTracker->RecordClinicalCosts(Scale(clinicalCostsDiscounted, 1 / discountFactor), clinicalCostsDiscounted);

		if(isOnArt())
		{
			std::array<double, 3> treatmentCostsDiscounted;
			int artLine = cepacPatient->getARTState()->currRegimenNum;
			assert(artLine >= 0 && artLine < 4);
			treatmentCostsDiscounted[0] = costsAfter.directCostsARTLine[artLine] - costsBefore.directCostsARTLine[artLine];
			treatmentCostsDiscounted[1] = costsAfter.costsDrugs - costsBefore.costsDrugs;
			treatmentCostsDiscounted[2] = costsAfter.costsToxicity - costsBefore.costsToxicity;
			costsTracker->RecordTreatmentCosts(Scale(treatmentCostsDiscounted, 1 / discountFactor), treatmentCostsDiscounted, artLine);
		}
	}

	//update HVL and CD4 for this Person if they are infected
	if(isInfected())
	{
        auto cd4Before = cd4;
		cd4 = cepacPatient->getDiseaseState()->currTrueCD4;

        if(cd4Before != cd4 && _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
        {
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !!# "
                << getID() << " CD4 changed from " << cd4Before << " to " << cd4 << std::endl;
        }

        auto hvlBefore = hvl;
        auto hivStatusBefore = hivStatus;

        hvl = HvlFromCepacHvl(cepacPatient->getDiseaseState()->currTrueHVLStrata);
		currentTrueHvl = hvl;

		if(cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN)
		{
			hvl = HVL_PRIMARY;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
				hivStatus = OBSERVED_ACUTE;
			}
			else
			{
				hivStatus = UNOBSERVED_ACUTE;
			}
		}
		//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
		else if((!(cepacPatient->getARTState()->hasNextRegimenAvailable) &&
		         (!(cepacPatient->getARTState()->isOnART) || cepacPatient->getARTState()->hasObservedFailure)) &&
		        cepacPatient->getDiseaseState()->currTrueCD4 <= 50)
		{
			hvl = HVL_LATESTAGE;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
				hivStatus = OBSERVED_LATESTAGE;
			}
			else
			{
				hivStatus = UNOBSERVED_LATESTAGE;
			}
		}
		else
		{
			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
				hivStatus = OBSERVED_CHRONIC;
			}
			else
			{
				hivStatus = UNOBSERVED_CHRONIC;
			}
		}

        if(hvl != hvlBefore && _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
        {
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !!# "
                << getID() << " HVL changed from " << to_string(hvlBefore) << " to " << to_string(hvl) << std::endl;
        }

        if(hivStatus != hivStatusBefore && _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
        {
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !!# "
                << getID() << " HIV status changed from " << to_string(hivStatusBefore) << " to " << to_string(hivStatus) << std::endl;
        }

		//Update OI History
		for(int i = 0; i < Constants::NUMBER_OF_OIS; i++)
		{
			oiHistory[i] = cepacPatient->getDiseaseState()->hasTrueOIHistory[i];
		}
	}

	bool offeredTest = hivScreeningAfter.numAcceptTest > hivScreeningBefore.numAcceptTest
	                   || hivScreeningAfter.numRefuseTest > hivScreeningBefore.numRefuseTest;
	bool acceptedTest = offeredTest && hivScreeningAfter.numAcceptTest > hivScreeningBefore.numAcceptTest;
	bool returnedForResults = hivScreeningAfter.numReturnForResults > hivScreeningBefore.numReturnForResults;
	SimContext::TEST_RESULT testResult = (SimContext::TEST_RESULT)0;

	if(returnedForResults)
	{
		if(hivScreeningAfter.numTestResultsHIVNegativeType[SimContext::TEST_FALSE_POS] >
		        hivScreeningBefore.numTestResultsHIVNegativeType[SimContext::TEST_FALSE_POS])
		{
			testResult = SimContext::TEST_FALSE_POS;
		}
		else if(hivScreeningAfter.numTestResultsPrevalentType[SimContext::TEST_TRUE_POS] >
		        hivScreeningBefore.numTestResultsPrevalentType[SimContext::TEST_TRUE_POS] ||
		        (hivScreeningAfter.numTestResultsIncidentType[SimContext::TEST_TRUE_POS] >
		         hivScreeningBefore.numTestResultsIncidentType[SimContext::TEST_TRUE_POS]))
		{
			testResult = SimContext::TEST_TRUE_POS;
		}
		else if(hivScreeningAfter.numTestResultsHIVNegativeType[SimContext::TEST_TRUE_POS] >
		        hivScreeningBefore.numTestResultsHIVNegativeType[SimContext::TEST_TRUE_POS])
		{
			testResult = SimContext::TEST_TRUE_POS;
		}
		else if(hivScreeningAfter.numTestResultsPrevalentType[SimContext::TEST_FALSE_NEG] >
		        hivScreeningBefore.numTestResultsPrevalentType[SimContext::TEST_FALSE_NEG] ||
		        (hivScreeningAfter.numTestResultsIncidentType[SimContext::TEST_FALSE_NEG] >
		         hivScreeningBefore.numTestResultsIncidentType[SimContext::TEST_FALSE_NEG]))
		{
			testResult = SimContext::TEST_FALSE_NEG;
		}
	}

    if(_eventParams.trace_files[EventParams::TraceFile::Type::ArtRollout].enabled)
	{
		if(offeredTest)
		{
			testTracker->recordTest(this, acceptedTest, returnedForResults, testResult);
		}
	}

	return costThisMonthDiscounted;
}

//Call this after all transmission/population dynamics are done.
//Runs infected through CEPAC until they die and adds their LM etc to CEPAC stats
void Person::runCEPACtoDeath(RandomNumberGenerator &_randomNums)
{
	if(!isAlive())
	{
		/** If we're already dead, don't do anything */
		return;
	}

	if(!wentThroughCEPAC)
	{
		while(!rollForDeath(_randomNums))
		{
			ageOneTimeUnit();
		}
	}
	else
	{
		while(cepacPatient->isAlive())
		{
			ageOneTimeUnit();
			cepacPatient->simulateMonth();
		}

		rollForDeath(_randomNums);
	}
}

void Person::resetCondomUsage()
{
	condomsUsedThisMonth = 0;
}

void Person::resetNumActs()
{
	numActsThisMonth = 0;
}
int Person::getCondomsUsedThisMonth()
{
	return condomsUsedThisMonth;
}

int Person::getNumActsThisMonth()
{
	return numActsThisMonth;
}

void Person::incrementCondomsUsedThisMonth(int condoms)
{
	condomsUsedThisMonth += condoms;
}

void Person::incrementNumActsThisMonth(int _numActs)
{
	numActsThisMonth += _numActs;
}
bool Person::getCondomUsedLastFOICalculation()
{
	return condomUsedLastFOICalculation;
}

//-------------< Start FullVector indices related methods >----------------------//
/* @function: setFVindices
 * @arguments: vector<int> FVind, FullVector* FV
 * @effects: if this.FVindices is currently empty and all indices correlate with members
 * of FV that point to this, sets this.FVindices to FVind
 * @return: true if this.FVindices was set to FVind or false otherwise
 */
bool Person::setFVindices(vector<unsigned int> FVind, FullVector *FV)
{
	//First make sure there is no vector already associated with FV
	map<FullVector *, vector<unsigned int>>::iterator iter = FVindices.find(FV);

	if(iter == FVindices.end())
	{
		//Make sure all members of FVind are indices of FV pointing to this
		bool FVmatch = true;
		vector<unsigned int>::iterator iter;

		for(iter = FVind.begin(); iter != FVind.end(); iter++)
		{
			if(FV->at(*iter)->getID() != id)
			{
				//Set FVmatch to false if one of the members of FVind is not an index to a pointer to this in FV
				FVmatch = false;
			}
		}

		if(FVmatch)
		{
			FVindices[FV] = FVind;
			return true;
		}
	}

	return false;
}


/* @function: addFVindices
 * @arguments: int index, FullVector* FV
 * @effects: If FV[index] points to this and index is not already a member of
 * this.FVindices, adds index to this.FVindices
 * @return: true if index was added to this.FVindices or false otherwise
 */
bool Person::addFVindices(int index, FullVector *FV)
{
	if(FV->at(index)->getID() == id)
	{
		//Will be used to check if index is already in the appropriate FVindex
		bool indexAlreadyInFVindices = false;
		//Will store the appropriate FVindex;
		vector<unsigned int> *FVindex;
		//See if the FVindices for FV exists
		map<FullVector *, vector<unsigned int>>::iterator mIter = FVindices.find(FV);

		if(mIter != FVindices.end())
		{
			FVindex = &(mIter->second);
			//Check that index is not already in this.FVindices
			vector<unsigned int>::iterator iter;

			for(iter = FVindex->begin(); iter != FVindex->end(); iter++)
			{
				if(static_cast<int>(*iter) == index)
				{
					indexAlreadyInFVindices = true;
					break;
				}//if (*iter == index)
			}//for (iter = FVindex->begin(); ...
		}//if (mIter != FVindices.end())
		else
		{
			//Create new vector<unsigned int> for FV if one doesn't exist
			vector<unsigned int> newFVindex;
			FVindices[FV] = newFVindex;
			mIter = FVindices.find(FV);
			FVindex = &(mIter->second);
			assert(FVindex->empty());
		}

		if(!indexAlreadyInFVindices)
		{
			//	cout << "doing the adding..." << std::endl;
			FVindex->push_back(index);
			return true;
		}
	}

	return false;
}

/* @function: removeFVindices
 * @arguments: int index, FullVector* FV
 * @effects: If FV[index] does not point to this, removes index from this.FVindices
 * @return: true if index was removed from this.FVindices, false otherwise
 */
bool Person::removeFVindices(int index, FullVector *FV)
{
	//Check if FV[index] points to this... but first check if FV[index] is within the size of FV
	bool conditionsToRemoveAreGo = (index > (FV->size() - 1));

	if(!conditionsToRemoveAreGo)
	{
		conditionsToRemoveAreGo = (FV->at(index)->getID() != id);
	}

	if(conditionsToRemoveAreGo)
	{
		map<FullVector *, vector<unsigned int>>::iterator mIter = FVindices.find(FV);

		if(mIter != FVindices.end())
		{
			vector<unsigned int> *FVindex = &(mIter->second);
			vector<unsigned int>::iterator iter;

			for(iter = FVindex->begin(); iter != FVindex->end(); iter++)
			{
				if(static_cast<int>(*iter) == index)
				{
					FVindex->erase(iter);

					//If we've removed the last index in FVindex, remove it from the map of FVindices
					if(FVindex->empty())
					{
						FVindices.erase(mIter);
					}

					return true;
				}
			}
		}
	}

	return false;
}

/* @function: memberFVindices
 * @arguments: int index
 * @effects: none
 * @return: true iff this.FVindices contains index
 */
bool Person::memberFVindices(int index, FullVector *FV)
{
	map<FullVector *, vector<unsigned int>>::iterator mIter = FVindices.find(FV);

	if(mIter != FVindices.end())
	{
		vector<unsigned int> FVindex = mIter->second;
		vector<unsigned int>::iterator iter;

		for(iter = FVindex.begin(); iter != FVindex.end(); iter++)
		{
			if(static_cast<int>(*iter) == index)
			{
				return true;
			}
		}
	}

	return false;
}

/* @function: getFVindices
 * @arguments: none
 * @effects: none
 * @return: copy of this.FVindices
 */
vector<unsigned int> Person::getFVindices(FullVector *FV)
{
	vector<unsigned int> vcopy;
	/*map<FullVector*, vector<int> >::iterator iter = FVindices.find(FV);
	if (iter != FVindices.end()){
		vcopy.assign(iter->second.begin(), iter->second.end());
	}*/
	//ERINWASHERE
	//NEW
	vector<unsigned int> personsIndices = FVindices[FV];

	if(personsIndices.size() > 0)
	{
		vcopy.assign(personsIndices.begin(), personsIndices.end());
	}

	//ENDNEW
	return vcopy;
}

//-------------< End FullVector indices related methods >----------------------//

//-------------< Start BucketAge related methods >----------------------//
/* @function: getRiskLevel
 * @return: this.risk
 */
Person::RiskLevel Person::getRiskLevel() const
{
	return risk;
}

/* @function: getHivStatus
 * @return: this.hivStatus
 */
Person::HIVStatus Person::getHIVStatus() const
{
	return hivStatus;
}

/* @function: getSexualActivity
 * @return: this.activityLevel
 */
int Person::getSexualActivity()
{
	return activityLevel;
}

/**** Start constructors, destructors, initializers *****/

/*
Person::Person()
{
	//healthAfterInfection = nullptr;
	risk = LOW;
	traceMe = false;
	generationOfInfection = -1;
	ageInfected = -1;
	wentThroughCEPAC = false;
	cepacPatient = nullptr;
    sexualActivityDelay = 0;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		unformedPartnershipsLatestTime[type] = 0;
		unformedPartnershipsTotal[type] = 0;
		numPartnersInHistory[type] = 0;
		monthOfLatestPartnershipDissolution[type] = 0;
	}

	monthOfLatestConcurrent = 0;
	CEPACcosts = 0;
}
*/

//this constructor is used by the Male and Female classes
Person::Person(int _age, unsigned int _populationID) : sexualActivityDelay(0)
{
	id = Person::idCounter++;
	populationID = _populationID;
#ifndef TESTING

    if(!Utility::withinRange<int>(_age, 0, Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Person::maxYrForDeathStats)))
	{
		if(_age < 0)
		{
			_age = 0;
		}
		else
		{
			//cout << "SOMEONE WAS TOO OLD (" << _age << ")!  MAKING THEM " << Utility::convertTime(YEAR, MONTH, Person::maxYrForDeathStats) << "!" << std::endl;
            _age = Utility::convertTime(TimeGranularity::Year, TimeGranularity::Month, Person::maxYrForDeathStats);
		}
	}

#endif
	age = _age;
	initAge = _age;
	ageInfected = -1;
	death = false;
	deathStatus = ALIVE;
	sexualActivityLevel = 1.0;
	//healthAfterInfection = nullptr;
	cepacPatient = nullptr;
	CEPACcosts = 0;
	wentThroughCEPAC = false;
	generationOfInfection = -1;
	stats.init(&Person::StatsEnum);
	traceMe = false;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		unformedPartnershipsLatestTime[type] = 0;
		unformedPartnershipsTotal[type] = 0;
		numPartnersInHistory[type] = 0;
		monthOfLatestPartnershipDissolution[type] = 0;
	}

	monthOfLatestConcurrent = 0;
	resetNumActs();
	//set the person's initial demographic profile. gender is set within the Male/Female constructors
	//everyone is set as NA, but you can call becomeSexuallyActive(_eventParams) elsewhere if you want this person to be SA
    dmgProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
    dmgProfile.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)DemographicProfile::SexualOrientation::Heterosexual);
    dmgProfile.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::Single);
	//everyone is set as NON_CSW, but you can call becomeCSW() elsewhere if you want this person to be CSW
    dmgProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
	hivStatus = NEGATIVE;
	cd4 = -1;
	hvl = UNINFECTED;
	//dmgProfile -- default constructor sets everything to wildcards
	//this person isn't a member of any bucket yet. this value will be changed when EntityPool adds or removes the person
	currentBucketID = DemographicProfile::END;
}

Person::~Person(void)
{
	//If this person went through CEPAC, delete their CEPACpatient
	//TODO: If they're not dead, force kill them (in CEPAC) to log the stats (?)
	//Didn't I do this somewhere?
	delete cepacPatient;
	//take person out of all current relationships
	list<SexualPartnership *>::iterator toDelete;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		list<SexualPartnership *>::iterator iter = partners[type].begin();
		list<SexualPartnership *>::iterator iterEnd = partners[type].end();

		while(iter != iterEnd)
		{
			toDelete = iter;
			iter++;
			delete(*toDelete);
		}
	}
}

void Person::deletePersonWithoutDeleting()
{
	//Don't delete the cepacPatient -- this causes a weird exception when you try to delete it at the close of simulation, so keep it around
	//take person out of all current relationships
	list<SexualPartnership *>::iterator toDelete;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		list<SexualPartnership *>::iterator iter = partners[type].begin();
		list<SexualPartnership *>::iterator iterEnd = partners[type].end();

		while(iter != iterEnd)
		{
			toDelete = iter;
			iter++;
			delete(*toDelete);
		}
	}

	std::map<FullVector *, vector<unsigned int>>::iterator FViter = FVindices.begin();

	while(FViter != FVindices.end())
	{
		FViter->first->remove(this);
		FViter++;
	}
}

/**** End constructors, destructors, initializers *****/

int Person::getCEPACSimContextIndex(EventParams &_eventParams)
{
	int returnValue = 0;

    for(std::size_t i = 0; i < _eventParams.cepacSimContexts.size(); i++)
	{
		if(_eventParams.currTime > _eventParams.timesToSwitchSimContext[i])
		{
			returnValue = static_cast<int>(i);
		}
	}

	return returnValue;
}
