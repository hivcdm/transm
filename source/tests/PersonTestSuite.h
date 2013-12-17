#include <cxxtest/TestSuite.h>

#include "../entities/Person.h"
#include "../statistics/InfectionsTracker.h"

class PersonTestSuite : public CxxTest::TestSuite
{
public:
	PersonTestSuite()
	{

	}

	~PersonTestSuite()
	{

	}

    void testConstructorsDestructors()
    {
		Person p1;
		EventParams params;
		Person p2(params, 123, 1234);

		//p.deletePersonWithoutDeleting();
		//TS_ASSERT_EQUALS(p2.getAge(MONTH), 123);
		//TS_ASSERT_EQUALS(p2.getPopulationID(), 1234);
    }

	void testReseters()
	{
		Person p;
		p.resetCondomUsage();
		p.resetNumActs();
		p.resetLatestUnformedPartnerships(SexualPartnership::CASUAL);
	}

	void testDeath()
	{
		Person p;
		p.death = false;
		RandomNums randomNums;
		p.rollForDeath(randomNums);
	}

	void testRolls()
	{
		Person p;
		EventParams params;
		RandomNums randomNums;
		Person inPerson;
		std::list<Person*> newlyInfected;
		InfectionsTracker infTrack;

		p.rollForBecomeSexWorker(params, false, 0.0);
		p.rerollRiskGroup(params);
		p.rollForAgeDifference((SexualPartnership::Type)0, randomNums);
		p.becomeInfected(0, params);
		p.initialCEPACpatient(params);
		p.updateHealthStatus(params);
		p.runCEPACtoDeath(randomNums);
		p.becomeSexuallyActive(params);
		p.quitSexWork(params);
		p.rollForNewPartnershipDuration((SexualPartnership::Type)0, randomNums, &inPerson);
		p.rollForNumPartners(randomNums, (SexualPartnership::Type)0);
		p.rollNumEventsPerPartner(&inPerson, randomNums, (SexualPartnership::Type)0);
		Person* outPerson = p.allPartnerSexualActivity(params, (SexualPartnership::Type)0, newlyInfected, &infTrack);
		outPerson = p.fling(&inPerson, (SexualPartnership::Type)0, params, &infTrack);
		outPerson = p.sexualActivity(&inPerson, 0, (SexualPartnership::Type)0, params, &infTrack);

		p.getFOI(&inPerson, SexualPartnership::CASUAL, params);
	}

	void testFullVector()
	{
		Person p;
		FullVector fv;

		std::vector<unsigned int> indices = p.getFVindices(&fv);
		p.setFVindices(indices, &fv);
		p.addFVindices(0, &fv);
		p.removeFVindices(0, &fv);
		p.memberFVindices(0, &fv);
	}

	void testMutators()
	{
		Person p;
		SexualPartnership partnership;

		p.addPartnership(&partnership);
		p.removePartnership(&partnership);

		EntityPool *pool = NULL;
		Person *partner = NULL;
		partner = p.choosePartner((SexualPartnership::Type)0, pool, true);
		partner = p.choosePartner((SexualPartnership::Type)0, pool, false);

		Person testMatchPerson;
		p.possibleMatch((SexualPartnership::Type)0, &testMatchPerson);
	}

    void testBasicGettersAndSetters()
    {
		Person p;
		EventParams params;
		//Person p2(params, 123, 1234, 0);

		p.getGenerationOfInfection();
		p.getNumPartnersInHistory();
		p.getHVL();
		p.getTransmissionCoeff();

		p.incrementCondomsUsedThisMonth(5);
		p.getCondomsUsedThisMonth();

		p.incrementNumActsThisMonth(5);
		p.getNumActsThisMonth();

		p.getPersonNode();
		p.getCondomUsedLastFOICalculation();
		p.getRiskLevel();
		p.getHIVStatus();
		p.getSexualActivity();
		p.getDmgProfile();
		p.getCurrBucketProfileID();
		p.getID();
		p.getPopulationID();
		p.getStats();
		p.inCorrectDmgProfileBucket();
		p.isAlive();
		p.isInfected();

		p.getMonthOfLatestConcurrent();
		p.setMonthOfLatestConcurrent(1);
		p.getMonthOfLatestConcurrent();

		p.trace();
		p.setToBeTraced();
		p.trace();

		//p.setCurrBucketProfileID(DmgProfile::ProfileID _profileID);
		//p.setSimContext(SimContext * newSimContext);
	}

    void testComplexGettersAndSetters()
    {
		Person p;

		for(int i = 0; i < SexualPartnership::ENDType; i++)
		{
			p.increaseUnformedPartnershipTallies((SexualPartnership::Type)i);
			p.availableForPartnership((SexualPartnership::Type)i);
			p.getNumPartners((SexualPartnership::Type)i);
			p.getNumPartners((SexualPartnership::Type)i, false); // different risk
			p.getNumPartners((SexualPartnership::Type)i, true); // same risk
			p.getNumPartnersInHistory((SexualPartnership::Type)i);
			p.getMonthOfLatestPartnershipDissolution((SexualPartnership::Type)i);
			p.getTotalUnformedPartnerships((SexualPartnership::Type)i);
			p.getLatestUnformedPartnerships((SexualPartnership::Type)i);
			p.hasPartnership((SexualPartnership::Type)i);

			std::list<SexualPartnership*> toEnd;
			p.getPartnershipsToEnd(0, (SexualPartnership::Type)i, toEnd, false);
			p.getPartnershipsToEnd(0, (SexualPartnership::Type)i, toEnd, true);

			for(int j = 0; j < Person::ENDSelectingCriteria; j++)
			{
				p.getMinPartnerSelectVal(Person::SelectingCriteria(j), (SexualPartnership::Type)i);
				p.getMaxPartnerSelectVal(Person::SelectingCriteria(j), (SexualPartnership::Type)i);
			}
		}

		for(int demographic = 0; demographic < DmgProfile::ENDDemographic; demographic++)
		{
			p.getDmgProfileVal((DmgProfile::Demographic)demographic);
		}
		
		for(int granularity = 0; granularity < ENDTimeGranularity; granularity++)
		{
			p.getAge(TimeGranularity(granularity));
		}

		p.ageOneTimeUnit();

		for(int granularity = 0; granularity < ENDTimeGranularity; granularity++)
		{
			p.getAge(TimeGranularity(granularity));
		}

		p.isPartneredWith(NULL);
    }
};
