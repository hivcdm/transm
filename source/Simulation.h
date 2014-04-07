#pragma once

#include "Outputs.h"
#include "Population.h"
#include "data/EventParams.h"
#include "statistics/PopStats.h"
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
	Nullable<int> age_lower;
	Nullable<int> age_upper;
	Nullable<DmgProfile::Employment> employment;
	Nullable<DmgProfile::SexualActivityStatus> sexual_activity_status;
	Nullable<DmgProfile::Gender> gender;
	Nullable<DmgProfile::RelationshipStatus> relationship_status;
	Nullable<DmgProfile::SexualOrientation> sexual_orientation;
	Nullable<Person::RiskLevel> risk_level;
	Nullable<Person::HIVStatus> observed_hiv_status;
	Nullable<bool> on_treatment;
};

enum class TemplateParameter
{
	BirthRate,
	ProportionMale,
	ProportionCircumcised,
	AgeSexualDebutYears,
	OIHistRank,
	OIHistOI0,
	OIHistOI1,
	OIHistOI2,
	OIHistOI3,
	OIHistOI4,
	OIHistOI5,
	OIHistOI6,
	OIHistOI7,
	OIHistOI8,
	OIHistOI9,
	OIHistOI10,
	OIHistOI11,
	OIHistOI12,
	OIHistOI13,
	OIHistOI14,
	OIHistNumOIToStart,
	CD4Rank,
	CD4CD4Upp,
	CD4CD4Lwr,
	CD4OIHistRank,
	CD4OIHistOI0,
	CD4OIHistOI1,
	CD4OIHistOI2,
	CD4OIHistOI3,
	CD4OIHistOI4,
	CD4OIHistOI5,
	CD4OIHistOI6,
	CD4OIHistOI7,
	CD4OIHistOI8,
	CD4OIHistOI9,
	CD4OIHistOI10,
	CD4OIHistOI11,
	CD4OIHistOI12,
	CD4OIHistOI13,
	CD4OIHistOI14,
	CD4OIHistCD4Upp,
	CD4OIHistCD4Lwr,
	HVLRank,
	HVLHVLUpp,
	HVLHVLLwr,
	CD4HVLRank,
	CD4HVLCD4Upp,
	CD4HVLCD4Lwr,
	CD4HVLHVLUpp,
	CD4HVLHVLLwr,
	SteadyChanceCondomUsePerEventHighRisk,
	SteadyChanceCondomUsePerEventLowRisk,
	RegularChanceCondomUsePerEventHighRisk,
	RegularChanceCondomUsePerEventLowRisk,
	CasualChanceCondomUsePerEventHighRisk,
	CasualChanceCondomUsePerEventLowRisk,
	CswChanceCondomUsePerEventHighRisk,
	CswChanceCondomUsePerEventLowRisk
};

class Simulation
{
public:
	typedef std::function<void(const std::string &)> MessageCallback;

	Simulation(const std::string &run_name);

	~Simulation();

	Outputs Run(MessageCallback message_callback);

	//returns eventParams.cepacRunStats for adding to the general popstats
	RunStats &GetCEPACRunStats();

	//returns population->popStats information for creating popStats-like file for transmission output
	PopStats &GetPopStats();

	EventParams &GetEventParams();

	double GetPrevalence() { return prevalence_; }

	double GetIncidence() { return incidence_; }

	int GetTotalTime() { return totalTime_; }

	int GetTime() { return time_; }

private:
	friend class Serializer;

	struct TreatmentFile
	{
		std::string file_name;
		int file_number;
		int time;
		int target_population;
	};

	struct TimeDependentParameter
	{
		std::string key;
		int time;
		Nullable<PopulationTarget> target_population;
		std::string value;
		std::function<void(Person *)> population_modifier;
		std::function<void(const std::string &)> simulation_modifier;
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

	const std::string name_;

	/** current time in the simulation */
	int time_;

	/** number of months to run this file in a sequence*/
	int duration_;

	/** pointer to current population */
	Population population_;

	/** housekeeping parameters that are universal to each event in the simulation */
	EventParams parameters_;

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

	std::map<int, double> target_rollout_proportions_;

	std::unordered_map<std::string, TemplateParameter> template_key_map_;

	std::vector<TimeDependentParameter> unmatched_parameters_;

	std::unordered_map<TemplateParameter, TimeDependentParameter> time_dependent_parameters_;
};

