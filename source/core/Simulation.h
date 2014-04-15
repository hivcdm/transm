#pragma once

#include <deque>

#include "Outputs.h"
#include "Population.h"
#include "data/EventParams.h"
#include "statistics/PopulationStatistics.h"
#include "util/HighResolutionTimer.h"
#include "util/Nullable.h"
#include "util/xml/pugixml.hpp"

class InfectionsTracker;

struct TraceFile
{
	enum class Type
	{
		Population,
		Infection,
		Partnership,
		Survival,
		CostEffectiveness,
		Clinical,
		Events,
		Health,
		SinglePerson,
		LifeExpectancy,
		PartnerAcquisition,
		CalibrationStatistics,
		ArtRollout,
		ShiftedOutcomes,
		Last,
		First = Population
	} type;
	bool enabled;
	std::string extension;
	bool toss;
};

struct PopulationTarget
{
	static PopulationTarget FromString(const std::string &s);
	static PopulationTarget Any;

	Nullable<Person::RiskLevel> risk_level;
	Nullable<DemographicProfile::Employment> employment;
	Nullable<DemographicProfile::SexualActivityStatus> sexual_activity_status;
	Nullable<DemographicProfile::Gender> gender;
	Nullable<DemographicProfile::RelationshipStatus> relationship_status;
	Nullable<DemographicProfile::SexualOrientation> sexual_orientation;
	Nullable<int> age_lower;
	Nullable<int> age_upper;
	Nullable<Person::HIVStatus> observed_hiv_status;
	Nullable<bool> on_treatment;

	bool operator==(const PopulationTarget &other) const
	{
		return risk_level == other.risk_level &&
			employment == other.employment &&
			sexual_activity_status == other.sexual_activity_status &&
			gender == other.gender &&
			relationship_status == other.relationship_status &&
			sexual_orientation == other.sexual_orientation &&
			age_lower == other.age_lower &&
			age_upper == other.age_upper &&
			observed_hiv_status == other.observed_hiv_status &&
			on_treatment == other.on_treatment;
	}

	bool operator!=(const PopulationTarget &other) const { return !(*this == other); }
};

class Simulation
{
public:
	typedef std::function<void(const std::string &)> MessageCallback;
	typedef std::function<void(Simulation &)> SimulationIntervention;
	typedef std::function<void(EventParams::RolloutEligibility &)> EligibilityIntervention;
	typedef std::function<void(PopulationParameters &)> PopulationIntervention;

	Simulation();

	~Simulation();

	Outputs Run(MessageCallback message_callback);

	//returns eventParams.cepacRunStats for adding to the general popstats
	RunStats &GetCEPACRunStats();

	//returns population->popStats information for creating popStats-like file for transmission output
	PopulationStatistics &GetPopulationStatistics();

	EventParams &GetEventParams();

	double GetPrevalence() { return prevalence_; }

	double GetIncidence() { return incidence_; }

	int GetTotalTime() { return totalTime_; }

	int GetTime() { return time_; }

	void RegisterSimulationIntervention(int time, SimulationIntervention callback);

	void SetRolloutEligibilityRank(const std::string &criterion, int rank);
	void SetRolloutEligibilityOIHist(int oi, bool required) { parameters_.rolloutEligibility.oiHistOIs[oi] = required; }
	void SetRolloutEligibilityNumToStart(int num) { parameters_.rolloutEligibility.oiHistNumToStart = num; }
	void SetRolloutEligibilityCD4Lwr(int lower) { parameters_.rolloutEligibility.cd4Bounds[0] = lower; }
	void SetRolloutEligibilityCD4Upp(int upper) { parameters_.rolloutEligibility.cd4Bounds[1] = upper; }
	void SetRolloutEligibilityCD4OIHist(int oi, bool required) { parameters_.rolloutEligibility.cd4OiHistOIs[oi] = required; }
	void SetRolloutEligibilityCD4OIHistCD4Lwr(int lower) { parameters_.rolloutEligibility.cd4OiHistCd4Bounds[0] = lower; }
	void SetRolloutEligibilityCD4OIHistCD4Upp(int upper) { parameters_.rolloutEligibility.cd4OiHistCd4Bounds[1] = upper; }
	void SetRolloutEligibilityHVLLwr(int lower) { parameters_.rolloutEligibility.hvlBounds[0] = lower; }
	void SetRolloutEligibilityHVLUpp(int upper) { parameters_.rolloutEligibility.hvlBounds[1] = upper; }
	void SetRolloutEligibilityCD4HVLCD4Lwr(int lower) { parameters_.rolloutEligibility.cd4HvlHvlBounds[0] = lower; }
	void SetRolloutEligibilityCD4HVLCD4Upp(int upper) { parameters_.rolloutEligibility.cd4HvlHvlBounds[1] = upper; }
	void SetRolloutEligibilityCD4HVLHVLLwr(int lower) { parameters_.rolloutEligibility.cd4HvlCd4Bounds[0] = lower; }
	void SetRolloutEligibilityCD4HVLHVLUpp(int upper) { parameters_.rolloutEligibility.cd4HvlCd4Bounds[1] = upper; }

	Population &GetPopulation() { return population_; }
	const Population &GetPopulation() const { return population_; }

