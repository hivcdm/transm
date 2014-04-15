#include "Person.h"
#include "Male.h"
#include "Female.h"
#include "classifiers/SexualPartnership.h"
#include "../Constants.h"
#include "../data/EventParams.h"
#include "../util/Util.h"
#include "../util/rand/RandomNums.h"
#include "../statistics/InfectionsTracker.h"
#include "../statistics/ArtRolloutTracker.h"
#include "../statistics/CostsTracker.h"

class EntityPool;

long Person::idCounter = 0;
int Person::numTracesSoFar = 0;

//This is pretty much only used by the NA folks who are NA at the end of the model and need to have their LMs added to total
//TODO: But maybe they shouldn't?
vector<double> Person::probDeathNatCauses[DmgProfile::ENDGender];

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
	        (getDmgProfileVal(DmgProfile::RELATIONSHIP_STATUS) == DmgProfile::SINGLE))
	{
		dmgProfile.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::NON_SINGLE);
	}
}

void Person::becomeInfected(int _generationOfInfection, EventParams &_eventParams)
{
	hvl =
	    HVL_PRIMARY;	//hvl needs to be set even for people who are about to go through CEPAC so that isInfected() correctly returns true
	cd4 = -1;			//CD4 doesn't affect much in the transmission model yet... will be updated with CEPAC
	ageInfected = age;
	generationOfInfection = _generationOfInfection;

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && trace())
	{
		if(getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "@ Male ";
		}
		else
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "@ Female ";
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << getID() << " has ";

		if(_generationOfInfection == 0)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "a prevalent case of HIV";
		}
		else
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "an incident case of HIV";
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "!" << endl;
	}

	stats.setStat(Person::STAT_TIME_OF_INFECTION_MTH, _eventParams.currTime);
	stats.setStat(Person::STAT_AGE_AT_INFECTION_MTH, getAge(MONTH));
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
			cerr << "Invalid CEPAC API infection state: " << *(SimContext::HVL_STRATA_STRS[hvlStrata]);
			Util::exitWithPrompt(-1);
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

	/** Update the graphNode to have value for time of infection */
	graphNode->timeInfected = _eventParams.currTime;
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

		if(getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
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
		                                 true, getAge(MONTH), cepacGender, setAsIncidentCase, _eventParams.currTime);

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
				cerr << "Invalid CEPAC API infection state: " << *(SimContext::HVL_STRATA_STRS[hvlStrata]);
				Util::exitWithPrompt(-1);
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

int Person::getGenerationOfInfection()
{
	//TODO: Make this a constant!
	if(generationOfInfection > 5)
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
	dmgProfile.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::SA);
	//CEPAC person needs to be initialized
	initialCEPACpatient(_eventParams);
	//Pass the info the personNode
	graphNode->timeSA = _eventParams.currTime;

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && trace())
	{
		if(getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Male ";
		}
		else
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Female ";
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << getID() << " becomes sexually active" << endl;
	}
}

Person *Person::fling(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams,
                      InfectionsTracker *infTrack)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::ENDType);
	int numActs = rollNumEventsPerPartner(_p, _eventParams.randomNums, _partnershipType);
	return sexualActivity(_p, numActs, _partnershipType, _eventParams, infTrack);
}

int Person::getAge(TimeGranularity _granularity) const
{
	assert(_granularity < ENDTimeGranularity);

	if(_granularity == MONTH)
	{
		return age;
	}
	else
	{
		return Util::convertTime(MONTH, _granularity, age);
	}
}


DmgProfile::ProfileID Person::getCurrBucketProfileID()
{
	return currentBucketID;
}

const DmgProfile *Person::getDmgProfile() const
{
	return &dmgProfile;
}

BaseEnumCls::Enum Person::getDmgProfileVal(DmgProfile::Demographic _demographic) const
{
	return dmgProfile.get(_demographic);
}

unsigned long Person::getID()
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

