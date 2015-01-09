#include "entity.hpp"
#include "male.hpp"
#include "female.hpp"
#include "msm.hpp"
#include "msmw.hpp"
#include "sexualpartnership.hpp"
#include "core/constants.hpp"
#include "parameters/eventparams.hpp"
#include "utility/utility.hpp"
#include "utility/randomnumbergenerator.hpp"
#include "statistics/infectionstracker.hpp"
#include "statistics/artrollouttracker.hpp"
#include "statistics/coststracker.hpp"

namespace transm {

class EntityPool;

long Entity::idCounter = 0;
int Entity::numTracesSoFar = 0;

//This is pretty much only used by the NA folks who are NA at the end of the model and need to have their LMs added to total
//TODO: But maybe they shouldn't?
std::vector<double> Entity::probDeathNatCauses[(std::size_t)DemographicProfile::Gender::Last];

const std::vector<std::string> Entity::StatsStr =
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

EnumCls<Entity::Stats> Entity::StatsEnum(Entity::StatsStr);

void Entity::ageOneTimeUnit()
{
	age++;
}

Entity::CD4Strata Entity::getCd4Stratum() const
{
	switch(cepacPatient->getDiseaseState()->currTrueCD4Strata)
	{
	case SimContext::CD4_VLO:
		return CD4Strata::CD4_ZERO;
	case SimContext::CD4__LO:
        return CD4Strata::CD4_ONE;
	case SimContext::CD4_MLO:
        return CD4Strata::CD4_TWO;
	case SimContext::CD4_MHI:
        return CD4Strata::CD4_THREE;
	case SimContext::CD4__HI:
        return CD4Strata::CD4_FOUR;
	case SimContext::CD4_VHI:
        return CD4Strata::CD4_FIVE;
	default:
        return CD4Strata::Last;
	}
}


    template<>
    DemographicProfile::Gender Entity::getDemographicProfileVal() const { return (DemographicProfile::Gender)getDemographicProfileVal(DemographicProfile::Demographic::Gender); }

    template<>
    DemographicProfile::SexualActivityStatus Entity::getDemographicProfileVal() const 
    { 
        return (DemographicProfile::SexualActivityStatus)getDemographicProfileVal(DemographicProfile::Demographic::SexualActivityStatus); 
    }

    template<>
    DemographicProfile::Employment Entity::getDemographicProfileVal() const
    {
        return (DemographicProfile::Employment)getDemographicProfileVal(DemographicProfile::Demographic::Employment);
    }

	template<>
	DemographicProfile::RelationshipStatus Entity::getDemographicProfileVal() const
	{
		return (DemographicProfile::RelationshipStatus)getDemographicProfileVal(DemographicProfile::Demographic::RelationshipStatus);
	}

bool Entity::isEligibleForTreatment(const SimContext::TreatmentInputs::ARTStartPolicy &artStartPolicy)
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

Entity *Entity::allPartnerSexualActivity(EventParams &_eventParams, SexualPartnership::Type _partnershipType,
    std::list<Entity *> &_newlyInfected, InfectionsTracker *infTrack, 
    const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &transmission_coefficients)
{
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	//iterate through all partnerships of SexualActivity::Type _partnershipType and have them engage in sexual activity
	auto iter = partners[(int)_partnershipType].begin();
	auto iterEnd = partners[(int)_partnershipType].end();
	//becomes non-nullptr only when this person gets infected. We are saving the partner who infected this person
	Entity *infectedMe = nullptr;

	while(iter != iterEnd)
	{
		//initiate sexual activity only if you are partner1
		if((*iter)->getPartner1() == this)
		{
			Entity *infected = (*iter)->monthlySexualActivity(_eventParams, infTrack, transmission_coefficients);

			//if you or your partners got infected, the infected joins the _newlyInfected list
			if(infected != nullptr)
			{
				_newlyInfected.push_back(infected);

				//if you got infected, then you have to save the person who infected you for record keeping
				if(infected == this)
				{
					infectedMe = (*iter)->getOtherPartner(this);
				}
			}
		}

		iter++;
	}

	return infectedMe;
}

bool Entity::availableForPartnership(SexualPartnership::Type _partnershipType) const
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

void Entity::addPartnership(SexualPartnership *_partnership)
{
	assert(_partnership != nullptr);
	assert((_partnership->getPartner1() != nullptr));
	assert(_partnership->getPartner1()->isAlive());
	assert((_partnership->getPartner2() != nullptr));
	assert((_partnership->getPartner2()->isAlive()));
	partners[(int)_partnership->getType()].push_back(_partnership);
	numPartnersInHistory[(int)_partnership->getType()]++;

	monthOfLatestPartnershipDissolution[(int)_partnership->getType()] =
		max(monthOfLatestPartnershipDissolution[(int)_partnership->getType()], _partnership->getTimeOfDissolution());

	//if a STEADY partnership was added && we are SINGLE, the we need to change or RELATIONSHIP_STATUS
	if((_partnership->getType() == SexualPartnership::Type::Steady) &&
		(!partners[(int)SexualPartnership::Type::Steady].empty()) &&
        (getDemographicProfileVal(DemographicProfile::Demographic::RelationshipStatus) == (std::size_t)DemographicProfile::RelationshipStatus::Single))
	{
        dmgProfile.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::NonSingle);
	}
}

