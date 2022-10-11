#ifndef ENTITY_HPP
#define ENTITY_HPP

/* Standard essentials */
#include <iostream>
#include <list>
#include <set>
#include <vector>
#include <include.h>

#include "entitytypes.hpp"
#include "demographicprofile.hpp"
#include "prep.hpp"
#include "sexualpartnership.hpp"
#include "transmissiontype.hpp"
#include "core/constants.hpp"
#include "entitypool/fullvector.hpp"
#include "statistics/statsrecord.hpp"
#include "utility/time.hpp"
#include "utility/utility.hpp"

/* BOOST essentials */
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>


namespace transm {

/** Forward declarations */
class ArtRolloutTracker;
class PrepTracker;
class CostsTracker;
class EntityPool;
class EventParams;
class FullVector;
class InfectionsTracker;
class RandomNumberGenerator;

/** All individuals in the simulation are of this class, or something derived from this
 * Fields and Methods are divided into the following categories:
 * Physical, Relational, DemographicProfile-related, other */
class Entity {

    /** used to create unique id's for each person this increments every time a New person is created */
    static long idCounter;

public:

    virtual void Circumcise() = 0;

    void SetSexualActivityDelay(TimeSpan delay) { sexualActivityDelay = delay; }

    TimeSpan GetSexualActivityDelay() const { return sexualActivityDelay; }

    virtual bool IsCircumcised() const = 0;

    virtual void SetProportionHighRisk(DemographicProfile::Employment employment, double proportion) = 0;

    double GetPreExposureProphylaxisEfficacy() const;

    static const std::array<std::string, (std::size_t) RiskLevel::Last> RiskStrings;

    virtual void SetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType, double chance) = 0;

    virtual double GetPartnershipRejectionChance(RiskLevel risk, SexualPartnership::Type partnershipType) const = 0;

    virtual void SetOverrideChanceCondomUse(double chance) = 0;

    virtual double GetOverrideChanceCondomUse() const = 0;

    bool HasOverrideChanceCondomUse() const { return GetOverrideChanceCondomUse() != -1; }

    /** We have made these stats referencable by enum so that we can more easily create
     * customizable outputs or reports we can perhaps have easier look-up of stat descriptions
     * if we choose to write some up. */
    enum class Stats {
        STAT_TOTAL_LM,                            /**< months lived during sim.  */
        STAT_HIV_NEG_LM,                          /**< life months lived as HIV Negative */
        STAT_HIV_POS_POSTINFECT_LM,               /**< life months lived after HIV infection */
        STAT_EXPOSURES_BEFORE_INF,                /**< Number times exposed to HIV but not infected */
        STAT_NUM_INFECTED,                        /**< Number of people that someone infected */
        STAT_AGE_AT_INFECTION_MTH,                /**< age when infection occurred */
        STAT_TIME_OF_INFECTION_MTH,               /**< calendar time of infection */
        STAT_GENERATION_OF_INFECTION,             /**< generation of infection: prevalent case is 0, otherwise (1+generation of infectors infection) */
        Last,
        First = STAT_TOTAL_LM
    };

    /* string representations of enum Stats */
    static const std::vector<std::string> StatsStr;

    /* this is a enum class wrapper that has helpful enum-related functions */
    static EnumCls <Stats> StatsEnum;

    /* this is a type declaration of a class that keeps track of statistics defined in enum Stats */
    using EntityStatsRecord = StatsRecord<Stats, BaseEnumCls::NULL_ENUM>;

    /**
     *  the CEPAC death table has stats for 0-100 years old.
     *  people automatically die at this age in the dynamic model */
    const static int maxYrForDeathStats = 101;

    /* contains probabilities of nonAIDS-death, read from CEPAC .in file */
    static std::vector<double> probDeathNatCauses[(std::size_t) DemographicProfile::Gender::Last];

    /* there is option to print patient traces to a text file. this keeps track of how many we've done so far */
    static int numTracesSoFar;

    /** Setting assortativenesses */
    virtual void SetRiskAssortativeness(double assortativeness) = 0;

    virtual void SetRaceEthnicAssortativeness(DemographicProfile::Race race, DemographicProfile::Ethnicity ethnicity,
                                              double assortativeness) = 0;

    void UsePreExposureProphylaxis(double adherence);

