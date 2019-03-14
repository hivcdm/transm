#pragma once

#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "parameters/agerangesizecontainer.hpp"
#include "entities/entity.hpp"
#include "entities/demographicprofile.hpp"
#include "entities/sexualpartnership.hpp"

namespace transm {

class Population;

class InfectionsTracker
{
public:
	template<typename T>
    using EntityTypeArray = std::array<T, (std::size_t)DemographicProfile::Gender::Last>;
    using HVLArray = std::array<std::size_t, (std::size_t)HVLStrata::Last>;
	using RiskArray = std::array<std::size_t, (std::size_t)RiskLevel::Last>;
	using PartnershipArray = std::array<unsigned long, (std::size_t)SexualPartnership::Type::Last>;
    using RiskEmploymentArray = std::array<std::array<std::size_t, (std::size_t)DemographicProfile::Employment::Last>, (std::size_t)RiskLevel::Last>;
    using DemographicArray = std::array<std::size_t, DemographicProfile::TotalNumBuckets>;
    using DemographicToDemographicArray = std::array<std::array<std::size_t, DemographicProfile::TotalNumBuckets>, DemographicProfile::TotalNumBuckets>;

	/// <summary>
    /// If this number gets changed, also change it in BucketAge.h
    /// </summary>
	static const int NUMBER_GENERATIONS_TO_TRACE = 6;

private:
	/// <summary>
    /// Total number of incident infections throughout the course of the model,
    /// stratified by HVL of the infector.
    /// </summary>
    HVLArray totalIncidentInfections;

    /// <summary>
    /// Total number of times an HIV infected person had sex with an HIV 
    /// uninfected person, stratified by HVL of the infector. This includes 
    /// exposures that resulted in an infection.
	/// </summary>
    HVLArray totalExposures;

	/// <summary>
    /// This contains prevalent infections of all buckets in an EntityPool. Should be sync'd w/ curr timestep.
    /// </summary>
    std::array<DemographicArray, NUMBER_GENERATIONS_TO_TRACE> currPrevalentInfections;

    EntityTypeArray<RiskEmploymentArray> currPrevalentInfectionsEntityTypeRiskEmployment;

	/// <summary>
    /// This contains prevalent infections of all buckets in an EntityPool 
    /// stratified by age and gender. Should be sync'd w/ curr timestep
    /// </summary>
	EntityTypeArray<AgeRangeSizeContainer> currPrevalentInfectionsEntityTypeAge;

    /// <summary>
    /// This contains prevalent infections of all buckets in an EntityPool 
    /// stratified by entity type. Should be sync'd w/ curr timestep
    /// </summary>
    EntityTypeArray<std::size_t> currPrevalentInfectionsEntityType;

    /// <summary>
	/// This contains profileID's that we will include in our traces.
    /// </summary>
	std::vector<DemographicProfile::ProfileID> profileIDsForDetailedTrace;

    /// <summary>
	/// This is a copy of the sim clock. We keep a copy to know when the time has advanced
	/// and when we need to reset incidence for the timestep.
    /// </summary>
	Time currTimeStep;

	/// <summary>
    /// Infections in the current time step, stratified by HVL of the infector.
    /// </summary>
	HVLArray currTimeStepIncidentInfs;

	/// <summary>
    /// Infections in the current time step, stratified by risk and CSW status
    /// </summary>
    EntityTypeArray<RiskEmploymentArray> currTimeStepIncidentInfsEntityTypeRiskEmployment;

    /// <summary>
	/// Infections in the current time step stratified by Age and Gender
    /// </summary>
    EntityTypeArray<AgeRangeSizeContainer> currTimeStepIncidentInfsEntityTypeAge;

    /// <summary>
	/// sum of age of infection and diagnosis for incident infections in current time step
    /// </summary>
    EntityTypeArray<std::size_t> currTimeStepAgeInfectionSumEntityType;
    EntityTypeArray<std::size_t> currTimeStepAgeInfectionSumSqEntityType;
    EntityTypeArray<std::size_t> currTimeStepNumInfectedEntityType;

    EntityTypeArray<RiskEmploymentArray> currTimeStepAgeInfectionSumEntityTypeRiskEmployment;
    EntityTypeArray<RiskEmploymentArray> currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment;
    EntityTypeArray<RiskEmploymentArray> currTimeStepNumInfectedEntityTypeRiskEmployment;

    /// <summary>
    /// sum of cd4 at transmission for incident infection in current time step
    /// </summary>
	double currTimeStepCD4InfectionSum;
	double currTimeStepCD4InfectionSumSq;
	std::size_t currTimeStepNumInfected;

    /// <summary>
	/// Total Infections in History
    /// </summary>
	AgeRangeSizeContainer totalIncidentInfsAge;

    EntityTypeArray<std::size_t> totalIncidentInfsEntityType;
    EntityTypeArray<RiskEmploymentArray> totalIncidentInfsEntityTypeRiskEmployment;