void Entity::becomeInfected(int _generationOfInfection, EventParams &_eventParams)
{
    //hvl needs to be set even for people who are about to go through CEPAC so that isInfected() correctly returns true
	hvl = HVLStrata::HVL_PRIMARY;
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
	}

	stats.setStat(Entity::Stats::STAT_TIME_OF_INFECTION_MTH, _eventParams.currTime.get_total_months());
    stats.setStat(Entity::Stats::STAT_AGE_AT_INFECTION_MTH, getAge().get_total_months());
    stats.setStat(Entity::Stats::STAT_GENERATION_OF_INFECTION, _generationOfInfection);

	//if a CEPAC person exists (i.e. they were created earlier and thus this is an incident case), set them to infected
	if(wentThroughCEPAC)
	{
		cepacPatient->forceNewInfection();
		cd4 = cepacPatient->getDiseaseState()->currTrueCD4;
		//update HVL state
		SimContext::HVL_STRATA hvlStrata = cepacPatient->getDiseaseState()->currTrueHVLStrata;

		if(hvlStrata == SimContext::HVL_VLO)
		{
			hvl = HVLStrata::HVL_ZERO;    //0-20
		}
		else if(hvlStrata == SimContext::HVL__LO)
		{
            hvl = HVLStrata::HVL_ONE;    //21-500
		}
		else if(hvlStrata == SimContext::HVL_MLO)
		{
            hvl = HVLStrata::HVL_TWO;    //501-3000
		}
		else if(hvlStrata == SimContext::HVL_MED)
		{
            hvl = HVLStrata::HVL_THREE;    //3001-10000
		}
		else if(hvlStrata == SimContext::HVL_MHI)
		{
            hvl = HVLStrata::HVL_FOUR;    //10001-30000
		}
		else if(hvlStrata == SimContext::HVL__HI)
		{
            hvl = HVLStrata::HVL_FIVE;    //30001-100000
		}
		else if(hvlStrata == SimContext::HVL_VHI)
		{
            hvl = HVLStrata::HVL_SIX;    //100000+
		}
		else
		{
		    throw std::runtime_error("Invalid CEPAC API infection state: " + std::string((SimContext::HVL_STRATA_STRS[hvlStrata])));
		}

		currentTrueHvl = hvl;

		if(cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN)
		{
            hvl = HVLStrata::HVL_PRIMARY;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
                hivStatus = HIVStatus::OBSERVED_ACUTE;
			}
			else
			{
                hivStatus = HIVStatus::UNOBSERVED_ACUTE;
			}
		}
		//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
		else if((!(cepacPatient->getARTState()->hasNextRegimenAvailable) &&
		         (!(cepacPatient->getARTState()->isOnART) || cepacPatient->getARTState()->hasObservedFailure)) &&
		        cepacPatient->getDiseaseState()->currTrueCD4 <= 50)
		{
            hvl = HVLStrata::HVL_LATESTAGE;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
                hivStatus = HIVStatus::OBSERVED_LATESTAGE;
			}
			else
			{
                hivStatus = HIVStatus::UNOBSERVED_LATESTAGE;
			}
		}
		else
		{
			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
                hivStatus = HIVStatus::OBSERVED_CHRONIC;
			}
			else
			{
                hivStatus = HIVStatus::UNOBSERVED_CHRONIC;
			}
		}

		//Update OI History
		for(int i = 0; i < Constants::NumberOfOIs; i++)
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