    void SetTargetedCepacContext(SimContext *context) {
        setSimContext(context);
        targetedCepacContext_ = context;
    }

    bool HasTargetedCepacContext() const {
        return targetedCepacContext_ != nullptr;
    }

    SimContext *GetTargetedCepacContext() const {
        return targetedCepacContext_;
    }

protected:

    unsigned int populationID;      /**< keeps track of which population this Entity belongs to */

    unsigned long id;               /**< person's unique id number */

    SimContext *targetedCepacContext_;

    /**
     * person's current demographic profile - values in here depend on person's
     * physical, relational state, and other preferences */
    DemographicProfile dmgProfile;

    /**
     * person keeps track of which BucketDemographicProfile they are currently in
     * this value should stay equal to dmgProfile->getProfileID()
     * sometimes a person's dmgProfile is changed, so we have to refresh their
     * place in the EntityPool */
    DemographicProfile::ProfileID currentBucketID;

    /**
     * Entity's relational state
     * contains all current partnerships including CSW and Casual  */
    std::list<SexualPartnership *> partners[(int) SexualPartnership::Type::Last];

    /* array of number of partners over persons history stratified by partnership type */
    int numPartnersInHistory[(int) SexualPartnership::Type::Last];

    /* array of month of their farthest current partnership dissolution time for each partnership type.  initialized to zero */
    Time monthOfLatestPartnershipDissolution[(int) SexualPartnership::Type::Last];

    /* array of month of latest concurrent relationship for each partnership type. Only updated for 12 months before calibration */
    Time monthOfLatestConcurrent;

    /**
     * Contains the number of partnerships the person tried to form over time, but didn't
     * (usually due to no partners available or re-hooking up with a current partner) */
    int unformedPartnershipsTotal[(int) SexualPartnership::Type::Last];
    int unformedPartnershipsLatestTime[(int) SexualPartnership::Type::Last];

    /** Which generation was the person infected in */
    int generationOfInfection;

    /** flag to indicate whether this person has CEPAC data */
    bool wentThroughCEPAC;

    /** a pointer to a CEPAC patient object which stores all post-infection health states */
    Patient *cepacPatient;

    /** A running tally of all costs accrued through CEPAC */
    double CEPACcosts;

    /** Used to keep track of number of acts this month */
    int numActsThisMonth;

    /** Used to keep track of number of condoms used in a given month for cost purposes */
    int condomsUsedThisMonth;

    /** Used to keep track of whether or not a condom was used the last time getFOI was called */
    bool condomUsedLastFOICalculation;

    /** currently defaults to "LOW" and 1 */
    RiskLevel risk;

    /** statistical information from this individual */
    EntityStatsRecord stats;

    /** True if this person should be followed in singlePersonTrace file */
    bool traceMe;

    /** The indices which point to the person in their assigned FullVector */
    std::map<FullVector *, std::vector<unsigned int>> FVindices;

    TimeSpan sexualActivityDelay;

public:

    /** dummy constructor */
    Entity();

    /** this constructor creates an actual person that can be simulated. It is generally called by Male and Female
     * we pass in _eventParams because becomeInfected() needs it... */
    Entity(Age age, unsigned int _populationID, const PrepParameters &prepParameters);

    /** deconstructor */
    virtual ~Entity();

    /** age of Entity (in months) */
    Age age;

    /** age of Entity on model init (in months) */
    Age initAge;

    /** age of Entity when they got infected (-1 for uninfected) */
    Age ageInfected;

    /** Age of Entity when is in care */
    Age ageInCare;

    /** Age of Entity when is detected HIV */
    Age ageDetected;

    /** whether this Entity is already detected HIV */
    bool alreadyDetected;

    bool newDiagnosis;

    bool inCareWithinThirty;

    /** whether this Entity is dead or not */
    bool death;

    /** Entity's infected status */
    HIVStatus hivStatus;

    /** CD4 cell count */
    double cd4;

    /** HIV Viral Load */
    HVLStrata hvl;

    /** HIV Viral Load of person from patient object.  No primary or late stage stratas */
    HVLStrata currentTrueHvl;

    /** Whether this person has observed HIV */
    bool isObserved;

    /** OI HIstory */
    bool oiHistory[Constants::NumberOfOIs];

