#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <cepac/include.h>

#include "BucketCounter.h"
#include "TabularOutput.h"
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

	void RecordCircumcision(double costUndiscounted, double costDiscounted);

	void RecordCondomUse(double costUndiscounted, double costDiscounted);

	void RecordMedicalCosts(const std::array<double, 4> &costsUndiscounted, const std::array<double, 4> &costsDiscounted);

	void RecordClinicalCosts(const std::array<double, 5> &costsUndiscounted, const std::array<double, 5> &costsDiscounted);

	void RecordTreatmentCosts(const std::array<double, 3> &costsUndiscounted, const std::array<double, 3> &costsDiscounted, int artLine);

	void RecordCepacCosts(double costsUndiscounted, double costsDiscounted, DmgProfile::Gender gender, Person::CD4Strata cd4,
		Person::HVLStrata hvl, Person::HIVStatus status);

	void RecordLifeMonth(double qualityOfLife, double discountFactor, Person::HIVStatus status);

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
		std::array<double, (size_t)ClinicalCostTypes::Last> clinicalCosts;
		std::array<double, NumArtLinesToRecord> artCosts;
		double drugCosts;
		double toxicityCosts;
		std::array<double, DmgProfile::ENDGender> totalCostsByGender;
		std::array<double, Person::ENDHIVStatus> totalCostsByHivState;
		std::array<double, Person::ENDCD4Strata> totalCostsByCd4;
		std::array<double, Person::ENDHVLStrata + 1> totalCostsByHvl;
	};

	Costs undiscounted_;

	Costs discounted_;

	void BuildHeader();

	void BuildRow(int time);

	void Reset();
};