void Entity::initialCEPACpatient(EventParams &_eventParams)
{
	// Only initialize the person if they haven't already been initialized!  
    // Prevalent cases will get called to initialize twice!
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
			true, (int)getAge().get_total_months(), cepacGender, setAsIncidentCase, (int)_eventParams.currTime.get_total_months());

		//Only update hvl and cd4 if the patient is infected
		//update HVL and CD4  and infection status for this Entity if they are infected
		if(isInfected())
		{
			cd4 = cepacPatient->getDiseaseState()->currTrueCD4;
			//update HVL state
			SimContext::HVL_STRATA hvlStrata = cepacPatient->getDiseaseState()->currTrueHVLStrata;

			if(hvlStrata == SimContext::HVL_VLO)
			{
				hvl = HVLStrata::HVL_ZERO;    //0-20
			}
			else if(hvlStrata == SimContext::HVL__LO)
			{
                hvl = HVLStrata::HVL_ONE;    //21-500
			}
			else if(hvlStrata == SimContext::HVL_MLO)
			{
                hvl = HVLStrata::HVL_TWO;    //501-3000
			}
			else if(hvlStrata == SimContext::HVL_MED)
			{
                hvl = HVLStrata::HVL_THREE;    //3001-10000
			}
			else if(hvlStrata == SimContext::HVL_MHI)
			{
                hvl = HVLStrata::HVL_FOUR;    //10001-30000
			}
			else if(hvlStrata == SimContext::HVL__HI)
			{
                hvl = HVLStrata::HVL_FIVE;    //30001-100000
			}
			else if(hvlStrata == SimContext::HVL_VHI)
			{
                hvl = HVLStrata::HVL_SIX;    //100000+
			}
			else
			{
			    throw std::runtime_error("Invalid CEPAC API infection state: " + std::string((SimContext::HVL_STRATA_STRS[hvlStrata])));
			}

			currentTrueHvl = hvl;

			if(cepacPatient->getDiseaseState()->infectedHIVState == SimContext::HIV_INF_ACUTE_SYN)
			{
                hvl = HVLStrata::HVL_PRIMARY;

				if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				{
					hivStatus = HIVStatus::OBSERVED_ACUTE;
				}
				else
				{
                    hivStatus = HIVStatus::UNOBSERVED_ACUTE;
				}
			}
			//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
			else if((!(cepacPatient->getARTState()->hasNextRegimenAvailable) &&
			         (!(cepacPatient->getARTState()->isOnART) || cepacPatient->getARTState()->hasObservedFailure)) &&
			        cepacPatient->getDiseaseState()->currTrueCD4 <= 50)
			{
                hvl = HVLStrata::HVL_LATESTAGE;

				if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				{
                    hivStatus = HIVStatus::OBSERVED_LATESTAGE;
				}
				else
				{
                    hivStatus = HIVStatus::UNOBSERVED_LATESTAGE;
				}
			}
			else
			{
				if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
				{
                    hivStatus = HIVStatus::OBSERVED_CHRONIC;
				}
				else
				{
                    hivStatus = HIVStatus::UNOBSERVED_CHRONIC;
				}
			}

			//Update OI History
			for(int i = 0; i < Constants::NumberOfOIs; i++)
			{
				oiHistory[i] = cepacPatient->getDiseaseState()->hasTrueOIHistory[i];
			}
		}

		Entity::numTracesSoFar++;
	}
}

int Entity::getGenerationOfInfection(bool cap_at_5) const
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

int Entity::getNumPartners(SexualPartnership::Type _type)
{
	return (int)partners[(int)_type].size();
}

