#pragma once

#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>

#include "data/AgeRangeSizeContainer.hpp"
#include "entities/Person.hpp"
#include "entities/DemographicProfile.hpp"
#include "entities/SexualPartnership.hpp"

class Population;

class InfectionsTracker
{
public:
	/** If this number gets changed, also change it in BucketAge.h */
	static const int NUMBER_GENERATIONS_TO_TRACE = 6;

private:
	/** Total number of incident infections throughout the course of the model, stratified by HVL of the infector */
	unsigned long totalIncidentInfections[Person::ENDHVLStrata];

	/** Total number of times an HIV infected person had sex with an HIV uninfected person, stratified by HVL of the infector
	 *
	 * This includes exposures that resulted in an infection
	 **/
	unsigned long totalExposures[Person::ENDHVLStrata];

	/** This contains prevalent infections of all buckets in an EntityPool. Should be sync'd w/ curr timestep */
	unsigned long currPrevalentInfections[DemographicProfile::TotalNumBuckets][NUMBER_GENERATIONS_TO_TRACE];
	unsigned long
        currPrevalentInfectionsRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last];

	/** This contains prevalent infections of all buckets in an EntityPool stratified by age and gender. Should be sync'd w/ curr timestep */
	AgeRangeSizeContainer currPrevalentInfectionsAgeMale;
	AgeRangeSizeContainer currPrevalentInfectionsAgeFemale;

    /// <summary>
	/// This contains profileID's that we will include in our traces.
    /// </summary>
	std::list<DemographicProfile::ProfileID> profileIDsForDetailedTrace;

    /// <summary>
	/// This is a copy of the sim clock. We keep a copy to know when the time has advanced
	/// and when we need to reset incidence for the timestep.
    /// </summary>
	unsigned int currTimeStep;

	/** Infections in the current time step, stratified by HVL of the infector */
	unsigned long currTimeStepIncidentInfs[Person::ENDHVLStrata];
	/** Infections in the current time step, stratified by risk and CSW status */
	unsigned long
        currTimeStepIncidentInfsRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last];
	/** Infections in the current time step stratified by Age and Gender*/
	AgeRangeSizeContainer currTimeStepIncidentInfsAgeMale;
	AgeRangeSizeContainer currTimeStepIncidentInfsAgeFemale;

	/** sum of age of infection and diagnosis for incident infections in current time step*/
    unsigned long currTimeStepAgeInfectionSumGender[(std::size_t)DemographicProfile::Gender::Last];
    unsigned long currTimeStepAgeInfectionSumSqGender[(std::size_t)DemographicProfile::Gender::Last];
    unsigned int currTimeStepNumInfectedGender[(std::size_t)DemographicProfile::Gender::Last];

	unsigned long
        currTimeStepAgeInfectionSumRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last];
	unsigned long
        currTimeStepAgeInfectionSumSqRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last];
	unsigned int
        currTimeStepNumInfectedRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last];

	/** sum of cd4 at transmission for incident infection in current time step*/
	double currTimeStepCD4InfectionSum;
	double currTimeStepCD4InfectionSumSq;
	unsigned int currTimeStepNumInfected;

	/** Total Infections in History*/
	AgeRangeSizeContainer totalIncidentInfsAge;
    unsigned long totalIncidentInfsGender[(std::size_t)DemographicProfile::Gender::Last];
	unsigned long
        totalIncidentInfsRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last];
	unsigned long totalIncidentInfsRiskCSW[Person::ENDRiskLevel];
	unsigned long totalIncidentInfsRisk[Person::ENDRiskLevel];
	/** Exposures in the current time step, stratified by HVL of the infector
	 *
	 * This includes exposures that resulted in an infection
	 **/
	unsigned long currTimeExposures[Person::ENDHVLStrata];

	/** A deque of the last twelve incidence rates, used to generate a yearly incidence */
	deque<double> lastTwelveIncidenceRates;

	/** Calculates the current annual incidence based on the sum of the last twelve monthly incidence rates */
	double calculateAnnualIncidence();

	/** keeps track of infections that happened as a result of sexual activity-
	//  the first dimension represents type of partnerships
	//	the second dimension represents demographic profiles of infectors
	//  the third dimension represents demographic profiles of people who were infected
	//so you can use this to track infection patterns. e.g. how many SINGLE_MALEs were infected by CSW_FEMALE */
	unsigned long incidentInfections[(int)SexualPartnership::Type::ENDType][DemographicProfile::TotalNumBuckets][DemographicProfile::TotalNumBuckets];

public :

	/**
	@param _profileIDsToPrint this determines which ProfileID's will be printed in traces. If a ProfileID is not in this vector, then no infections involving them will be printed in traces
	**/
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
	void recordExposure(long _time, const Person *_infector);

	/**
	records a New infection and also prints the infection out to a trace
	**/
	void recordIncidentInfection(long _time, SexualPartnership::Type _partnershipType, const Person *_infector,
	                             const Person *_infected, bool _print, ostream &_traceOutStream);

	/**
	records the cd4 at transmission (requested by clinical out stream)
	**/
	void recordCD4AtTransmission(ostream &_outStream);

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
	void setPrevalentInfections(long _time,
	                            unsigned long _prevalenceByBucket[DemographicProfile::TotalNumBuckets][NUMBER_GENERATIONS_TO_TRACE],
								const AgeRangeSizeContainer &_prevalenceByAgeMale,
								const AgeRangeSizeContainer &_prevalenceByAgeFemale,
	                            unsigned long
                                _PrevalenceByRiskGenderEmployment[Person::ENDRiskLevel][(std::size_t)DemographicProfile::Gender::Last][(std::size_t)DemographicProfile::Employment::Last]);

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
