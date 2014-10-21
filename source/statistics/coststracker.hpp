#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <include.h>

#include "bucketcounter.hpp"
#include "tabularoutput.hpp"
#include "entities/entity.hpp"
#include "data/eventparams.hpp"

namespace transm {

class Entity;
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

	void RecordCepacCosts(double costsUndiscounted, double costsDiscounted, DemographicProfile::Gender gender, Entity::CD4Strata cd4,
		Entity::HVLStrata hvl, Entity::HIVStatus status);

	void RecordLifeMonth(double qualityOfLife, double discountFactor, Entity::HIVStatus status);

	void PrintCosts(int time, std::ostream &_outStream);

	static const int NumArtLinesToRecord = 4;

private:	
	struct Costs
	{
		std::array<double, Entity::ENDHIVStatus> lifeMonthsByHivStatus;
		std::array<double, Entity::ENDHIVStatus> qalmsByHivStatus;
		double condomCosts;
		double circumcisionCosts;
		std::array<double, SimContext::COST_NUM_TYPES> medicalCosts;
		std::array<double, (size_t)ClinicalCostTypes::Last> clinicalCosts;
		std::array<double, NumArtLinesToRecord> artCosts;
		double drugCosts;
		double toxicityCosts;
		std::array<double, (std::size_t)DemographicProfile::Gender::Last> totalCostsByGender;
		std::array<double, Entity::ENDHIVStatus> totalCostsByHivState;
		std::array<double, Entity::ENDCD4Strata> totalCostsByCd4;
		std::array<double, Entity::ENDHVLStrata + 1> totalCostsByHvl;
	};

	Costs undiscounted_;

	Costs discounted_;

	void BuildHeader();

	void BuildRow(int time);

	void Reset();
};

} // namespace transm