    DeathStatus deathStatus;

    CD4Strata getCd4Stratum() const;

    HVLStrata getHvlStratum() const;

    bool isEligibleForTreatment(const SimContext::TreatmentInputs::ARTStartPolicy &artStartPolicy);

    /** This is for keeping dead people around for graph printing reasons It mimics the destructor
     * without destroying the Entity object.  */
    void deleteEntityWithoutDeleting();

    void ageOneTimeUnit();

    /** call this to infect person...
     * if CEPAC bridge is in place, will call CEPAC to determine the health trajectory of this person
     * @param _generationOfInfection : if true, than this person was a prevalent infection
     * @param _eventParams */
    void becomeInfected(int _generationOfInfection, EventParams &_eventParams);

    void seedInfection(int _generationOfInfection, EventParams &_eventParams, bool chronicInfection);

    /** Initializes cepacPatient using the persons current age, gender, and infection status.
     * Prevalent cases should call "becomeInfected" before calling this function; incident cases will become
     * infected later. */
    void initializeCEPACpatient(EventParams &_eventParams);

    void updateCEPACpatient(EventParams &_eventParams);

    virtual double getChanceBecomeCsw() const = 0;

    virtual bool PassedCSWEndAge() const = 0;

    /**
     * @return generation of infection */
    int getGenerationOfInfection(bool cap_at_5 = true) const;

    /**
     * @return the number of partners by partnership type */
    int getNumPartners(SexualPartnership::Type);

    /**
     * @return the number of partners by partnership type that are either the samerisk or different */
    int getNumPartners(SexualPartnership::Type, bool);

    double getQualityOfLife() const {
        return cepacPatient != nullptr ? cepacPatient->getGeneralState()->QOLMultiplier : 1;
    }

    double getCepacDiscountFactor(EventParams &parameters) const {
        auto context = parameters.useRollout ? parameters.untreatedContext :
                       parameters.cepacSimContexts[getCEPACSimContextIndex(parameters)];
        return Utility::computeCepacDiscountFactor(context->getRunSpecsInputs()->discountFactor,
                                                   parameters.currTime.in_months());
    }

    /**
     * @return the number of partners in history */
    int getNumPartnersInHistory();

    /**
     * @return number of partners in history stratified by type */
    int getNumPartnersInHistory(SexualPartnership::Type);

    /**
     * @return month of latest partnership dissolution (may be in the future) for given partner type */
    Time getMonthOfLatestPartnershipDissolution(SexualPartnership::Type);


    /** gets and sets month of latest concurrent */
    void setTimeOfLatestConcurrent(Time time);

    Time getTimeOfLatestConcurrent();

    /**
     * @return hvl  */
    HVLStrata getHVL() const {
        return hvl;
    }

    /** this calculates the FOI towards Entity _p (this uses the Transmission coefficient) per event
     * @param _p - partner
     * @param _parteringType - whether this is a fling or steadyCouple */
    virtual double getFOI(Entity *_p,
                          const std::unordered_map<TransmissionType, std::array<double, (std::size_t) HVLStrata::Last>> &transmission_coefficients,
                          SexualPartnership::Type _partnershipType, EventParams &_eventParams) = 0;

    /**
     * @return true if person is currently alive */
    bool isAlive() const;

    /**
     * @return true if person is currently infected */
    bool isInfected() const;

    /** Self explanatory I'd say! */
    bool isSexuallyActive();

    /** Commercial Sex Worker */
    bool isCSW() const;

    /** Have penis */
    bool isMale() const;

    /** Black */
    bool isBlack() const;

    /** White */
    bool isWhite() const;

    /** Hispanic is an ethnicity*/
    bool isHispanic() const;

    /** see whether person dies. If they went through CEPAC, use health trace. else roll against nonAIDS death probs */
    bool rollForDeath(RandomNumberGenerator &_randomNums);

    /** update health status of HIV infected people -- i.e. cd4, hvl, art, etc.
     *  in version 1, this information is taken from CEPAC model
     * @return: costs (accrued in CEPAC) of updating health */
    double
    updateHealthStatus(EventParams &_eventParams, ArtRolloutTracker *testTracker, CostsTracker *costsTracker);

    void updatePrepStatus(EventParams &_eventParams, PrepTracker *prepTracker);