bool Person::inCorrectDmgProfileBucket()
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
void Person::print(ostream &_outStream, string _prefix) const
{
	_outStream << _prefix << endl;
	_outStream << ((getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE) ? "Male" : "Female") << Constants::TAB;
	_outStream << "ID: " << id << Constants::TAB;
	_outStream << "(";
	getDmgProfile()->print(_outStream, "");
	_outStream << ")";
	_outStream <<  Constants::TAB << "Age(mos.): " << getAge(MONTH);
	_outStream <<  Constants::TAB << "CD4: " << cd4;
	_outStream << Constants::TAB << "HVL: " << hvl;
	_outStream << Constants::TAB << "Risk: " << ((risk == Person::HIGH) ? "HIGH" : "LOW");
	_outStream << Constants::TAB << "Marbles: " << activityLevel;
	_outStream << endl;
}

void Person::printCurrentPartners(ostream &_outStream, std::string)
{
	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		list<SexualPartnership *>::iterator iter = partners[(int)type].begin();
		list<SexualPartnership *>::iterator iterEnd = partners[(int)type].end();

		if(iter != iterEnd)
		{
			_outStream << (SexualPartnership::TypeStrings.at(SexualPartnership::Type(type))) << Constants::COLON << endl;
		}

		while(iter != iterEnd)
		{
			Person *partner = (*iter)->getOtherPartner(this);
			partner->print(_outStream, "\t\t");
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
	_outStream << "id:" << id << "," << endl; //id
	dmgProfile.saveState(_outStream); //dmg profile
	_outStream << "curBktID:" << currentBucketID << "," << endl; //bucket id (contains same information as dmgprofile)
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

	_outStream << "]," << endl;
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

	_outStream << "]," << endl;
	_outStream << "genInf:" << generationOfInfection << "," << endl; //generation of infection
	_outStream << "risk:" << risk << "," << endl; //risk Level
	_outStream << "activity:" << activityLevel << "," << endl; //activity Level
	_outStream << "age:" << age << "," << endl; //age
	_outStream << "initAge:" << initAge << "," << endl; //initial age
	_outStream << "dead:" << death << "," << endl; //death
	_outStream << "hvl:" << hvl; //hvl in transmission includes primary and late stage
	//patient data
	//	if (wentThroughCEPAC){
	//		_outStream << "," << endl;
	//		cepacPatient->saveState(_outStream);
	//}
	/*
	//save the full vector indices of person
	_outStream << "fvInd:[";
	bool firstFV=true;
	for (map<FullVector*, vector<unsigned int> >::iterator it=FVindices.begin(); it != FVindices.end(); it++){ //loop over all Full Vectors
	if (!firstFV)
	_outStream << ",";
	firstFV=false;
	//full vector id and indices of person in fv
	_outStream << "{" << "fvID:" << (*it).first->getID() << ",ind:[";
	bool firstIndex=true;
	vector <unsigned int> * indicesPtr=&(*it).second;
	for (vector <unsigned int>::iterator indIter=indicesPtr->begin(); indIter != indicesPtr->end(); indIter++){
	if (!firstIndex)
	_outStream << ",";
	firstIndex=false;
	_outStream << *indIter;
	}
	_outStream << "]}";
	}
	_outStream << "]";
	*/
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
	        (getDmgProfileVal(DmgProfile::RELATIONSHIP_STATUS) == DmgProfile::NON_SINGLE))
	{
		dmgProfile.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::SINGLE);
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
		if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && trace())
		{
			if(getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
			{
				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Male ";
			}
			else
			{
				_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Female ";
			}

			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << getID() << " becomes CSW" << endl;
		}

		dmgProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::CSW);
	}
}

void Person::quitSexWork(EventParams &_eventParams)
{
	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && trace())
	{
		if(getDmgProfileVal(DmgProfile::GENDER) == DmgProfile::MALE)
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Male ";
		}
		else
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " % Female ";
		}

		_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << getID() << " quits being CSW" << endl;
	}

	dmgProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);
}

void Person::rerollRiskGroup(EventParams &/*_eventParams*/)
{
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::rerollRiskGroup()" << endl;
}