int Entity::getNumPartners(SexualPartnership::Type _type, bool sameRisk)
{
	int numPartners = 0;

	for(std::list<SexualPartnership *>::iterator partnerIter = partners[(int)_type].begin();
		partnerIter != partners[(int)_type].end(); partnerIter++)
	{
		Entity *partner;

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

int Entity::getNumPartnersInHistory(SexualPartnership::Type _type)
{
	return numPartnersInHistory[(int)_type];
}
int Entity::getNumPartnersInHistory()
{
	int total = 0;

	for(int i = 0; i < (int)SexualPartnership::Type::ENDType; i++)
	{
		total += numPartnersInHistory[i];
	}

	return total;
}
Time Entity::getMonthOfLatestPartnershipDissolution(SexualPartnership::Type _type)
{
	return monthOfLatestPartnershipDissolution[(int)_type];
}
Time Entity::getTimeOfLatestConcurrent()
{
	return monthOfLatestConcurrent;
}
void Entity::setTimeOfLatestConcurrent(Time _month)
{
	monthOfLatestConcurrent = _month;
}
void Entity::becomeSexuallyActive(EventParams &_eventParams)
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

Time Entity::getAge() const
{
	return age;
}


DemographicProfile::ProfileID Entity::getCurrBucketProfileID()
{
	return currentBucketID;
}

const DemographicProfile *Entity::getDemographicProfile() const
{
	return &dmgProfile;
}

BaseEnumCls::Enum Entity::getDemographicProfileVal(DemographicProfile::Demographic _demographic) const
{
	return dmgProfile.get(_demographic);
}

unsigned long Entity::getID() const
{
	return id;
}

long Entity::getPartnershipsToEnd(Time _currTime, SexualPartnership::Type _partnershipType,
                                  std::list<SexualPartnership *> &_partnershipsToEnd, bool _fromDeath)
{
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	assert((_currTime >= Time::Zero) || _fromDeath);

	if(partners[(int)_partnershipType].size() == 0)
	{
		return 0;
	}

	//iterate through all current partnerships that had any duration to them.
	//The iterator points to class SexualPartnership
	auto iter = partners[(int)_partnershipType].begin();
	auto iterEnd = partners[(int)_partnershipType].end();
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
	}

	return numEnded;
}

//Begin Unformed Partnership helper methods
int Entity::getTotalUnformedPartnerships(SexualPartnership::Type type)
{
	return unformedPartnershipsTotal[(int)type];
}

int Entity::getLatestUnformedPartnerships(SexualPartnership::Type type)
{
	return unformedPartnershipsLatestTime[(int)type];
}

void Entity::increaseUnformedPartnershipTallies(SexualPartnership::Type type)
{
	unformedPartnershipsLatestTime[(int)type] += 1;
	unformedPartnershipsTotal[(int)type] += 1;
}

void Entity::resetLatestUnformedPartnerships(SexualPartnership::Type type)
{
	unformedPartnershipsLatestTime[(int)type] = 0;
}

//End Unformed Partnership helper methods

unsigned int Entity::getPopulationID()
{
	return populationID;
}

//returns traceMe
bool Entity::trace()
{
	return traceMe;
}
//sets traceMe to true
void Entity::setToBeTraced()
{
	traceMe = true;
}

const Entity::EntityStatsRecord *Entity::getStats()
{
	return &stats;
}

bool Entity::inCorrectBucketDemographicProfile()
{
	return (dmgProfile.getProfileID() == currentBucketID);
}

bool Entity::isAlive() const
{
	if(this == nullptr)
	{
		return false;
	}

	return !death;
}

bool Entity::isPartneredWith(Entity *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());

	for(int partnershipType = 0; partnershipType < (int)SexualPartnership::Type::ENDType; ++partnershipType)
	{
		//iterate through each partnership and check if _p is a member of one of them
		auto iter = partners[(int)partnershipType].begin();
		auto endIter = partners[(int)partnershipType].end();

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

bool Entity::hasPartnership(SexualPartnership::Type partnershipType)
{
	if(partners[(int)partnershipType].size() > 0)
	{
		return true;
	}

	return false;
}

bool Entity::isInfected()
{
    return hvl != HVLStrata::UNINFECTED;
}

void Entity::removePartnership(SexualPartnership *_partnership)
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

void Entity::rollForBecomeSexWorker(EventParams &_eventParams, bool _isInit, double initialProb)
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

void Entity::quitSexWork(EventParams &_eventParams)
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
bool Entity::rollForDeath(RandomNumberGenerator &_randomNums)
{
	assert(death == false);

	//if person is too old, then they automatically die
    if(getAge().get_total_years() >= Entity::maxYrForDeathStats)
	{
		death = true;
		deathStatus = DeathStatus::DTH_OTHER;
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
                deathStatus = DeathStatus::DTH_OI;    //oi death
			}
			else if(causeOfDeath == SimContext::DTH_CHRAIDS)
			{
                deathStatus = DeathStatus::DTH_CHRAIDS;
			}
			else if(causeOfDeath == SimContext::DTH_NONAIDS)
			{
                deathStatus = DeathStatus::DTH_NONAIDS;
			}
			else if(causeOfDeath == SimContext::DTH_TOX_ART)
			{
                deathStatus = DeathStatus::DTH_TOX_ART;
			}
			else if(causeOfDeath == SimContext::DTH_TOX_PROPH)
			{
                deathStatus = DeathStatus::DTH_TOX_PROPH;
			}
			else
			{
                deathStatus = DeathStatus::DTH_OTHER;
			}
		}
	}
	//There may be those remaining that didn't go through CEPAC: the non-sexually actives!
	else
	{
        auto gender = getDemographicProfileVal<DemographicProfile::Gender>();
        auto age = getAge();
        double deathRate = probDeathNatCauses[(std::size_t)gender].at((std::size_t)age.get_total_months());

        if(_randomNums.chance(deathRate))
		{
            death = true;
			deathStatus = DeathStatus::DTH_NONAIDS;
		}
	}

	//if they died, collect statistics
	if(death)
	{
        stats.setStat(Stats::STAT_TOTAL_LM, getAge().get_total_months());
        stats.setStat(Stats::STAT_HIV_NEG_LM, getAge().get_total_months() - (isInfected() ? stats.getStat(Stats::STAT_TIME_OF_INFECTION_MTH) : 0));
        stats.setStat(Stats::STAT_HIV_POS_POSTINFECT_LM, stats.getStat(Stats::STAT_TOTAL_LM) - stats.getStat(Stats::STAT_AGE_AT_INFECTION_MTH));
	}

	return death;
}