    /** Call this after all transmission/population dynamics are done.
     * Runs infected through CEPAC until they die and adds their LM etc to CEPAC stats */
    void runCEPACtoDeath(RandomNumberGenerator &_randomNums);

    /** Sets the condom total to 0 */
    void resetCondomUsage();

    /** Sets numacts to 0 */
    void resetNumActs();

    /** Return the number of condoms used */
    int getCondomsUsedThisMonth();

    /** Return num acts this month */
    int getNumActsThisMonth();

    /** Increase condoms used this month by a given number (default 1) */
    void incrementCondomsUsedThisMonth(int condoms = 1);

    /** Increase num acts this month */
    void incrementNumActsThisMonth(int _numActs);

    /** Return true if a condom was used the last time FOI was called */
    bool getCondomUsedLastFOICalculation();

    /**
     * @function: setFVindices
     * @arguments: vector<int> FVind, FullVector* FV
     * @effects: if this.FVindices is currently empty and all indices correlate with members
     * of FV that point to this, sets this.FVindices to FVind
     * @return: true if this.FVindices was set to FVind or false otherwise */
    bool setFVindices(std::vector<unsigned int> FVind, FullVector *FV);

    /**
     * @function: addFVindices
     * @arguments: int index, FullVector* FV
     * @effects: If FV[index] points to this and index is not already a member of
     * this.FVindices, adds index to this.FVindices
     * @return: true if index was added to this.FVindices or false otherwise */
    bool addFVindices(int index, FullVector *FV);

    /**
     * @function: removeFVindices
     * @arguments: int index, FullVector* FV
     * @effects: If FV[index] does not point to this, removes index from this.FVindices
     * @return: true if index was removed from this.FVindices, false otherwise */
    bool removeFVindices(int index, FullVector *FV);

    /**
     * @function: memberFVindices
     * @arguments: int index, FullVector* FV
     * @effects: none
     * @return: true iff this.FVindices contains index */
    bool memberFVindices(int index, FullVector *FV);

    /**
     * @function: getFVindices
     * @arguments: none
     * @effects: none
     * @return: copy of this.FVindices */
    std::vector<unsigned int> getFVindices(FullVector *FV);

    /**
     * @function: getRiskLevel
     * @return: this.risk */
    RiskLevel getRiskLevel() const;

    /**
     * @function: getHIVStatus
     * @return: this.hivStatus */
    HIVStatus getHIVStatus() const;

    /**
     * @function: getSexualActivity
     * @return: this.activityLevel */
    int getSexualActivity();

    virtual const std::string getEntityType() const = 0;

    /**changes this person to sexually active and initializes the CEPAC person */
    void becomeSexuallyActive(EventParams &_eventParams);

    /** returns structure that holds current DemographicProfile */
    const DemographicProfile *getDemographicProfile() const;

    template<typename D>
    D getDemographicProfileVal() const;

    /** sets and gets current BucketDemographicProfile membership */
    DemographicProfile::ProfileID getCurrBucketProfileID() const;

    void setCurrBucketProfileID(DemographicProfile::ProfileID _profileID);

    /** returns true if the person's DemographicProfile matches their current BucketDemographicProfile membership */
    bool inCorrectBucketDemographicProfile();

    /** changes isSexWorker with probability taken from population prevalence of CSW (or initial csw chance
     * if prevalent population)  */
    void rollForBecomeSexWorker(EventParams &_eventParams);

    /** Stop being csw  */
    void quitSexWork(EventParams &_eventParams);

    /** rerolls risk group based on if they are csw or not.  Called after rolling for becoming sex worker */
    virtual void rerollRiskGroup(EventParams &_eventParams) = 0;

    /** Sets a new SimContext for the person  */
    void setSimContext(SimContext *newSimContext);

    /** stores data to indicate that this person is in a sexual partnership
     * if this partnership is STEADY, then will change RelationshipStatus */
    void addPartnership(SexualPartnership *_partnership);

    /** returns true if this person is available for steady partnership
     * however, this does not change the person's DemographicProfile value that corresponds to
     * DemographicProfile::Demographic::RelationshipStatus */
    bool availableForPartnership(SexualPartnership::Type _partnershipType) const;

