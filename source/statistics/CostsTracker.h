#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <boost/tuple/tuple.hpp>

#include "TabularOutput.h"
#include "BucketCounter.h"
#include "../cepac/include.h"
#include "../entities/Person.h"
#include "../data/EventParams.h"

class Person;
class Population;

enum class ClinicalCostTypes : size_t
{
	CD4Testing = 0,
	HvlTesting,
	ClinicVisits,
	HivScreeningTests,
	HivScreeningMisc,
	Last
};

template <class T>
size_t number() { return T::Last; }

class CostsTracker : protected TabularOutput
{
public:
	CostsTracker();
	~CostsTracker();

	void RecordCircumcision(double cost);

	void RecordCondomUse(double cost);

	void RecordMedicalCosts(const std::array<double, 4> &medicalCosts, const Person &person);

	void RecordClinicalCosts(const std::array<double, (size_t)ClinicalCostTypes::Last> &medicalCosts, const Person &person);

	void RecordTreatmentCosts(const std::array<double, 3> &medicalCosts, int artLine, const Person &person);

	void RecordCepacCosts(double costsUndiscounted, double costsDiscounted, const Person &person);

	void RecordLifeMonth(const Person &person);

	void PrintCosts(int time, std::ostream &_outStream);

	static const int NumArtLinesToRecord = 4;

private:	
	struct Costs
	{
		std::array<double, Person::ENDHIVStatus> lifeMonthsByHivStatus;
		std::array<double, Person::ENDHIVStatus> qalmsByHivStatus;
		double condomCosts;
		double circumcisionCosts;
		std::array<double, SimContext::COST_NUM_TYPES> medicalCosts;
		std::array<double, DmgProfile::ENDGender> medicalCostsByGender;
		std::array<double, (size_t)ClinicalCostTypes::Last> clinicalCosts;
		std::array<double, SimContext::HIV_ID_NUM> medicalCostsByHivState;
		std::array<double, Person::ENDCD4Strata> medicalCostsByCd4;
		std::array<double, SimContext::HVL_NUM_STRATA> medicalCostsByHvl;
		std::array<double, SimContext::HVL_NUM_STRATA> medicalCostsByHvlSetpoint;
		std::array<double, NumArtLinesToRecord> artCosts;
		double drugCosts;
		double toxicityCosts;
		std::array<double, Person::ENDCD4Strata> medicalCostsByCd4NoOiHist;
		std::array<double, Person::ENDCD4Strata> medicalCostsByCd4WithOiHist;
	};

	Costs undiscounted_;

	Costs discounted_;

	void BuildHeader();

	void BuildRow(int time);

	void Reset();
};
