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
    using EntityTypeMap = std::unordered_map<std::string, T>;
    using HVLArray = std::array<std::size_t, (std::size_t)Entity::HVLStrata::Last>;
    using RiskEmploymentArray = std::array<std::array<std::size_t, (std::size_t)DemographicProfile::Employment::Last>, (std::size_t)Entity::RiskLevel::Last>;
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

    EntityTypeMap<RiskEmploymentArray> currPrevalentInfectionsEntityTypeRiskEmployment;

	/// <summary>
    /// This contains prevalent infections of all buckets in an EntityPool 
    /// stratified by age and gender. Should be sync'd w/ curr timestep
    /// </summary>
	EntityTypeMap<AgeRangeSizeContainer> currPrevalentInfectionsEntityTypeAge;

    /// <summary>
	/// This contains profileID's that we will include in our traces.
    /// </summary>
	std::list<DemographicProfile::ProfileID> profileIDsForDetailedTrace;

    /// <summary>
	/// This is a copy of the sim clock. We keep a copy to know when the time has advanced
	/// and when we need to reset incidence for the timestep.
    /// </summary>
	int currTimeStep;

	/// <summary>
    /// Infections in the current time step, stratified by HVL of the infector.
    /// </summary>
	HVLArray currTimeStepIncidentInfs;

	/// <summary>
    /// Infections in the current time step, stratified by risk and CSW status
    /// </summary>
    EntityTypeMap<RiskEmploymentArray> currTimeStepIncidentInfsEntityTypeRiskEmployment;

    /// <summary>
	/// Infections in the current time step stratified by Age and Gender
    /// </summary>
    EntityTypeMap<AgeRangeSizeContainer> currTimeStepIncidentInfsEntityTypeAge;

    /// <summary>
	/// sum of age of infection and diagnosis for incident infections in current time step
    /// </summary>
    EntityTypeMap<std::size_t> currTimeStepAgeInfectionSumEntityType;
    EntityTypeMap<std::size_t> currTimeStepAgeInfectionSumSqEntityType;
    EntityTypeMap<std::size_t> currTimeStepNumInfectedEntityType;

    EntityTypeMap<RiskEmploymentArray> currTimeStepAgeInfectionSumEntityTypeRiskEmployment;
    EntityTypeMap<RiskEmploymentArray> currTimeStepAgeInfectionSumSqEntityTypeRiskEmployment;
    EntityTypeMap<RiskEmploymentArray> currTimeStepNumInfectedEntityTypeRiskEmployment;

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

    EntityTypeMap<std::size_t> totalIncidentInfsEntityType;
    EntityTypeMap<RiskEmploymentArray> totalIncidentInfsEntityTypeRiskEmployment;

    std::array<std::size_t, (std::size_t)Entity::RiskLevel::Last> totalIncidentInfsRiskCSW;
    std::array<std::size_t, (std::size_t)Entity::RiskLevel::Last> totalIncidentInfsRisk;

    /// <summary>
	/// Exposures in the current time step, stratified by HVL of the infector. 
    /// This includes exposures that resulted in an infection.
	/// </summary>
	HVLArray currTimeExposures;

	/// <summary>
    /// A queue of the last twelve incidence rates, used to generate a yearly incidence
    /// </summary>
	std::deque<double> lastTwelveIncidenceRates;

    /// <summary>
	/// Calculates the current annual incidence based on the sum of the last twelve monthly incidence rates
    /// </summary>
	double calculateAnnualIncidence();

    /// <summary>
	/// keeps track of infections that happened as a result of sexual activity-
	///  the first dimension represents type of partnerships
	///	the second dimension represents demographic profiles of infectors
	///  the third dimension represents demographic profiles of people who were infected
	/// so you can use this to track infection patterns. e.g. how many SINGLE_MALEs were infected by CSW_FEMALE
    /// </summary>
    std::array<DemographicToDemographicArray, (std::size_t)SexualPartnership::Type::ENDType> incidentInfections;

public :

    /// <summary>
	/// @param _profileIDsToPrint this determines which ProfileID's will be printed in traces. If a ProfileID is not in this vector, then no infections involving them will be printed in traces
    /// </summary>
	InfectionsTracker();

	/**
	This will add any _profileID's that we would like to know about. any _profileID's added by this function
	will appear on more detailed infection traces. If a _profileID is not added, it will not appear on any traces even though
	the infection will be recorded internally
	**/
	void addToDetailedTrace(DemographicProfile::ProfileID _profileID);

	/**
	@return total number of incident infections throughout this simulation
	**/
	unsigned long getNumIncidentInfections();

	//see how many of 1 type of person infected another
	unsigned long getNumIncidentInfections(DemographicProfile::ProfileID _infectors, DemographicProfile::ProfileID _infecteds);

	//see how many of 1 type of person infected another in the context of a particular partnership type
	unsigned long getNumIncidentInfections(SexualPartnership::Type _partnershipType, DemographicProfile::ProfileID _infectors,
	                                       DemographicProfile::ProfileID _infecteds);

	//returns the prevalence rate among sexually active pop
	double getSAPrev(Population &_population);

	/**
	 * Records a new exposure regardless of whether an infection happened or not
	 */
	void recordExposure(long _time, const Entity *_infector);

	/**
	records a New infection and also prints the infection out to a trace
	**/
	void recordIncidentInfection(long _time, SexualPartnership::Type _partnershipType, const Entity *_infector,
	                             const Entity *_infected, bool _print, std::ostream &_traceOutStream);

	/**
	records the cd4 at transmission (requested by clinical out stream)
	**/
	void recordCD4AtTransmission(std::ostream &_outStream);

	/**
	resets counting of incident infections for time step
	**/
	void resetIncidentInfections(long _time);

	/**
	initializes the counters for incident infections by age and gender
	**/
	void initializeIncidentInfectionsByAge(const AgeRangeSizeContainer &_incMale, 
		const AgeRangeSizeContainer &_incFemale, const AgeRangeSizeContainer &_totalIncAge);

	//takes values of _prevalence and copies into internal prevalence representation BucketDemographicProfile (keyed by _classifierVal)
	void setPrevalentInfections(int time,
	                            std::array<DemographicArray, NUMBER_GENERATIONS_TO_TRACE> _prevalenceByBucket,
								const EntityTypeMap<AgeRangeSizeContainer> &_prevalenceByEntityTypeAge,
	                            EntityTypeMap<RiskEmploymentArray> _PrevalenceByRiskGenderEmployment);

	//prints both flings and couple infections (who infected whom) 1 row = 1 month
	/**
	@param _time the current time in the simulation
	@param _outStream the stream to print
	@returns the current prevalence (for GUI purposes)
	**/
	int printInfections(EventParams &_eventParams, long _time, std::ostream &_outStream, Population *_population);

    /** Returns total number of incident infections that have occurred during current timestep */
    unsigned long getCurrTimeStepIncidentInfsTotal();
};

} // namespace transm