    /**
     * @return true if this person is already in some sort of REGULAR or STEADY partnership with _p */
    bool isPartneredWith(Entity *_p);

    /**
     * @return true if the person is in a relationship of the given type */
    bool hasPartnership(SexualPartnership::Type);

    /**
     * @return true if person is in ANY partnership */
    bool hasPartnership();

    /**
     * These enums expose characterstics of a person for the purpose of indexing or to assist for partner selection.
     *
     * It is used a data structure that has a template argument for sorting key
     *
     * If we change any enums here, we should change:
     *    class Entity::Sorter;
     *   _KeyValType getMinPartnerSelectVal(SexualPartnership::Type _partnershipType, Gender _partnerGender) const; - for class Male, Female
     *   _KeyValType getMaxPartnerSelectVal(SexualPartnership::Type _partnershipType, Gender _partnerGender) const; - for class Male, Female
     */
    enum SelectingCriteria {
        AGE,                        /** unsigned int */
        ID,                         /** unsigned int */
        ENDSelectingCriteria
    };

    /** gets the upper and lower bounds for an acceptable SelectingCriteria values of a potential partner
     * we are not allowed to have virtual templated functions... so we are forced to set return as double
     * @param _partnershipType - type of partnership this person is seeking
     * @param _partnerGender - gender of prospective partner */
    virtual double
    getMinPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const = 0;

    virtual double
    getMaxPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const = 0;