//determine whether this person died
bool Person::rollForDeath(RandomNums &_randomNums)
{
	assert(death == false);

	//if person is too old, then they automatically die
	if(getAge(MONTH) >= (12 * Person::maxYrForDeathStats))
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
		assert(Person::probDeathNatCauses[getDmgProfileVal(DmgProfile::GENDER)].size() > 0);

		//if this person is past Person::maxYrForDeathStats, they should not be alive
		//get the correct probability of death for this person's gender and age
		if(getAge(YEAR) >= static_cast<int>(Person::probDeathNatCauses[getDmgProfileVal(DmgProfile::GENDER)].size()))
		{
			cout << "The age is " << getAge(YEAR) << endl;
		}

		double deathRate = Person::probDeathNatCauses[getDmgProfileVal(DmgProfile::GENDER)].at(getAge(YEAR));
		death = _randomNums.chance(deathRate);

		if(death)
		{
			deathStatus = DTH_NONAIDS;
		}
	}

	//if they died, collect statistics
	if(death)
	{
		stats.setStat(STAT_TOTAL_LM, getAge(MONTH));
		stats.setStat(STAT_HIV_NEG_LM, getAge(MONTH) - (isInfected() ? stats.getStat(STAT_TIME_OF_INFECTION_MTH) : 0));
		stats.setStat(STAT_HIV_POS_POSTINFECT_LM, stats.getStat(STAT_TOTAL_LM) - stats.getStat(STAT_AGE_AT_INFECTION_MTH));
	}

	return death;
}