void Entity::setCurrBucketProfileID(DemographicProfile::ProfileID _profileID)
{
	currentBucketID = _profileID;
}

void Entity::setSimContext(SimContext *newSimContext)
{
	cepacPatient->setSimContext(newSimContext);
}

Entity *Entity::sexualActivity(Entity *_p, int _numActs, 
    SexualPartnership::Type _partnershipType, EventParams &_eventParams, 
    InfectionsTracker *infTrack, 
    const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &transmission_coefficients)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	//TODO: CONDOM STUFF!

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
	{
		if(trace())
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] 
                << "# " << getEntityType() << " " << getID() << " engages in " 
                << _numActs << " acts with his " 
                << (SexualPartnership::TypeStrings.at(_partnershipType)) 
                << " " << _p->getID() << std::endl;
		}
        else if(_p->trace())
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson]
                << "# " << _p->getEntityType() << " " << _p->getID() << " engages in "
                << _numActs << " acts with her "
                << (SexualPartnership::TypeStrings.at(_partnershipType))
                << " " << getID() << std::endl;
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
	Entity *infected = isInfected() ? this : _p;
	Entity *uninfected = isInfected() ? _p : this;
	bool transmissionOccured = false;

	for(int i = 0; i < _numActs; i++)
	{
		/** Regardless of infection, record the exposure */
		infTrack->recordExposure(_eventParams.currTime, infected);
		//force of infection from infected to uninfected
		double foifPerEvent = infected->getFOI(uninfected, transmission_coefficients, _partnershipType, _eventParams);

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
            stats.incrStat(Entity::Stats::STAT_NUM_INFECTED, 1);
		}
		else
		{
            _p->stats.incrStat(Entity::Stats::STAT_NUM_INFECTED, 1);
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
        uninfected->stats.incrStat(Entity::Stats::STAT_EXPOSURES_BEFORE_INF, _numActs);
	}

	return nullptr;
}

