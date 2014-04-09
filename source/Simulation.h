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
	static PopulationTarget FromString(const std::string &s);

	Nullable<Person::RiskLevel> risk_level;
	Nullable<DmgProfile::Employment> employment;
	Nullable<DmgProfile::SexualActivityStatus> sexual_activity_status;
	Nullable<DmgProfile::Gender> gender;
	Nullable<DmgProfile::RelationshipStatus> relationship_status;
	Nullable<DmgProfile::SexualOrientation> sexual_orientation;
	Nullable<int> age_lower;
	Nullable<int> age_upper;
	Nullable<Person::HIVStatus> observed_hiv_status;
	Nullable<bool> on_treatment;
};

enum class TemplateParameter
{
	//Population
	BirthRate,
	ProportionMale,
	ProportionCircumcised,
	AgeSexualDebutYears,
	//Assortativeness
	AssortativenessSteady,
	AssortativenessRegular,
	AssortativenessCasual,
	AssortativenessCsw,
	//Male:General
	MaleChanceBecomeSexWorker,
	MalePartnerAcqMultWithSteadyHigh,
	MalePartnerAcqMultWithSteadyLow,
	//Male:Steady
	MaleSteadyAcquisitionRateHighMean,
	MaleSteadyAcquisitionRateHighStdDev,
	MaleSteadyAcquisitionRateLowMean,
	MaleSteadyAcquisitionRateLowStdDev,
	MaleSteadyAverageYearsYounger,
	MaleSteadyCoitalEventsPerMonthHigh,
	MaleSteadyCoitalEventsPerMonthLow,
	MaleSteadyChanceCondomUsePerEventHighMean,
	MaleSteadyChanceCondomUsePerEventHighStdDev,
	MaleSteadyChanceCondomUsePerEventLowMean,
	MaleSteadyChanceCondomUsePerEventLowStdDev,
	MaleSteadyPartnershipDurationHighMean,
	MaleSteadyPartnershipDurationHighStdDev,
	MaleSteadyPartnershipDurationHighShift,
	MaleSteadyPartnershipDurationLowMean,
	MaleSteadyPartnershipDurationLowStdDev,
	MaleSteadyPartnershipDurationLowShift,
	//Male:Regular
	MaleRegularAcquisitionRateHighMean,
	MaleRegularAcquisitionRateHighStdDev,
	MaleRegularAcquisitionRateLowMean,
	MaleRegularAcquisitionRateLowStdDev,
	MaleRegularAverageYearsYounger,
	MaleRegularCoitalEventsPerMonthHigh,
	MaleRegularCoitalEventsPerMonthLow,
	MaleRegularChanceCondomUsePerEventHighMean,
	MaleRegularChanceCondomUsePerEventHighStdDev,
	MaleRegularChanceCondomUsePerEventLowMean,
	MaleRegularChanceCondomUsePerEventLowStdDev,
	MaleRegularPartnershipDurationHighMean,
	MaleRegularPartnershipDurationHighStdDev,
	MaleRegularPartnershipDurationHighShift,
	MaleRegularPartnershipDurationLowMean,
	MaleRegularPartnershipDurationLowStdDev,
	MaleRegularPartnershipDurationLowShift,
	//Male:Casual
	MaleCasualAcquisitionRateHighMean,
	MaleCasualAcquisitionRateHighStdDev,
	MaleCasualAcquisitionRateLowMean,
	MaleCasualAcquisitionRateLowStdDev,
	MaleCasualAverageYearsYounger,
	MaleCasualCoitalEventsPerMonthHigh,
	MaleCasualCoitalEventsPerMonthLow,
	MaleCasualChanceCondomUsePerEventHighMean,
	MaleCasualChanceCondomUsePerEventHighStdDev,
	MaleCasualChanceCondomUsePerEventLowMean,
	MaleCasualChanceCondomUsePerEventLowStdDev,
	MaleCasualPartnershipDurationHighMean,
	MaleCasualPartnershipDurationHighStdDev,
	MaleCasualPartnershipDurationHighShift,
	MaleCasualPartnershipDurationLowMean,
	MaleCasualPartnershipDurationLowStdDev,
	MaleCasualPartnershipDurationLowShift,
	//Male:Csw
	MaleCswAcquisitionRateHighMean,
	MaleCswAcquisitionRateHighStdDev,
	MaleCswAcquisitionRateLowMean,
	MaleCswAcquisitionRateLowStdDev,
	MaleCswAverageYearsYounger,
	MaleCswCoitalEventsPerMonthHigh,
	MaleCswCoitalEventsPerMonthLow,
	MaleCswChanceCondomUsePerEventHighMean,
	MaleCswChanceCondomUsePerEventHighStdDev,
	MaleCswChanceCondomUsePerEventLowMean,
	MaleCswChanceCondomUsePerEventLowStdDev,
	MaleCswPartnershipDurationHighMean,
	MaleCswPartnershipDurationHighStdDev,
	MaleCswPartnershipDurationHighShift,
	MaleCswPartnershipDurationLowMean,
	MaleCswPartnershipDurationLowStdDev,
	MaleCswPartnershipDurationLowShift,
	//Male:Health
	MaleTransmissionCoefficientsByHvl,
	MaleTransmissionCoefficientsPrimary,
	MaleTransmissionCoefficientsLate,
	//Female:Behavior
	FemaleChanceBecomeSexWorker,
	FemaleProportionHighRiskCsw,
	FemaleProportionHighRiskNonCsw,
	//Female:Health
	FemaleTransmissionCoefficientsByHvl,
	FemaleTransmissionCoefficientsPrimary,
	FemaleTransmissionCoefficientsLate,
	//Costs
	CondomCost,
	CircumcisionCost,
	//ArtRolloutEligibility:OIHist
	ArtOIHistRank,
	ArtOIHistOI0,
	ArtOIHistOI1,
	ArtOIHistOI2,
	ArtOIHistOI3,
	ArtOIHistOI4,
	ArtOIHistOI5,
	ArtOIHistOI6,
	ArtOIHistOI7,
	ArtOIHistOI8,
	ArtOIHistOI9,
	ArtOIHistOI10,
	ArtOIHistOI11,
	ArtOIHistOI12,
	ArtOIHistOI13,
	ArtOIHistOI14,
	ArtOIHistNumOIToStart,
	//ArtRolloutEligibility:CD4
	ArtCD4Rank,
	ArtCD4CD4Upp,
	ArtCD4CD4Lwr,
	//ArtRolloutEligibility:CD4OIHist
	ArtCD4OIHistRank,
	ArtCD4OIHistOI0,
	ArtCD4OIHistOI1,
	ArtCD4OIHistOI2,
	ArtCD4OIHistOI3,
	ArtCD4OIHistOI4,
	ArtCD4OIHistOI5,
	ArtCD4OIHistOI6,
	ArtCD4OIHistOI7,
	ArtCD4OIHistOI8,
	ArtCD4OIHistOI9,
	ArtCD4OIHistOI10,
	ArtCD4OIHistOI11,
	ArtCD4OIHistOI12,
	ArtCD4OIHistOI13,
	ArtCD4OIHistOI14,
	ArtCD4OIHistCD4Upp,
	ArtCD4OIHistCD4Lwr,
	//ArtRolloutEligibility:HVL
	ArtHVLRank,
	ArtHVLHVLUpp,
	ArtHVLHVLLwr,
	//ArtRolloutEligibility:CD4HVL
	ArtCD4HVLRank,
	ArtCD4HVLCD4Upp,
	ArtCD4HVLCD4Lwr,
	ArtCD4HVLHVLUpp,
	ArtCD4HVLHVLLwr
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
	friend class SimulationBuilder;

	struct TreatmentFile
	{
		std::string file_name;
		int file_number;
		int time;
		int target_population;
	};

	struct TimeDependentParameter
	{
		int time;
		std::string value;
		std::string key;
		Nullable<PopulationTarget> target_population;
		std::function<void(Person *)> population_modifier;
		std::function<void()> simulation_modifier;
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

	std::vector<TimeDependentParameter> time_dependent_parameters_;
};

