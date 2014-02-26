#pragma once

#include <deque>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <boost/tuple/tuple.hpp>

#include "../entities/Person.h"
#include "../entities/classifiers/DmgProfile.h"
#include "../entities/classifiers/SexualPartnership.h"

class Population;

class InfectionsTracker
{

public:
	/** If this number gets changed, also change it in BucketAge.h */
	static const int NUMBER_GENERATIONS_TO_TRACE = 6;

private:
	/** Total number of incident infections throughout the course of the model, stratified by HVL of the infector */
	unsigned int totalIncidentInfections[Person::ENDHVLStrata];

	/** Total number of times an HIV infected person had sex with an HIV uninfected person, stratified by HVL of the infector
	 *
	 * This includes exposures that resulted in an infection
	 **/
	unsigned int totalExposures[Person::ENDHVLStrata];

	/** This contains prevalent infections of all buckets in an EntityPool. Should be sync'd w/ curr timestep */
	unsigned int currPrevalentInfections[DmgProfile::TotalNumBuckets][NUMBER_GENERATIONS_TO_TRACE];
	unsigned int
	currPrevalentInfectionsRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];

	/** This contains prevalent infections of all buckets in an EntityPool stratified by age and gender. Should be sync'd w/ curr timestep */
	vector<boost::tuple<int, int, int>> currPrevalentInfectionsAgeMale;
	vector<boost::tuple<int, int, int>> currPrevalentInfectionsAgeFemale;

	/** This contains profileID's that we will include in our traces. */
	list<DmgProfile::ProfileID> profileIDsForDetailedTrace;

	/** This is a copy of the sim clock. We keep a copy to know when the time has advanced
		and when we need to reset incidence for the timestep */
	unsigned int currTimeStep;

	/** Infections in the current time step, stratified by HVL of the infector */
	unsigned int currTimeStepIncidentInfs[Person::ENDHVLStrata];
	/** Infections in the current time step, stratified by risk and CSW status */
	unsigned int
	currTimeStepIncidentInfsRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];
	/** Infections in the current time step stratified by Age and Gender*/
	vector<boost::tuple<int, int, int>> currTimeStepIncidentInfsAgeMale;
	vector<boost::tuple<int, int, int>> currTimeStepIncidentInfsAgeFemale;

	/** sum of age of infection and diagnosis for incident infections in current time step*/
	unsigned int currTimeStepAgeInfectionSumGender[DmgProfile::ENDGender];
	unsigned int currTimeStepAgeInfectionSumSqGender[DmgProfile::ENDGender];
	unsigned int currTimeStepNumInfectedGender[DmgProfile::ENDGender];

	unsigned int
	currTimeStepAgeInfectionSumRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];
	unsigned int
	currTimeStepAgeInfectionSumSqRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];
	unsigned int
	currTimeStepNumInfectedRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];

	/** sum of cd4 at transmission for incident infection in current time step*/
	double currTimeStepCD4InfectionSum;
	double currTimeStepCD4InfectionSumSq;
	unsigned int currTimeStepNumInfected;

	/** Returns total number of incident infections that have occurred during current timestep */
	unsigned int getCurrTimeStepIncidentInfsTotal();

	/** Total Infections in History*/
	vector<boost::tuple<int, int, int>> totalIncidentInfsAge;
	unsigned int totalIncidentInfsGender[DmgProfile::ENDGender];
	unsigned int
	totalIncidentInfsRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment];
	unsigned int totalIncidentInfsRiskCSW[Person::ENDRiskLevel];
	unsigned int totalIncidentInfsRisk[Person::ENDRiskLevel];
	/** Exposures in the current time step, stratified by HVL of the infector
	 *
	 * This includes exposures that resulted in an infection
	 **/
	unsigned int currTimeExposures[Person::ENDHVLStrata];

	/** A deque of the last twelve incidence rates, used to generate a yearly incidence */
	deque<double> lastTwelveIncidenceRates;

	/** Calculates the current annual incidence based on the sum of the last twelve monthly incidence rates */
	double calculateAnnualIncidence();

	/** keeps track of infections that happened as a result of sexual activity-
	//  the first dimension represents type of partnerships
	//	the second dimension represents demographic profiles of infectors
	//  the third dimension represents demographic profiles of people who were infected
	//so you can use this to track infection patterns. e.g. how many SINGLE_MALEs were infected by CSW_FEMALE */
	unsigned int incidentInfections[SexualPartnership::ENDType][DmgProfile::TotalNumBuckets][DmgProfile::TotalNumBuckets];

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
	void addToDetailedTrace(DmgProfile::ProfileID _profileID);

	/**
	@return total number of incident infections throughout this simulation
	**/
	unsigned int getNumIncidentInfections();

	//see how many of 1 type of person infected another
	unsigned int getNumIncidentInfections(DmgProfile::ProfileID _infectors, DmgProfile::ProfileID _infecteds);

	//see how many of 1 type of person infected another in the context of a particular partnership type
	unsigned int getNumIncidentInfections(SexualPartnership::Type _partnershipType, DmgProfile::ProfileID _infectors,
	                                       DmgProfile::ProfileID _infecteds);

	//returns the prevalence rate among sexually active pop
	double getSAPrev(Population *_population);

	/**
	 * Records a new exposure regardless of whether an infection happened or not
	 */
	void recordExposure(int _time, const Person *_infector);

	/**
	records a New infection and also prints the infection out to a trace
	**/
	void recordIncidentInfection(int _time, SexualPartnership::Type _partnershipType, const Person *_infector,
	                             const Person *_infected, bool _print, ostream &_traceOutStream);

	/**
	records the cd4 at transmission (requested by clinical out stream)
	**/
	void recordCD4AtTransmission(ostream &_outStream);

	/**
	resets counting of incident infections for time step
	**/
	void resetIncidentInfections(int _time);

	/**
	initializes the counters for incident infections by age and gender
	**/
	void initializeIncidentInfectionsByAge(vector<boost::tuple<int, int, int>> &_incMale,
	                                       vector<boost::tuple<int, int, int>> &_incFemale,  vector<boost::tuple<int, int, int>> &_totalIncAge);
	//takes values of _prevalence and copies into internal prevalence representation DmgProfileBucket (keyed by _classifierVal)
	void setPrevalentInfections(int _time,
	                            unsigned int _prevalenceByBucket[DmgProfile::TotalNumBuckets][NUMBER_GENERATIONS_TO_TRACE],
	                            const vector <boost::tuple<int, int, int>> &_prevalenceByAgeMale,
	                            const vector <boost::tuple<int, int, int>> &_prevalenceByAgeFemale,
	                            unsigned int
	                            _PrevalenceByRiskGenderEmployment[Person::ENDRiskLevel][DmgProfile::ENDGender][DmgProfile::ENDEmployment]);

	//-----------------< Begin functions to print out infections >-----------------------//
	//prints both flings and couple infections (who infected whom) 1 row = 1 month
	/**
	@param _time the current time in the simulation
	@param _outStream the stream to print
	@returns the current prevalence (for GUI purposes)
	**/
	int printInfections(EventParams &_eventParams, int _time, std::ostream &_outStream, Population *_population);

	/*
	//prints the headers that label each column of the infections trace file
	void printIncidenceHeaders(std::ostream &_outStream);
	*/
	//-----------------< Begin functions to print out infections >-----------------------//

};
