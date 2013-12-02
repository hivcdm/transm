#include <cxxtest/TestSuite.h>

#include "entities/Person.h"

class PersonTest : public CxxTest::TestSuite
{
/*
// constructors/destructors
	Person();
	Person(EventParams &_eventParams, int _age, unsigned int _populationID);
	~Person(void);
	void deletePersonWithoutDeleting();

// eventParams
	virtual double getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams& _eventParams);
	int getCEPACSimContextIndex(EventParams& _eventParams);

// getters
	int getGenerationOfInfection();
	int getNumPartners(SexualPartnership::Type);
	int getNumPartners(SexualPartnership::Type, bool);
	int getNumPartnersInHistory();
	int getNumPartnersInHistory(SexualPartnership::Type);
	int getMonthOfLatestPartnershipDissolution(SexualPartnership::Type);
	int getMonthOfLatestConcurrent();
	HVLStrata getHVL() const { return this->hvl; }
	virtual double getTransmissionCoeff();
	bool isAlive() const;
	bool isInfected();
	int getCondomsUsedThisMonth();
	int getNumActsThisMonth();
	GraphVizGraphElements::personNode* getPersonNode(){ return this->graphNode; }
	bool getCondomUsedLastFOICalculation();
	const Person::RiskLevel getRiskLevel() const;
	const Person::HIVStatus getHIVStatus() const;
	int getSexualActivity();
	const DmgProfile* getDmgProfile() const;
	BaseEnumCls::Enum getDmgProfileVal(DmgProfile::Demographic _demographic) const;
	DmgProfile::ProfileID getCurrBucketProfileID();
	virtual double getMinPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	virtual double getMaxPartnerSelectVal(Person::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const;
	long getPartnershipsToEnd(long _currTime,SexualPartnership::Type _partnershipType, list<SexualPartnership*> &_partnershipsToEnd, bool _fromDeath);
	int getAge(TimeGranularity _granularity) const;
	unsigned long getID();
	unsigned int getPopulationID();
	const Person::StatsRecord* getStats();
	int getTotalUnformedPartnerships(SexualPartnership::Type type);
	int getLatestUnformedPartnerships(SexualPartnership::Type type);
	bool inCorrectDmgProfileBucket();
	bool trace();
	bool isPartneredWith(Person *_p);
	bool hasPartnership(SexualPartnership::Type);

// setters
	void setMonthOfLatestConcurrent(int);
	void setCurrBucketProfileID(DmgProfile::ProfileID _profileID);
	void setSimContext(SimContext * newSimContext);
	void setToBeTraced();

// reseters
	void resetCondomUsage();
	void resetNumActs();
	void resetLatestUnformedPartnerships(SexualPartnership::Type type);

// incrementors
	void ageOneTimeUnit();
	void incrementCondomsUsedThisMonth(int condoms = 1);
	void incrementNumActsThisMonth(int _numActs);
	void increaseUnformedPartnershipTallies(SexualPartnership::Type type);

// rolls
	bool rollForDeath(RandomNums &_randomNums);
	void rollForBecomeSexWorker(EventParams& _eventParams, bool _isInit, double initialProb = 0.0);
	virtual void rerollRiskGroup(EventParams& _eventParams);
	virtual double rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNums& _randomNums);
	void becomeInfected(int _generationOfInfection, EventParams& _eventParams);
	void initialCEPACpatient(EventParams& _eventParams);
	double updateHealthStatus(EventParams& _eventParams);
	void runCEPACtoDeath(RandomNums& _randomNums);
	void becomeSexuallyActive(EventParams& _eventParams);
	void quitSexWork(EventParams &_eventParams);
	Person* fling(Person *_p, SexualPartnership::Type _partnershipType, EventParams& _eventParams, InfectionsTracker *infTrack);
	Person* allPartnerSexualActivity(EventParams& _eventParams, SexualPartnership::Type _partnershipType, list<Person*> &_newlyInfected, InfectionsTracker *infTrack);
	virtual int rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNums& _randomNums, Person *_p);
	virtual int rollForNumPartners(RandomNums& _randomNums, SexualPartnership::Type _partnershipType);
	virtual int rollNumEventsPerPartner(Person *_p, RandomNums& _randomNums, SexualPartnership::Type _partnershipType);
	Person* sexualActivity(Person *_p, int _numActs, SexualPartnership::Type _partnershipType, EventParams& _eventParams, InfectionsTracker *infTrack);

// full vector
	vector<unsigned int> getFVindices(FullVector* FV);
	bool setFVindices(vector<unsigned int> FVind, FullVector* FV);
	bool addFVindices(int index, FullVector* FV);
	bool removeFVindices(int index, FullVector* FV);
	bool memberFVindices(int index, FullVector* FV);

// mutators
	void addPartnership(SexualPartnership *_partnership);
	bool availableForPartnership(SexualPartnership::Type _partnershipType) const;
	virtual Person* choosePartner(SexualPartnership::Type _partnershipType, EntityPool *_availableEntities, bool _remove);
	virtual bool possibleMatch(SexualPartnership::Type _partnershipType, Person *_p);
	void removePartnership(SexualPartnership *_partnership);
	virtual void saveState(ostream& _outStream, long currTime);

// output
	void print(ostream& _outStream, string _prefix) const;
	void printCurrentPartners(ostream& _outStream, string _prefix);
*/
public:
    void testConstructorsDestructors()
    {
	Person p1;
	EventParams params;
	Person p2(params, 123, 1234);

	TS_ASSERT_EQUALS(p2.getAge(MONTH), 123);
	TS_ASSERT_EQUALS(p2.getPopulationID(), 1234);
    }

    void testGettersAndSetters()
    {
	Person p;
    }
};