    /** Returns the age difference (in years) to center around */
    virtual double
    rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums) = 0;

    /** checks to see whether the duration limit of any SexualPartnerships have elapsed and will add them to a list to be removed
     * note: this method does not remove any partnerships from this person.
     * @param _fromDeath we are ending b/c this person has died. so will force all partnerships of this type to end
     * @param _partnershipsToEnd when method is complete, _partnershipsToEnd will contain partnerships that should end.
     * @return number of partnerships ended */
    virtual long getPartnershipsToEnd(Time currTime, EventParams &_eventParams, SexualPartnership::Type _partnershipType,
                              list<SexualPartnership *> &_partnershipsToEnd, bool _fromDeath) = 0;

    /** have sex with all partners where the SexualPartnership has a duration. To prevent double-counting activity (iterator hits both partners)
     * sexual activity will only happen for the SexualPartnerships where this person is partner1
     * @return returns a pointer to the person who infected this person. */
    Entity *allPartnerSexualActivity(EventParams &_eventParams, SexualPartnership::Type _partnershipType,
                                     list<Entity *> &_newlyInfected, InfectionsTracker *infTrack,
                                     const std::unordered_map<TransmissionType, std::array<double, (std::size_t) HVLStrata::Last>> &transmission_coefficients);

    /** returns whether this person could partner with Entity _p
     * split this by gender because there might be behaviour differences between them */
    virtual bool possibleMatch(SexualPartnership::Type _partnershipType, Entity *_p) = 0;

    /** removes indications that this person is a particular sexual partnership
     * this is called when that partnership separates
     * will remove pointers to the SexualPartnership from both partners' partner lists */
    void removePartnership(SexualPartnership *_partnership);

    /** for a New partnership, roll how this person wants to be in this relationship */
    virtual int
    rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums,
                                  Entity *_p) = 0;

    /** for a particular month, choose how many partners of _partnershipType this Entity will have */
    virtual int
    rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType) = 0;

    /** for a particular partner, choose how many events this male will have */
    virtual int rollNumEventsPerPartner(Entity *_p, RandomNumberGenerator &_randomNums,
                                        SexualPartnership::Type _partnershipType) = 0;

    virtual std::size_t GetTimesSelected() const = 0;

    virtual void IncrementTimesSelected() = 0;

    virtual void ResetTimesSelected() = 0;

    /** sexual activity with person _p. This can happen within context of class SexualPartnership or just between to Entitys
     * @param _p partner for sexual activity
     * @param _numActs number of sexual acts that happened
     * @param _randomNums random number generator
     * @param _infectionsTracker tracks the number of inf
    *  @return a pointer to a person who has been newly infected. nullptr if no infection occured */
    Entity *
    sexualActivity(Entity *_p, int _numActs, SexualPartnership::Type _partnershipType, EventParams &_eventParams,
                   InfectionsTracker *infTrack,
                   const std::unordered_map<TransmissionType, std::array<double, (std::size_t) HVLStrata::Last>> &transmission_coefficients);

    /** gets the age of the person */
    Age getAge() const;

    /** returns the unique id number of this person */
    unsigned long getID() const;

    unsigned int getPopulationID() const;

    /** returns traceMe */
    bool trace() const;

    /** sets traceMe to true */
    void setToBeTraced();

    const EntityStatsRecord *getStats();

    void enableInfectionTrace(int _generationOfInfection,
                              EventParams &_eventParams) const;

    /** Unformed partnership tallies getters and setters -- the total should never be reset,
     * only the "latest" (i.e. current time step) */
    int getTotalUnformedPartnerships(SexualPartnership::Type type);

    int getLatestUnformedPartnerships(SexualPartnership::Type type);

    void increaseUnformedPartnershipTallies(SexualPartnership::Type type);

    void resetLatestUnformedPartnerships(SexualPartnership::Type type);

    /** Check if the patient is on ART */
    bool isOnArt() {
        return cepacPatient && cepacPatient->getARTState()->isOnART;
    }

    /** If they are in the first line of ART */
    bool isOnFirstLineART() {
        return (cepacPatient && cepacPatient->getARTState()->currRegimenNum == 0);
    }

    /** If they are in the second line of ART */
    bool isOnSecondLineART() {
        return (cepacPatient && cepacPatient->getARTState()->currRegimenNum == 1) ;
    }

    /** If they RTC (Returned to Care) after loss to follow up */
    bool isRTC() {
        if (cepacPatient && cepacPatient->getMonitoringState()->careState == SimContext::HIV_CARE_RTC) {
//            if (cepacPatient->getARTState()->hasNextRegimenAvailable) {
//                cout << "Has next regimen" << cepacPatient->getARTState()->currRegimenNum << endl;
//            }  else {
//                cout << "Has NO next regimen" << cepacPatient->getARTState()->currRegimenNum << endl;
//            }
            return true;
        }
        return false;
    }

    void isSwitchedRegimen() {
        if (cepacPatient && cepacPatient->getARTState()->currRegimenNum != cepacPatient->getARTState()->prevRegimenNum) {
            cout << "Switched the REgimen" << endl;
        }
    }

    /** Check if the patient is linked to care.
     * Important notes: 1) The patient can be linked but be HIV negative.
     *                  2) The patient can be linked but not HIV_CARE_IN_CARE. */
    bool isLinked() const {
        return cepacPatient && cepacPatient->getMonitoringState()->isLinked;
    }

    /** Check if the patient is unlinked. */
    bool isUnLinked() const {
        return (cepacPatient && cepacPatient->getMonitoringState()->careState == SimContext::HIV_CARE_UNLINKED);
    }

    /** Check if the patient is Loss to Follow Up (LTFU) */
    bool isLTFU() const {
        if (cepacPatient && cepacPatient->getMonitoringState()->careState == SimContext::HIV_CARE_LTFU) {
//            if (cepacPatient->getMonitoringState()->wasOnARTWhenLostToFollowUp) {
//                cout << "Was ON ART when LTFU" << endl;
//            } else {
//                cout << "Was OFF ART when LTFU" << endl;
//            }
            return true;
        }
        return false;
    }

    /** This is the same as the definition of Person Living With HIV (PLWH)
     * @return if the patient is detected HIV positive */
    bool isDetected() const {
        return (cepacPatient && cepacPatient->getMonitoringState()->isDetectedHIVPositive);
    }

    int getARTRegimen() const {
        return cepacPatient->getARTState()->currRegimenNum;
    }

    void getClinicVisitType() {
        if (cepacPatient->getMonitoringState()->clinicVisitType == SimContext::CLINIC_INITIAL)
            cout << "Initial" << endl;
        else if (cepacPatient->getMonitoringState()->clinicVisitType == SimContext::CLINIC_INIT_ACUTE)
            cout << "Acute Init" << endl;
        else if (cepacPatient->getMonitoringState()->clinicVisitType == SimContext::CLINIC_SCHED)
            cout << "Scheduled" << endl;
        else
            cout << "Unknown" << endl;
    }

    void getMonthOfDetected() {
        cout << "Month of infected: " << cepacPatient->getDiseaseState()->monthOfHIVInfection << endl;
        cout << "Month of detected: " << cepacPatient->getMonitoringState()->monthOfDetection << endl;

    }

    /** Check if the patient is actually in care:
     * Definition of Person "In Care" according to HIV Care Continuum, Miami 2014-2018 is to have at least one
     * documented VL or CD4 lab, medical visit or prescription from the first day of the year to the last day of
     * the third months of next year.
     *
     * Our definition for this is whoever has been in care is counted as one. This included any entity who is Loss to
     * Follow up (LTFU) or just returned in care (RTC). This counts for anyone who has ever been in care. */
    bool isInCare() const {

        return (cepacPatient && (cepacPatient->getMonitoringState()->careState == SimContext::HIV_CARE_IN_CARE
                                 || cepacPatient->getMonitoringState()->careState == SimContext::HIV_CARE_RTC));


//                                 || cepacPatient->getMonitoringState()->careState == SimContext::HIV_CARE_LTFU);
    }


    /** Check if the patient has suppressed level of VL */
    bool isSuppressd() const {
        bool flag;

        /** Call the local function that pulls HVL status from CEPAC */
        HVLStrata hvl = getHvlStratum();
        if (hvl == HVLStrata::HVL_ZERO) {
            flag = true;
        } else {
            flag = false;
        }
        return (cepacPatient && flag);
    }

    /** Recording Person Living with HIV to do that just check if the agent is HIV detected */
    bool isPLWH() const {
        return (cepacPatient && cepacPatient->getDiseaseState()->infectedHIVState != SimContext::HIV_INF_NEG);
    }

    bool isNewDiagnosed() const {
        bool flag = false;
        int current_month = cepacPatient->getGeneralState()->monthNum;
        int detected_month = cepacPatient->getMonitoringState()->monthOfDetection;
        if (current_month == detected_month + 1)  {
            flag = true;
        }
        return (cepacPatient && flag);
    }

    bool isInCareWithinThirty() const {
        bool flag = false;
//            if (this->ageDetected != Age(0, -1) && this->ageInCare != Age(0,-1)) {
//                int monthOfDetection = this->ageDetected.in_months();
//                int monthOfLinkage = this->ageInCare.in_months();
//                if (monthOfLinkage - monthOfDetection <= 1) {
//                    flag = true;
//                }
//            } else {
//                flag = false;
//            }
        int current_month = cepacPatient->getGeneralState()->monthNum;

        int linkage_month = cepacPatient->getMonitoringState()->monthOfLinkage;
        int detected_month = cepacPatient->getMonitoringState()->monthOfDetection;
//        cout << current_month << linkage_month << detected_month << endl;
        if (current_month == detected_month + 1)  {
            if (detected_month == linkage_month) {
                flag = true;
            }
        }

//        if (cepacPatient->getMonitoringState()->monthOfLinkage - cepacPatient->getMonitoringState()->monthOfDetection <
//            1) {
//            flag = true;
//        }
        if (!cepacPatient) {
            cout << "not a patient!" << endl;
        }
        return (cepacPatient && flag);
    }