void Person::setCurrBucketProfileID(DmgProfile::ProfileID _profileID)
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

	if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (trace() || _p->trace()))
	{
		if(trace())
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "# Male " << getID() << " engages in " << _numActs <<
			        " acts with his " << (SexualPartnership::TypeStrings.at(_partnershipType)) << " " << _p->getID() << endl;
		}
		else
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << "# Female " << _p->getID() << " engages in " << _numActs <<
			        " acts with her " << (SexualPartnership::TypeStrings.at(_partnershipType)) << " " << getID() << endl;
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
		if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (trace() || _p->trace()))
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " !!# " << infected->getID() << " infected " <<
			        uninfected->getID() << "!" << endl;
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
		uninfected->becomeInfected(infected->getGenerationOfInfection() + 1, _eventParams);
		return uninfected;
	}
	else
	{
		if(_eventParams.outputTrace[EventParams::TraceFileType::Singleperson] && (trace() || _p->trace()))
		{
			_eventParams.traceStreams[EventParams::TraceFileType::Singleperson] << " !# " << infected->getID() << " exposed but did not infect " <<
			        uninfected->getID() << "!" << endl;
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

	//run this person's patient info one month forward in CEPAC
	cepacPatient->simulateMonth();

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
			static_cast<DmgProfile::Gender>(getDmgProfileVal(DmgProfile::GENDER)), getCd4Stratum(), 
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
			cerr << "Invalid CEPAC API infection state: " << *(SimContext::HVL_STRATA_STRS[hvlStrata]);
			Util::exitWithPrompt(-1);
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

	if(_eventParams.outputTrace[EventParams::TraceFileType::ArtRollout])
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
void Person::runCEPACtoDeath(RandomNums &_randomNums)
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
			//	cout << "doing the adding..." << endl;
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

Person::Person()
{
	/*healthAfterInfection = nullptr;*/
	risk = LOW;
	traceMe = false;
	generationOfInfection = -1;
	ageInfected = -1;
	wentThroughCEPAC = false;
	cepacPatient = nullptr;

	for(int type = 0; type < (int)SexualPartnership::Type::ENDType; ++type)
	{
		unformedPartnershipsLatestTime[type] = 0;
		unformedPartnershipsTotal[type] = 0;
		numPartnersInHistory[type] = 0;
		monthOfLatestPartnershipDissolution[type] = 0;
	}

	monthOfLatestConcurrent = 0;
	CEPACcosts = 0;
	graphNode = new GraphVizGraphElements::personNode(0, true, 0);
}

//this constructor is used by the Male and Female classes
Person::Person(EventParams &_eventParams, int _age, unsigned int _populationID)
{
	assert(_populationID >= 0);
	id = Person::idCounter++;
	populationID = _populationID;
#ifndef TESTING

	if(!Util::withinRange<int>(_age, 0, Util::convertTime(YEAR, MONTH, Person::maxYrForDeathStats)))
	{
		if(_age < 0)
		{
			_age = 0;
		}
		else
		{
			//cout << "SOMEONE WAS TOO OLD (" << _age << ")!  MAKING THEM " << Util::convertTime(YEAR, MONTH, Person::maxYrForDeathStats) << "!" << endl;
			_age = Util::convertTime(YEAR, MONTH, Person::maxYrForDeathStats);
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
	dmgProfile.set(DmgProfile::SEXUAL_ACTIVITY_STATUS, DmgProfile::NA);
	dmgProfile.set(DmgProfile::SEXUAL_ORIENTATION, DmgProfile::HETERO);
	dmgProfile.set(DmgProfile::RELATIONSHIP_STATUS, DmgProfile::SINGLE);
	//everyone is set as NON_CSW, but you can call becomeCSW() elsewhere if you want this person to be CSW
	dmgProfile.set(DmgProfile::EMPLOYMENT, DmgProfile::NON_CSW);
	hivStatus = NEGATIVE;
	cd4 = -1;
	hvl = UNINFECTED;
	//dmgProfile -- default constructor sets everything to wildcards
	//this person isn't a member of any bucket yet. this value will be changed when EntityPool adds or removes the person
	currentBucketID = DmgProfile::END;
	/** Create the new graph element keeping track of all relationship history via edges */
	graphNode = new GraphVizGraphElements::personNode(id,
	        (dmgProfile.get(DmgProfile::GENDER) == DmgProfile::MALE), _eventParams.currTime);
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

	//Remove person from all FVs
	/*map<FullVector*, vector<unsigned int> >::iterator FViter = FVindices.begin();
	while (FViter != FVindices.end()){
		FViter->first->remove(this);
		FViter++;
	}*/
	delete graphNode;
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

	map<FullVector *, vector<unsigned int>>::iterator FViter = FVindices.begin();

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

	for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
	{
		if(_eventParams.currTime > _eventParams.timesToSwitchSimContext[i])
		{
			returnValue = i;
		}
	}

	return returnValue;
}

//----------------< End Methods for Person >-------------------//





//----------------< Start Methods for to be implemented by Male and Female >-------------------//

/*
Person *Person::choosePartner(RandomNums &, EntityPool *, SexualPartnership::Type, bool)
{
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Called Person::choosePartner()" << endl;
	return nullptr;
}
*/

double Person::getFOI(Person * /*_p*/, SexualPartnership::Type /*_partnershipType*/, EventParams &/*_eventParams*/)
{
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Called Person::getFOI()" << endl;
	return 0.0;
}

double Person::getMinPartnerSelectVal(Person::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::getMinPartnerSelectVal()" << endl;
	return 0.0;
}



double Person::getMaxPartnerSelectVal(Person::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::getMaxPartnerSelectVal()" << endl;
	return 0.0;
}

double Person::rollForAgeDifference(SexualPartnership::Type /*_partnershipType*/, RandomNums &/*_randomNums*/)
{
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	cerr << "Person::rollForAgeDifference()" << endl;
	return 0.0;
}

double Person::getTransmissionCoeff()
{
	cerr << "Called Person::getTransmissionCoeff()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return 0.0;
}

bool Person::possibleMatch(SexualPartnership::Type /*_partnershipType*/, Person * /*_p*/)
{
	cerr << "Called Person::possibleMatch()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;
}

int Person::rollForNewPartnershipDuration(SexualPartnership::Type /*_partnershipType*/, RandomNums &/*_randomNums*/,
        Person * /*_p*/)
{
	cerr << "Called Person::rollForNewPartnershipDuration()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;
}

int Person::rollForNumPartners(RandomNums &/*_randomNums*/, SexualPartnership::Type /*_partnershipType*/)
{
	cerr << "Called Person::rollForNumPartners()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;
}

int Person::rollNumEventsPerPartner(Person * /*_p*/, RandomNums &/*_randomNums*/,
                                    SexualPartnership::Type /*_partnershipType*/)
{
	cerr << "Called Person::rollNumEventsPerPartner()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
	return false;
}


//----------------< Start Methods for to be implemented by Male and Female >-------------------//


