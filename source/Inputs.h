#pragma once

#include <array>
#include <string>
#include <unordered_map>

#include "entities/Person.h"
#include "util/Util.h"
#include "util/ticpp/ticpp.h"

enum class PartnershipType
{
	Steady,
	Regular,
	Casual,
	CSW,
	SteadyMsm,
	RegularMsm,
	CasualMsm,
	CswMsm,
	SteadyBisexual,
	RegularBisexual,
	CasualBisexual,
	CswBisexual
};

struct Distribution
{
	enum class DistributionType
	{
		Poission,
		Normal,
		ShiftedLogNormal
	} Type;
	double mu;
	double sigma;
	double c;
};

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

struct ConcurrencyDefinition
{
	int minimum_needed;
	bool allow;
};

struct CalibrationTarget
{
	double lower;
	double upper;
	enum class PopulationOfInterest
	{
		AllSexuallyActive = 0,
		MaleSexuallyActive = 1
	} target;
};

struct CalibrationSettings
{
	bool enabled;
	int month;
	CalibrationTarget steady_target;
	CalibrationTarget casual_target;
	CalibrationTarget csw_target;
	CalibrationTarget proportion_concurrent_target;
	CalibrationTarget num_acts_target;
	double female_casual_prevalence;
	double female_proportion_in_concurrent;
	double female_proportion_lr_to_hr;
	std::array<double, 13> target_prevalence;
};

struct PopulationSettings
{
	std::string id;
	int initial_size;
	int prevalence_delay;
	struct AgeDistributionStratum
	{
		int lower;
		int upper;
		double male_distribution;
		double female_distribution;
		int num_infected_male_csw;
		int num_infected_male_high_risk;
		int num_infected_male_low_risk;
		int num_infected_female_csw;
		int num_infected_female_high_risk;
		int num_infected_female_low_risk;
	};
	std::vector<AgeDistributionStratum> age_distributions;
	double birth_rate;
	double proportion_male;
	double proportion_circumcised;
	double age_sexual_debut;
	struct MaleSettings
	{
		double chance_become_sex_worker;
		double partner_acquisition_multiplier_with_steady_high;
		double partner_acquisition_multiplier_with_steady_low;
		bool enable_high_risk_multiplier;
		double high_risk_acquisition_rate_multiplier;
		bool enable_csw_high_risk_acquisition_rate_multiplier;
		double csw_high_risk_acquisition_rate_multiplier;
		int heterogeneity_var_method;
		int coefficient_of_variation;
		struct PartnershipSettings
		{
			PartnershipType type;
			Distribution acquisition_rate_high_risk;
			Distribution acquisition_rate_low_risk;
			std::unordered_map<std::string, double> available_buckets;
			Distribution average_years_younger;
			Distribution coital_events_per_month_high_risk;
			Distribution chance_condom_user_per_event_high_risk;
			Distribution partnership_duration_months_high_risk;
			Distribution coital_events_per_month_low_risk;
			Distribution chance_condom_user_per_event_low_risk;
			Distribution partnership_duration_months_low_risk;
			Distribution activity_level;
			double proportion_high_risk_csw;
			double proportion_high_risk_non_csw;
			int age_discounting_start_age;
			double acquisition_rate_discounting_yearly;
			double coital_acts_discounting_yearly;
		};
		std::unordered_map<PartnershipType, PartnershipSettings> partnership_settings;
		double circumcision_protection_efficacy;
		double condom_protection_efficacy;
		std::array<double, Person::ENDHVLStrata> transmission_coefficients;
	};
	MaleSettings male_settings;
	struct FemaleSettings
	{
		double chance_become_sex_worker;
		double proportion_high_risk_csw;
		double proportion_high_risk_non_csw;
		Distribution activity_level;
		std::array<double, Person::ENDHVLStrata> transmission_coefficients;
	};
	FemaleSettings female_settings;
};

struct Costs
{
	double condom_cost;
	double circumcision_cost;
};

struct Interventions
{
	struct TreatmentFile
	{
		std::string file_name;
		int file_number;
		int time;
		int target_population;
	};

