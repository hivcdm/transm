#ifndef COSTTRACKER_HPP
#define COSTTRACKER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <include.h>

#include "bucketcounter.hpp"
#include "tabularoutput.hpp"
#include "entities/entitytypes.hpp"
#include "parameters/eventparams.hpp"

namespace transm {

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

    void RecordPrEPCost(double costUndiscounted, double costDiscounted);

    void RecordVaginalMicrobicideCost(double costUndiscounted, double costDiscounted);

	void RecordMedicalCosts(const std::array<double, 4> &costsUndiscounted, const std::array<double, 4> &costsDiscounted);

	void RecordClinicalCosts(const std::array<double, 5> &costsUndiscounted, const std::array<double, 5> &costsDiscounted);

	void RecordTreatmentCosts(const std::array<double, 3> &costsUndiscounted, const std::array<double, 3> &costsDiscounted, int artLine);

	void RecordCepacCosts(double costsUndiscounted, double costsDiscounted, const std::string &entityType, CD4Strata cd4,
		HVLStrata hvl, HIVStatus status);

	void RecordLifeMonth(double qualityOfLife, double discountFactor, HIVStatus status);

	void PrintCosts(Time time, std::ostream &_outStream);

	static const int NumArtLinesToRecord = 4;

private:
	struct Costs
	{
		std::array<double, (std::size_t)HIVStatus::Last> lifeMonthsByHivStatus;
		std::array<double, (std::size_t)HIVStatus::Last> qalmsByHivStatus;
		double condomCosts;
		double circumcisionCosts;
        double prEPCosts;
        double vaginalMicrobicideCosts;
		std::array<double, SimContext::COST_NUM_TYPES> medicalCosts;
		std::array<double, (size_t)ClinicalCostTypes::Last> clinicalCosts;
		std::array<double, NumArtLinesToRecord> artCosts;
		double drugCosts;
		double toxicityCosts;
		std::unordered_map<std::string, double> totalCostsByEntityType;
		std::array<double, (std::size_t)HIVStatus::Last> totalCostsByHivState;
        std::array<double, (std::size_t)CD4Strata::Last> totalCostsByCd4;
        std::array<double, (std::size_t)HVLStrata::Last> totalCostsByHvl;
	};

	Costs undiscounted_;

	Costs discounted_;

	void BuildHeader();

	void BuildRow(Time time);

	void Reset();
};

} // namespace transm


#endif /* COSTTRACKER_HPP */