//    void printFailure() {
//        if (cepacPatient && cepacPatient && cepacPatient->getARTState()->hasObservedFailure) {
//            cout << "Failed ART!" << endl;
//        }
//            if (cepacPatient->getARTState()->stop )
//            if (cepacPatient && cepacPatient->getARTState()->typeCurrStop == SimContext::ART_STOP_LTFU) {
//                cout << "Stop: LTFU" << endl;
//            } else if (cepacPatient && cepacPatient->getARTState()->typeCurrStop == SimContext::ART_STOP_MAX_MTHS){
//                cout << "Stop: SthElse" << endl;
//            }
//    }

    virtual void SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist,
                                            RandomNumberGenerator &rng) = 0;

    virtual const BetaDist GetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType) = 0;

    bool isIdentified() {
        return cepacPatient && cepacPatient->getMonitoringState()->isDetectedHIVPositive;
    }

    BetaDist CalculateChanceCondomUse(NormalDist target_dist, BetaDist curr_dist,
                                      int time, int duration);

    virtual void
    SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents) = 0;

    virtual void
    SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist) = 0;

    virtual void SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist) = 0;

    virtual void
    SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist,
                               RandomNumberGenerator &rng) = 0;

    virtual void SetChanceBecomeSexWorker(double chance) = 0;

     /** this class has a method that compares two Entities based on the desired key
      * _PSC holds the key that we search and index against.
      * _DEFAULTKEY provides a 2nd layer of ordering if people have identical _PSC
      * true is returned if key value of _p1 >= _p2. If key values are equal, then sorts based on Entity's
      * EntityID num */
    template<Entity::SelectingCriteria _PSC, class KeyValType>
    class Sorter {
    public :

        /* gets value associated with _p */
        static inline KeyValType getSortKey(Entity *_p) {
            switch (_PSC) {
                case AGE:
                    return (KeyValType) (static_cast<unsigned long>(_p->age.in_months()));
                case ID:
                    return (KeyValType) _p->id;
            }

            throw std::runtime_error("Invalid Sorting key");
        }

        /* functor associated with the < operator. Generally used for template args in in sets and maps */
        inline bool operator()(const Entity *_p1, const Entity *_p2) const {
            return getSortKey(_p1) < getSortKey(_p2);
        }
    };

    void reset_costs() {
        monthly_cepac_costs_discounted_ = 0;
        monthly_cepac_costs_undiscounted_ = 0;
        monthly_cdm_costs_discounted_ = 0;
        monthly_cdm_costs_undiscounted_ = 0;
    }

    void add_cepac_cost(double cost_undiscounted, double cost_discounted) {
        monthly_cepac_costs_undiscounted_ += cost_undiscounted;
        monthly_cepac_costs_discounted_ += cost_discounted;
    }

    double get_monthly_cepac_costs_undiscounted() const { return monthly_cepac_costs_undiscounted_; }

    double get_monthly_cepac_costs_discounted() const { return monthly_cepac_costs_discounted_; }

    void add_cdm_cost(double cost_undiscounted, double cost_discounted) {
        monthly_cdm_costs_undiscounted_ += cost_undiscounted;
        monthly_cdm_costs_discounted_ += cost_discounted;
    }

    double get_monthly_cdm_costs_undiscounted() const { return monthly_cdm_costs_undiscounted_; }

    double get_monthly_cdm_costs_discounted() const { return monthly_cdm_costs_discounted_; }

    bool UsingPrEP() {
        return (prepStatus == PrepStatus::PREP_ADHERENT);
    }

    std::list<SexualPartnership *> GetPartnerships() {
        std::list<SexualPartnership *> partnerships;

        for (auto type: enum_iterator<SexualPartnership::Type>()) {
            auto iter = partners[(int) type].begin();
            auto iterEnd = partners[(int) type].end();

            while (iter != iterEnd) {
                partnerships.push_back(*iter);
                iter++;
            }
        }
        return partnerships;
    }