	void SetCondomCost(double condom_cost) { population_.SetCondomCost(condom_cost); }

	void SetCircumcisionCost(double circumcision_cost) { population_.SetCircumcisionCost(circumcision_cost); }

	void SetDuration(int duration) { duration_ = duration; }

	void SetFixedSeed(int seed);

	void SetBirthRate(double birth_rate) { population_.popWideParams.setBirthRate(birth_rate); };

	void SetAgeSexualDebut(int age, TimeGranularity granularity = YEAR) { population_.popWideParams.setAgeSexualDebut(age, granularity); }

	void SetProportionMale(double proportion_male) { population_.popWideParams.setProportionMale(proportion_male); }

	void SetProportionCircumcised(double proportion_circumcised, Nullable<PopulationTarget> target)
	{
		if(!target.has_value)
		{
			population_.popWideParams.setProportionCircumcised(proportion_circumcised);
		}
		else
		{
			population_.circumcise(parameters_.randomNums, proportion_circumcised, target.value);
		}
	}

	void SetAssortativeness(SexualPartnership::Type type, double assortativeness)
	{
		population_.popWideParams.setAssortativeness(type, assortativeness);
	}

	void SetTransmissionCoefficients(DemographicProfile::Gender gender, const std::array<double, 7> &coefficients)
	{
		for(int i = 0; i < 7; i++)
		{
			population_.popWideParams.SetTransmissionCoefficient(gender, (Person::HVLStrata)i, coefficients[i]);
		}
	}

	void SetTransmissionCoefficient(DemographicProfile::Gender gender, Person::HVLStrata stratum, double coefficient)
	{
		population_.popWideParams.SetTransmissionCoefficient(gender, stratum, coefficient);
	}

	void SetChanceBecomeCsw(DemographicProfile::Gender gender, double chance)
	{
		population_.popWideParams.SetChanceBecomeCsw(gender, chance);
	}

	void SetProportionHighRisk(DemographicProfile::Gender gender, DemographicProfile::Employment employment, double proportion)
	{
		population_.popWideParams.SetProportionHighRisk(gender, employment, proportion);
	}

	void SetAverageYearsYounger(SexualPartnership::Type type, NormalDist dist) { population_.popWideParams.setAverageYearsYounger(type, dist); }
	void SetAcquisitionRatePerMonth(Person::RiskLevel risk, SexualPartnership::Type type, LogNormalDist dist) { population_.popWideParams.setAcquisitionRatePerMonth(risk, type, dist); }
	void SetCoitalEventsPerMonth(Person::RiskLevel risk, SexualPartnership::Type type, double mean) { population_.popWideParams.setCoitalEventsPerMonth(risk, type, mean); }
	void SetChanceCondomUsePerEvent(Person::RiskLevel risk, SexualPartnership::Type type, BetaDist dist) { population_.popWideParams.setChanceCondomUsePerEvent(risk, type, dist); }
	void SetPartnershipDuration(Person::RiskLevel risk, SexualPartnership::Type type, ShiftedLogNormalDist dist) { population_.popWideParams.setPartnershipDuration(risk, type, dist); }

	void SetName(const std::string &name) { name_ = name; parameters_.simName = name; }

	void SetChanceBecomeSexWorker(DemographicProfile::Gender gender, double chance) { population_.popWideParams.SetChanceBecomeCsw(gender, chance); }
	void SetPartnerAcquisitionSteadyMultiplier(Person::RiskLevel risk, double multiplier) { population_.popWideParams.SetPartnerAcquisitionSteadyMultiplier(risk, multiplier); }

private:
	friend class SimulationBuilder;

	struct TreatmentFile
	{
		std::string file_name;
		int file_number;
		int time;
		int target_population;
	};

	typedef std::array<TreatmentFile, Constants::NUMBER_OF_CEPAC_FILES> CepacTreatmentFiles;
	typedef std::array<TreatmentFile, Constants::NUMBER_OF_ROLLOUT_FILES> RolloutTreatmentFiles;

	void FirstStep();

	void LastStep();

	void Step();

	/** Returns true if all simContexts loaded correctly */
	bool LoadCepacSimContexts(const CepacTreatmentFiles &treatment_files);

	/** */
	bool LoadRolloutSimContexts(const RolloutTreatmentFiles &treatment_files);

	/** Sets the Non aids death from a cepac simcontext */
	void SetNonAidsDeathFromCepac(SimContext &context, std::vector<double> &male, std::vector<double> &female);

	/** perform one timestep of simulation */
	int SimulateMonth();

	void UpdateTimeDependentParameters();

	void ValidateState();

	std::string name_;

	/** current time in the simulation */
	int time_;

	/** number of months to run this file in a sequence*/
	int duration_;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams parameters_;

	/** current population */
	Population population_;

	bool failedCalibration_;

	bool hasPassedFirstMonthCalibPrev_;

	int monthOfFirstMonthCalibPrev_;

	int totalTime_;

	int fixedSeed_;

	double incidence_;

	double prevalence_;

	HighResolutionTimer timer_;

	Outputs outputs_;

	CepacTreatmentFiles cepac_treatment_files_;

	RolloutTreatmentFiles rollout_treatment_files_;

	std::deque<std::pair<int, std::vector<SimulationIntervention>>> simulation_interventions;
};