    RiskArray totalIncidentInfsRiskCSW;
    RiskArray totalIncidentInfsRisk;

    /// <summary>
	/// Exposures in the current time step, stratified by HVL of the infector.
    /// This includes exposures that resulted in an infection.
	/// </summary>
	HVLArray currTimeExposures;
	unsigned long currTimeTotalExposures;
	PartnershipArray currTimeExposuresByType;

	/// <summary>
	/// Number of times a condom was used this time step by partnership type
	/// </summary>
	unsigned long currTimeCondomUse;
	PartnershipArray currTimeCondomUseByType;

	/// <summary>
	/// A queue of the last twelve incidence rates, used to generate a yearly incidence
	/// </summary>
	std::deque<double> lastTwelvePopIncidenceRates;
	std::deque<double> lastTwelveSAPopIncidenceRates;

    /// <summary>
	/// keeps track of infections that happened as a result of sexual activity-
	///  the first dimension represents type of partnerships
	///	the second dimension represents demographic profiles of infectors
	///  the third dimension represents demographic profiles of people who were infected
	/// so you can use this to track infection patterns. e.g. how many SINGLE_MALEs were infected by CSW_FEMALE
    /// </summary>
    std::array<DemographicToDemographicArray, (std::size_t)SexualPartnership::Type::Last> incidentInfectionsByDemographic;

    std::array<EntityTypeArray<EntityTypeArray<std::size_t>>, (std::size_t)SexualPartnership::Type::Last> incidentInfectionsByEntityType;

public :

    /// <summary>
	/// @param _profileIDsToPrint this determines which ProfileID's will be printed in traces. If a ProfileID is not in this vector, then no infections involving them will be printed in traces
    /// </summary>
	InfectionsTracker();

	/* This happens after we parse the active demographic profiles so the arrays can be zero-ed out*/
	void Initialize();

	/**
	This will add any _profileID's that we would like to know about. any _profileID's added by this function
	will appear on more detailed infection traces. If a _profileID is not added, it will not appear on any traces even though
	the infection will be recorded internally
	**/
	void addToDetailedTrace(DemographicProfile::ProfileID _profileID);

	/**
 	@return number of people infected in the current time step
 	**/
 	unsigned int getCurrTimeStepNumInfected();

	/// <summary>
	/// Return total number of incident infections throughout this simulation.
	/// </summary>
	std::size_t getNumIncidentInfections();

    /// <summary>
	/// See how many of 1 type of person infected another.
    /// </summary>
	std::size_t getNumIncidentInfections(DemographicProfile::ProfileID _infectors, 
        DemographicProfile::ProfileID _infecteds);

	//returns the prevalence rate among sexually active pop
	double getSAPrev(Population &_population);

	// returns the annual incidence rate for the population
	double getPopAnnualIncidence();

	// returns the annual incidence rate for the sexually active population
	double getSAPopAnnualIncidence();

	/// <summary>
	/// Records a new exposure regardless of whether an infection happened or not.
    /// </summary>
	void recordExposure(Time time, const Entity *_infector, SexualPartnership::Type partnershipType,
	    bool condomUsed);

	/**
	records a New infection and also prints the infection out to a trace
	**/
	void recordIncidentInfection(Time time, SexualPartnership::Type _partnershipType, const Entity *_infector,
	                             const Entity *_infected);

	/**
	records the cd4 at transmission (requested by clinical out stream)
	**/
	void recordCD4AtTransmission(std::ostream &stream);

	/**
	resets counting of incident infections for time step
	**/
	void resetIncidentInfections(Time time);

	/**
	initializes the counters for incident infections by age and gender
	**/
    void initializeIncidentInfectionsByAge(
        const EntityTypeArray<AgeRangeSizeContainer> &incident_by_entity_type_age,
        const AgeRangeSizeContainer &incident_by_age);

	//takes values of _prevalence and copies into internal prevalence representation BucketDemographicProfile (keyed by _classifierVal)
	void setPrevalentInfections(std::array<DemographicArray, NUMBER_GENERATIONS_TO_TRACE> _prevalenceByBucket,
								const EntityTypeArray<AgeRangeSizeContainer> &_prevalenceByEntityTypeAge,
	                            EntityTypeArray<RiskEmploymentArray> _PrevalenceByRiskGenderEmployment);

	//prints both flings and couple infections (who infected whom) 1 row = 1 month
	/**
	@param _time the current time in the simulation
	@param _outStream the stream to print
	@returns the current prevalence (for GUI purposes)
	**/
	int printInfections(EventParams &_eventParams, Time time, std::ostream &_outStream, Population *_population);

    /// <summary>
    /// Returns total number of incident infections that have occurred during
    /// current timestep.
    /// </summary>
    std::size_t getCurrTimeStepIncidentInfsTotal();

    /// <summary>
	/// Calculates the current annual incidence based on the sum of the last twelve monthly incidence rates
    /// </summary>
	double calculateAnnualIncidence(std::deque<double> lastTwelveIncidenceRates);
};

} // namespace transm