private:
    BaseEnumCls::Enum getDemographicProfileVal(DemographicProfile::Demographic _demographic) const;

    /* Return the current index of which SimContext should be used to update the */
    /* health of a patient */
    static int getCEPACSimContextIndex(EventParams &_eventParams) ;

    static int getCEPACSimContextIndex(EventParams &_eventParams, int popToApply) ;

    double updateHealthCosts(EventParams &_eventParams,
                             CostsTracker *costsTracker,
                             const RunStats::OverallCosts before,
                             const RunStats::OverallCosts after);

    void updateTestingStatus(EventParams &_eventParams,
                             ArtRolloutTracker *testTracker,
                             const RunStats::HIVScreening before,
                             const RunStats::HIVScreening after);

    void traceTreatmentChange(EventParams &_eventParams, bool after);

    void traceCD4Change(EventParams &_eventParams, double before, double after);

    void traceHVLChange(EventParams &_eventParams, HVLStrata before,
                        HVLStrata after);

    void traceHIVChange(EventParams &_eventParams, HIVStatus before,
                        HIVStatus after);

    double monthly_cepac_costs_undiscounted_;
    double monthly_cepac_costs_discounted_;
    double monthly_cdm_costs_undiscounted_;
    double monthly_cdm_costs_discounted_;

    PrepParameters prepParameters;
    PrepStatus prepStatus;
    PrepEligibility prepEligibility;

};

} // namespace transm

#endif  /* ENTITY_HPP */