template<typename T>
T Scale(const T &t, double factor)
{
	T r;
	std::transform(t.begin(), t.end(), r.begin(), std::bind1st(std::multiplies<double>(), factor));
	return r;
}

Entity::HVLStrata HvlFromCepacHvl(SimContext::HVL_STRATA stratum)
{
    switch(stratum)
    {
    case SimContext::HVL_VLO: return Entity::HVLStrata::HVL_ZERO;
    case SimContext::HVL__LO: return Entity::HVLStrata::HVL_ONE;
    case SimContext::HVL_MLO: return Entity::HVLStrata::HVL_TWO;
    case SimContext::HVL_MED: return Entity::HVLStrata::HVL_THREE;
    case SimContext::HVL_MHI: return Entity::HVLStrata::HVL_FOUR;
    case SimContext::HVL__HI: return Entity::HVLStrata::HVL_FIVE;
    case SimContext::HVL_VHI: return Entity::HVLStrata::HVL_SIX;
    default: throw std::runtime_error("invalid hvl");
    }
}

std::string to_string(Entity::HVLStrata stratum)
{
    switch(stratum)
    {
    case Entity::HVLStrata::UNINFECTED: return "uninfected";
    case Entity::HVLStrata::HVL_ZERO: return "0-20";
    case Entity::HVLStrata::HVL_ONE: return "21-500";
    case Entity::HVLStrata::HVL_TWO: return "501-3000";
    case Entity::HVLStrata::HVL_THREE: return "3001-10000";
    case Entity::HVLStrata::HVL_FOUR: return "10001-30000";
    case Entity::HVLStrata::HVL_FIVE: return "30001-100000";
    case Entity::HVLStrata::HVL_SIX: return "100000+";
    case Entity::HVLStrata::HVL_PRIMARY: return "primary";
    case Entity::HVLStrata::HVL_LATESTAGE: return "late-stage";
    default: throw std::runtime_error("invalid hvl");
    }
}

std::string to_string(Entity::HIVStatus status)
{
    switch(status)
    {
    case Entity::HIVStatus::NEGATIVE: return "negative";
    case Entity::HIVStatus::OBSERVED_ACUTE: return "acute (observed)";
    case Entity::HIVStatus::OBSERVED_CHRONIC: return "chronic (observed)";
    case Entity::HIVStatus::OBSERVED_LATESTAGE: return "late-stage (observed)";
    case Entity::HIVStatus::UNOBSERVED_ACUTE: return "acute (unobserved)";
    case Entity::HIVStatus::UNOBSERVED_CHRONIC: return "chronic (unobserved)";
    case Entity::HIVStatus::UNOBSERVED_LATESTAGE: return "late-stage (unobserved)";
    default: throw std::runtime_error("invalid hiv status");
    }
}