	bool use_art_rollout;
	typedef std::array<TreatmentFile, Constants::NUMBER_OF_CEPAC_FILES> CepacTreatmentFiles;
	CepacTreatmentFiles cepac_treatment_files;
	typedef std::array<TreatmentFile, Constants::NUMBER_OF_ROLLOUT_FILES> RolloutTreatmentFiles;
	RolloutTreatmentFiles rollout_treatment_files;

	struct RolloutEligibility
	{
		struct OIHistCriteria
		{
			int rank;
			std::array<bool, 15> require_oi_history;
			int required_oi_number;
		} oi_history_criteria;
		struct CD4Criteria
		{
			int rank;
			int cd4_lower_bound;
			int cd4_upper_bound;
		} cd4_criteria;
		struct CD4OIHistCriteria
		{
			int rank;
			int cd4_lower_bound;
			int cd4_upper_bound;
			std::array<bool, 15> require_oi_history;
		} cd4_oi_history_criteria;
		struct HVLCriteria
		{
			int rank;
			int hvl_lower_bound;
			int hvl_upper_bound;
		} hvl_criteria;
		struct CD4HVLCriteria
		{
			int rank;
			int cd4_lower_bound;
			int cd4_upper_bound;
			int hvl_lower_bound;
			int hvl_upper_bound;
		} cd4_hvl_criteria;
	} rollout_eligibility;

	std::unordered_map<int, double> target_rollout_proportions;
};

template<typename T>
struct Nullable
{
	bool has_value = false;
	T value;
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

class Inputs
{
public:
	typedef std::array<ConcurrencyDefinition, Constants::NUMBER_CONCURRENCY_DEFS> ConcurrencyDefinitions;

	Inputs(const Inputs &);

	struct TimeDependentParameter
	{
		std::string key;
		int time;
		Nullable<PopulationTarget> target_population;
		double value;
	};

	static Inputs FromFile(const std::string &filename);
	static void ToFile(const Inputs &inputs, const std::string &filename);

	Version GetVersion() const { return version_; }
	int GetDebugLevel() const { return debug_level_; }
	int GetDuration() const { return duration_; }
	int GetFixedSeed() const { return fixed_seed_; }
	int GetMonthOf1990() const { return month_of_1990_; }
	std::string GetFilename() const { return filename_; }
	std::string GetRunName() const { return run_name_; }
	ConcurrencyDefinitions GetConcurrencyDefinitions() const { return concurrency_definitions_; }
	std::unordered_map<TraceFile::Type, TraceFile> GetTraceFiles() const { return trace_files_; }
	CalibrationSettings GetCalibrationSettings() const { return calibration_settings_; }
	Interventions GetInterventions() const { return interventions_; }
	PopulationSettings GetPopulationSettings() const { return population_settings_; }
	std::unordered_map<std::string, TemplateParameter> GetTemplateKeyMap() const { return template_key_map_; }
	std::unordered_map<TemplateParameter, TimeDependentParameter> GetTimeDependentParameters() const { return time_dependent_parameters_; }

private:
	Inputs();
	void operator=(const Inputs &);

	void LoadSimulationParameters(const ticpp::Element &root_node);
	void ValidateSimulationParameters();

	void LoadTracingSettings(const ticpp::Element &root_node, const ticpp::Element &write_trace_node, 
		const ticpp::Element &extension_names_node, const ticpp::Element &toss_files_node);
	void ValidateTracingSettings();

	void LoadCalibrationSettings(const ticpp::Element &calibration_node);
	void ValidateCalibrationSettings();

	void LoadPopulationSettings(const ticpp::Element &population_node);
	void ValidatePopulationSettings();

	void LoadCosts(const ticpp::Element &costs_node);
	void ValidateCosts();

	void LoadInterventions(const ticpp::Element &interventions_node);
	void ValidateInterventions();

	std::string filename_;
	std::string run_name_;
	Version version_;
	int debug_level_;
	int duration_;
	int fixed_seed_;
	int month_of_1990_;
	std::unordered_map<TraceFile::Type, TraceFile> trace_files_;
	ConcurrencyDefinitions concurrency_definitions_;
	CalibrationSettings calibration_settings_;
	PopulationSettings population_settings_;
	Costs costs_;
	Interventions interventions_;

	std::unordered_map<std::string, TemplateParameter> template_key_map_;
	std::unordered_map<TemplateParameter, TimeDependentParameter> time_dependent_parameters_;
};