double Entity::updateHealthStatus(EventParams &_eventParams, ArtRolloutTracker *testTracker, CostsTracker *costsTracker)
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
		add_cepac_cost(costThisMonthUndiscounted, costThisMonthDiscounted);
		costsTracker->RecordCepacCosts(costThisMonthUndiscounted, costThisMonthDiscounted,
			getEntityType(), getCd4Stratum(), 
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

	//update HVL and CD4 for this Entity if they are infected
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
			hvl = HVLStrata::HVL_PRIMARY;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
                hivStatus = HIVStatus::OBSERVED_ACUTE;
			}
			else
			{
                hivStatus = HIVStatus::UNOBSERVED_ACUTE;
			}
		}
		//Late stage is defined as having failed the last ART regimen (or having no art regimens to start with) and a CD4 <= 50
		else if((!(cepacPatient->getARTState()->hasNextRegimenAvailable) &&
		         (!(cepacPatient->getARTState()->isOnART) || cepacPatient->getARTState()->hasObservedFailure)) &&
		        cepacPatient->getDiseaseState()->currTrueCD4 <= 50)
		{
            hvl = HVLStrata::HVL_LATESTAGE;

			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
                hivStatus = HIVStatus::OBSERVED_LATESTAGE;
			}
			else
			{
                hivStatus = HIVStatus::UNOBSERVED_LATESTAGE;
			}
		}
		else
		{
			if(cepacPatient->getMonitoringState()->isDetectedHIVPositive)
			{
                hivStatus = HIVStatus::OBSERVED_CHRONIC;
			}
			else
			{
                hivStatus = HIVStatus::UNOBSERVED_CHRONIC;
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
		for(int i = 0; i < Constants::NumberOfOIs; i++)
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
void Entity::runCEPACtoDeath(RandomNumberGenerator &_randomNums)
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

void Entity::resetCondomUsage()
{
	condomsUsedThisMonth = 0;
}

void Entity::resetNumActs()
{
	numActsThisMonth = 0;
}
int Entity::getCondomsUsedThisMonth()
{
	return condomsUsedThisMonth;
}

int Entity::getNumActsThisMonth()
{
	return numActsThisMonth;
}

void Entity::incrementCondomsUsedThisMonth(int condoms)
{
	condomsUsedThisMonth += condoms;
}

void Entity::incrementNumActsThisMonth(int _numActs)
{
	numActsThisMonth += _numActs;
}
bool Entity::getCondomUsedLastFOICalculation()
{
	return condomUsedLastFOICalculation;
}

/* @function: setFVindices
 * @arguments: vector<int> FVind, FullVector* FV
 * @effects: if this.FVindices is currently empty and all indices correlate with members
 * of FV that point to this, sets this.FVindices to FVind
 * @return: true if this.FVindices was set to FVind or false otherwise
 */
bool Entity::setFVindices(std::vector<unsigned int> FVind, FullVector *FV)
{
	//First make sure there is no vector already associated with FV
    if(FVindices.find(FV) == FVindices.end())
	{
		//Make sure all members of FVind are indices of FV pointing to this
		bool FVmatch = true;
		
		for(auto iter = FVind.begin(); iter != FVind.end(); iter++)
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
bool Entity::addFVindices(int index, FullVector *FV)
{
	if(FV->at(index)->getID() == id)
	{
		//Will be used to check if index is already in the appropriate FVindex
		bool indexAlreadyInFVindices = false;
		//Will store the appropriate FVindex;
		std::vector<unsigned int> *FVindex;
		//See if the FVindices for FV exists
		auto mIter = FVindices.find(FV);

		if(mIter != FVindices.end())
		{
			FVindex = &(mIter->second);

			//Check that index is not already in this.FVindices
			for(auto iter = FVindex->begin(); iter != FVindex->end(); iter++)
			{
				if(static_cast<int>(*iter) == index)
				{
					indexAlreadyInFVindices = true;
					break;
				}
			}
		}
		else
		{
			//Create new vector<unsigned int> for FV if one doesn't exist
			std::vector<unsigned int> newFVindex;
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
bool Entity::removeFVindices(int index, FullVector *FV)
{
	//Check if FV[index] points to this... but first check if FV[index] is within the size of FV
	bool conditionsToRemoveAreGo = (index > (FV->size() - 1));

	if(!conditionsToRemoveAreGo)
	{
		conditionsToRemoveAreGo = (FV->at(index)->getID() != id);
	}

	if(conditionsToRemoveAreGo)
	{
		auto mIter = FVindices.find(FV);

		if(mIter != FVindices.end())
		{
			auto FVindex = &(mIter->second);

			for(auto iter = FVindex->begin(); iter != FVindex->end(); iter++)
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
bool Entity::memberFVindices(int index, FullVector *FV)
{
	auto mIter = FVindices.find(FV);

	if(mIter != FVindices.end())
	{
		auto FVindex = mIter->second;

		for(auto iter = FVindex.begin(); iter != FVindex.end(); iter++)
		{
			if(static_cast<int>(*iter) == index)
			{
				return true;
			}
		}
	}

	return false;
}

//XXX: lots of passing vectors by value around here, check on performance!
/* @function: getFVindices
 * @arguments: none
 * @effects: none
 * @return: copy of this.FVindices
 */
std::vector<unsigned int> Entity::getFVindices(FullVector *FV)
{
	std::vector<unsigned int> vcopy;
	std::vector<unsigned int> personsIndices = FVindices[FV];

	if(personsIndices.size() > 0)
	{
		vcopy.assign(personsIndices.begin(), personsIndices.end());
	}

	return vcopy;
}

Entity::RiskLevel Entity::getRiskLevel() const
{
	return risk;
}

Entity::HIVStatus Entity::getHIVStatus() const
{
	return hivStatus;
}

int Entity::getSexualActivity()
{
	return activityLevel;
}

//this constructor is used by the Male and Female classes
Entity::Entity(Age _age, unsigned int _populationID) : 
	monthly_cepac_costs_undiscounted_(0),
	monthly_cepac_costs_discounted_(0),
	monthly_cdm_costs_undiscounted_(0),
	monthly_cdm_costs_discounted_(0)
{
	id = Entity::idCounter++;
	populationID = _populationID;

	if (_age < Time::Zero || _age > Time(Entity::maxYrForDeathStats, 0))
	{
        throw std::runtime_error("invalid age");
	}

	age = _age;
	initAge = _age;
	ageInfected = Age(0, -1);
	death = false;
	deathStatus = DeathStatus::ALIVE;
	sexualActivityLevel = 1.0;
	cepacPatient = nullptr;
	CEPACcosts = 0;
	wentThroughCEPAC = false;
	generationOfInfection = -1;
	stats.init(&Entity::StatsEnum);
	traceMe = false;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		unformedPartnershipsLatestTime[type] = 0;
		unformedPartnershipsTotal[type] = 0;
		numPartnersInHistory[type] = 0;
		monthOfLatestPartnershipDissolution[type] = Time::Zero;
	}

	monthOfLatestConcurrent = Time::Zero;
	resetNumActs();
	//set the person's initial demographic profile. gender is set within the Male/Female constructors
	//everyone is set as NA, but you can call becomeSexuallyActive(_eventParams) elsewhere if you want this person to be SA
    dmgProfile.set(DemographicProfile::Demographic::SexualActivityStatus, (std::size_t)DemographicProfile::SexualActivityStatus::NotActive);
    dmgProfile.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)DemographicProfile::SexualOrientation::Heterosexual);
    dmgProfile.set(DemographicProfile::Demographic::RelationshipStatus, (std::size_t)DemographicProfile::RelationshipStatus::Single);
	//everyone is set as NON_CSW, but you can call becomeCSW() elsewhere if you want this person to be CSW
    dmgProfile.set(DemographicProfile::Demographic::Employment, (std::size_t)DemographicProfile::Employment::NonCsw);
    hivStatus = HIVStatus::NEGATIVE;
	cd4 = -1;
	hvl = HVLStrata::UNINFECTED;
	//dmgProfile -- default constructor sets everything to wildcards
	//this person isn't a member of any bucket yet. this value will be changed when EntityPool adds or removes the person
	currentBucketID = DemographicProfile::END;
}

Entity::~Entity(void)
{
	//If this person went through CEPAC, delete their CEPACpatient
	//TODO: If they're not dead, force kill them (in CEPAC) to log the stats (?)
	//Didn't I do this somewhere?
	delete cepacPatient;

	//take person out of all current relationships
	std::list<SexualPartnership *>::iterator toDelete;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		auto iter = partners[type].begin();
		auto end = partners[type].end();

		while(iter != end)
		{
			toDelete = iter;
			iter++;
			delete(*toDelete);
		}
	}
}

void Entity::deleteEntityWithoutDeleting()
{
	//Don't delete the cepacPatient -- this causes a weird exception when you try to delete it at the close of simulation, so keep it around
	//take person out of all current relationships
	std::list<SexualPartnership *>::iterator toDelete;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
        std::list<SexualPartnership *>::iterator iter = partners[type].begin();
        std::list<SexualPartnership *>::iterator end = partners[type].end();

		while(iter != end)
		{
			toDelete = iter;
			iter++;
			delete(*toDelete);
		}
	}

	auto FViter = FVindices.begin();

	while(FViter != FVindices.end())
	{
		FViter->first->remove(this);
		FViter++;
	}
}

int Entity::getCEPACSimContextIndex(EventParams &_eventParams)
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

} // namespace transm
