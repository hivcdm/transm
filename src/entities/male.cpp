#include <vector>

#include "male.hpp"
#include "female.hpp"
#include "sexualpartnership.hpp"
#include "sexualbehavior.hpp"
#include "entitypool/entitypool.hpp"
#include "utility/utility.hpp"
#include "utility/enum_iterator.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

const string Male::getEntityType() const {
	const BaseEnumCls *currCategoryCls = transm::DemographicProfile::getEnumCls(DemographicProfile::Demographic::SexualOrientation);
	BaseEnumCls::Enum dmgProfileEnum = dmgProfile.get(DemographicProfile::Demographic::SexualOrientation);
	return *currCategoryCls->toString(dmgProfileEnum);
}

Male::SubPopParams::SubPopParams() :
    maxPartnershipRejections(0),
    preExposureProphylaxisEfficacy_(0)
{
}

Male::SubPopParams::~SubPopParams() = default;

double Male::SubPopParams::getChanceBecomeCSW() const
{
	return chanceBecomeCSW;
}

double Male::SubPopParams::getPartnerAcqMultWithSteady(RiskLevel _risk) const
{
    return partnerAcqMultWithSteady[(std::size_t)_risk];
}

/* sexual behavior params for each type as specified by SexualPartnership::Type */
const SexualBehavior &Male::SubPopParams::getSexualBehavior(SexualPartnership::Type _type) const
{
	return sexualBehaviorParams.at(_type);
}

/* sexual behavior params for each type as specified by SexualPartnership::Type */
SexualBehavior &Male::SubPopParams::getSexualBehavior(SexualPartnership::Type _type)
{
	return sexualBehaviorParams.at(_type);
}

double Male::SubPopParams::getProportionHighRisk(DemographicProfile::Employment _cswStatus) const
{
    return proportionHighRisk[(std::size_t)_cswStatus];
}

double Male::SubPopParams::getCircumProtectEff()  const
{
	return circumProtectEff;
}

double Male::SubPopParams::getCondomProtectEff()  const
{
	return condomProtectEff;
}

Age Male::SubPopParams::getPartneringDiscStartAgeYrs() const
{
	return partneringDiscStartAgeYrs;
}

double Male::SubPopParams::getPartneringAcqDiscMult(Age _ageYrs) const
{
	assert(Utility::within_range(_ageYrs, Age::Zero, Age(Entity::maxYrForDeathStats, 0)));
	return partneringAcqDiscMult.at((_ageYrs - partneringDiscStartAgeYrs).years_as_index());
}

double Male::SubPopParams::getPartneringActsDiscMult(Age _ageYrs) const
{
	assert(Utility::within_range(_ageYrs, Age::Zero, Age(Entity::maxYrForDeathStats, 0)));
	return partneringActsDiscMult.at((_ageYrs - partneringDiscStartAgeYrs).years_as_index());
}

void Male::SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng)
{
    auto &behavior = populationSpecificParams.getSexualBehavior(partnershipType);
    if(risk == getRiskLevel())
    {
        auto current_dist = behavior.getChanceCondomUsePerEvent(risk);
        if(current_dist.alpha != dist.beta || current_dist.alpha != dist.alpha)
        {
            chanceCondomUsePerEvent[(int)partnershipType] = rng.randBeta(dist);
        }
    }
    behavior.setChanceCondomUsePerEvent(risk, dist);
}

void Male::SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents)
{
    auto &behavior = populationSpecificParams.getSexualBehavior(partnershipType);
    if(risk == getRiskLevel())
    {
        auto current_dist = behavior.getCoitalEventsPerMonth(risk);
        if(current_dist != meanEvents)
        {
            numActsPerMonth[(int)partnershipType] = meanEvents;
        }
    }
    behavior.setCoitalEventsPerMonth(risk, meanEvents);
}

void Male::SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist)
{
    auto &behavior = populationSpecificParams.getSexualBehavior(partnershipType);
	behavior.setPartnershipDuration(risk, dist);
}

void Male::SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist)
{
    auto &behavior = populationSpecificParams.getSexualBehavior(partnershipType);
	behavior.setAverageYearsYounger(dist);
    averageYearsYounger[(int)partnershipType] = dist;
}

void Male::SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng)
{
    auto &behavior = populationSpecificParams.getSexualBehavior(partnershipType);
    if(risk == getRiskLevel())
    {
        auto current_dist = behavior.getAcquisitionRatePerMonth(risk);
        if(current_dist.mu != dist.mu || current_dist.sigma != dist.sigma)
        {
            partnerAcqRates[(int)partnershipType] = rng.randLogNormal(dist);
        }
    }

	behavior.setAcquisitionRatePerMonth(risk, dist);
}

DemographicProfile::ProfileID Male::ChoosePartnerDemographic(RandomNumberGenerator &_randomNums,
    SexualPartnership::Type _partnershipType)
{
    DemographicProfile::ProfileID entityProfileID;
    DemographicProfile selector;

    /* only sexually active partners allowed */
    selector.set(DemographicProfile::Demographic::SexualActivityStatus,
        (std::size_t)DemographicProfile::SexualActivityStatus::Active);

    /* choose CSW status */
    if (_partnershipType == SexualPartnership::Type::Csw)
    {
        selector.set(DemographicProfile::Demographic::Employment,
            (std::size_t)DemographicProfile::Employment::Csw);
        selector.set(DemographicProfile::Demographic::RelationshipStatus,
            (std::size_t)DemographicProfile::RelationshipStatus::Single);
    }
    else
    {
        selector.set(DemographicProfile::Demographic::Employment,
            (std::size_t)DemographicProfile::Employment::NonCsw);

        /* choose steady partner -- base on percentWithSteady */
        SexualBehavior behavior = populationSpecificParams.getSexualBehavior(_partnershipType);
        if (_randomNums.chance(behavior.getChanceChooseWithSteady()))
        {
            selector.set(DemographicProfile::Demographic::RelationshipStatus,
                (std::size_t)DemographicProfile::RelationshipStatus::NonSingle);
        }
        else
        {
            selector.set(DemographicProfile::Demographic::RelationshipStatus,
                (std::size_t)DemographicProfile::RelationshipStatus::Single);
        }
    }

    /* choose partner gender and orientation -- based on orientation and, if MSMW, percentMsmwChooseMale */
    if (getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
        DemographicProfile::SexualOrientation::Msw)
    {
        selector.set(DemographicProfile::Demographic::Gender,
            (std::size_t)DemographicProfile::Gender::Female);
        selector.set(DemographicProfile::Demographic::SexualOrientation,
            (std::size_t)DemographicProfile::SexualOrientation::Msw);
    }
    else if (getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
        DemographicProfile::SexualOrientation::Msm)
    {
        selector.set(DemographicProfile::Demographic::Gender,
            (std::size_t)DemographicProfile::Gender::Male);

        if (_randomNums.chance(populationSpecificParams.getChanceMsmChooseMsmw()))
        {
            selector.set(DemographicProfile::Demographic::SexualOrientation,
                (std::size_t)DemographicProfile::SexualOrientation::Msmw);
        }
        else
        {
        selector.set(DemographicProfile::Demographic::SexualOrientation,
            (std::size_t)DemographicProfile::SexualOrientation::Msm);
        }
    }
    else if (getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
        DemographicProfile::SexualOrientation::Msmw)
    {
        if (_randomNums.chance(populationSpecificParams.getChanceMsmwChooseMale()))
        {
            selector.set(DemographicProfile::Demographic::Gender,
                (std::size_t)DemographicProfile::Gender::Male);

            if (_randomNums.chance(populationSpecificParams.getChanceMsmChooseMsmw()))
            {
                selector.set(DemographicProfile::Demographic::SexualOrientation,
                    (std::size_t)DemographicProfile::SexualOrientation::Msmw);
            }
            else
            {
                selector.set(DemographicProfile::Demographic::SexualOrientation,
                    (std::size_t)DemographicProfile::SexualOrientation::Msm);
            }

        }
        else
        {
            selector.set(DemographicProfile::Demographic::Gender,
                (std::size_t)DemographicProfile::Gender::Female);
            selector.set(DemographicProfile::Demographic::SexualOrientation,
                (std::size_t)DemographicProfile::SexualOrientation::Msw);
        }
    }
    else
    {
        throw std::runtime_error("Unknown sexual orientation");
    }

    auto entityRace = getDemographicProfileVal<DemographicProfile::Race>();
    auto entityEthnicity = getDemographicProfileVal<DemographicProfile::Ethnicity>();

    DemographicProfile::Race partnerRace;
    DemographicProfile::Ethnicity partnerEthnicity;

    if (_randomNums.chance(populationSpecificParams.getRaceEthnicAssortativeness(entityRace, entityEthnicity)))
    {
        /* if non-assortative (homogeneous), choose entities own race */
        partnerRace = entityRace;
        partnerEthnicity = entityEthnicity;
    }
    else
    {
        /* choose a race randomly from the allowed list of races */
        std::vector<DemographicProfile::Race> races = populationSpecificParams.
            allowedRaceEthnicityMap.GetRaceKeysFromMap();
        partnerRace = races[_randomNums.chooseIndex(races.size())];

        /* choose an ethnicity randomly from the allowed list of ethnicities for the race chosen */
        std::vector<DemographicProfile::Ethnicity> ethnicities =
          populationSpecificParams.allowedRaceEthnicityMap.GetEthnicityForRace(partnerRace);
        partnerEthnicity = ethnicities[_randomNums.chooseIndex(ethnicities.size())];
    }
    selector.set(DemographicProfile::Demographic::Race, (std::size_t)partnerRace);
    selector.set(DemographicProfile::Demographic::Ethnicity, (std::size_t)partnerEthnicity);

    std::vector<DemographicProfile::ProfileID> validBucketIDs;
    selector.selectProfileIDs(validBucketIDs, nullptr);

    assert(validBucketIDs.size() == 1);
    return validBucketIDs.at(0);
}

double Male::getChanceBecomeCsw() const
{
	return populationSpecificParams.getChanceBecomeCSW();
}

void Male::Circumcise()
{
	circumcised = true;
}

Male::Male(EventParams &_eventParams, Age _age, bool _circumcised,
    const DemographicProfile &profile, unsigned int _populationID,
    const Male::SubPopParams &params, const PrepParameters &prepParams) :
    Entity(_age, _populationID, prepParams),
    populationSpecificParams(params)
{
	/* Only set the gender and sexual orientation */
	/* The other demographic profiles values get set in the Entity constructor -- don't overwrite the entire profile */
    assert(profile.get(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male);
    dmgProfile = profile;

	circumcised = _circumcised;

    /* Set this male's risk level assume everyone is low risk on creation. */
    /* Risk is rerolled when they reach the age of sexual maturity */
    risk = RiskLevel::LOW;

	for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
	{
        if(!populationSpecificParams.hasSexualBehavior(partnership_type)) continue;

		auto &sexualBehaviorParams = populationSpecificParams.getSexualBehavior(partnership_type);

		auto acquisition_rate_dist = sexualBehaviorParams.getAcquisitionRatePerMonth(risk);
		auto acquisition_rate = _eventParams.randomNums.randLogNormal(acquisition_rate_dist);
        /* correction of MSM partnerships */
        if (getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
                                       DemographicProfile::SexualOrientation::Msm) {
            acquisition_rate *= 0.5*1.61;
        }
        partnerAcqRates[(int)partnership_type] = acquisition_rate;
		numActsPerMonth[(int)partnership_type] = sexualBehaviorParams.getCoitalEventsPerMonth(risk);

		auto chance_condom_use_dist = sexualBehaviorParams.getChanceCondomUsePerEvent(risk);
		auto chance_condom_use = _eventParams.randomNums.randBeta(chance_condom_use_dist);
		chanceCondomUsePerEvent[(int)partnership_type] = chance_condom_use;

		averageYearsYounger[(int)partnership_type] = sexualBehaviorParams.getAverageYearsYounger();
	}
}

Male::~Male() = default;

std::size_t Male::GetSexualOrientation()
{
    return (std::size_t)getDemographicProfileVal<DemographicProfile::SexualOrientation>();
}

double Male::getCondomUseProb(Entity *_p, SexualPartnership::Type _partnershipType)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::Last);
	return chanceCondomUsePerEvent[(int)_partnershipType];
}

double Male::getCircumProtectEff()
{
	return (circumcised ? populationSpecificParams.getCircumProtectEff() : 0);
}

double Male::getCondomProtectEff()
{
	return populationSpecificParams.getCondomProtectEff();
}

/* in this case, the male is infected and female is uninfected */
double Male::getFOI(Entity *_p, const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &transmission_coefficients, SexualPartnership::Type _partnershipType, EventParams &_eventParams)
{
	/* note: in the case of male->female transmission, circumcision makes no difference */
	/* transmission coeff				1-	(condoms are used and succeed) */
	assert(Utility::valid_probability(getCondomProtectEff()));
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::Last);

	/* Determine if a condom was used and record */
    double chanceCondomUse = getCondomUseProb(_p, _partnershipType);
    if(_p->HasOverrideChanceCondomUse())
    {
        chanceCondomUse = _p->GetOverrideChanceCondomUse();
    }
    assert(Utility::valid_probability(chanceCondomUse));
	condomUsedLastFOICalculation = _eventParams.randomNums.chance(chanceCondomUse);

	/* Determine the condom efficacy --> 0 if no condom was used */
	double condomEff = 0;

	if(condomUsedLastFOICalculation)
	{
		condomEff = getCondomProtectEff();
	}

    double microbicideEfficacy = 0;
    double baseFoi = 0;
    if (_p->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Female)
    {
        baseFoi = transmission_coefficients.at(TransmissionType::male_to_female)[(std::size_t)getHVL()];
        microbicideEfficacy =
          dynamic_cast<Female *>(_p)->RollForVaginalMicrobicideUse(_eventParams.randomNums)
          ? dynamic_cast<Female *>(_p)->GetVaginalMicrobicideEfficacy() : 0;
    }
    else
    {
        baseFoi = transmission_coefficients.at(TransmissionType::male_to_male)[(std::size_t)getHVL()];
    }

    double prepEfficacy = _p->GetPreExposureProphylaxisEfficacy();
    // cout << prepEfficacy << endl;

    // corrections for efficacy based on prep adherence level
    PrepAherenceLevel prepadherencelevel = _p->getAdhereceStatus();
    if (prepadherencelevel == PrepAherenceLevel::PREP_SUBSTANTIALLY_ADHERENT) {
        prepEfficacy *= 0.80;
    } else if (prepadherencelevel == PrepAherenceLevel::PREP_PARTIALLY_ADHERENT) {
        prepEfficacy *= 0.10;
    } else if (prepadherencelevel == PrepAherenceLevel::PREP_INADHERENT) {
        prepEfficacy *= 0.05;
    } else if (prepadherencelevel == PrepAherenceLevel::PREP_ADHERENT) {
        prepEfficacy *= 0.99;
    } else if (prepadherencelevel == PrepAherenceLevel::PREP_OFF_PREP) {
        prepEfficacy *= 0.0;
    } else {
        cout << "Invalid adherence level" << endl;
    }
 
    double FOI = baseFoi * (1 - condomEff) * (1 - microbicideEfficacy) * (1 - prepEfficacy);

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !Transmission coefficient from " << getID() << " to " <<
		        _p->getID() << " is " << baseFoi;
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ";" << std::endl << " !A condom was ";

		if(!condomUsedLastFOICalculation)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "NOT ";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "used (efficacy " << getCondomProtectEff();
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ");" << std::endl << " !Total FOI = " << FOI << std::endl;
	}

	return FOI;
}


double Male::getMinPartnerSelectVal(Entity::SelectingCriteria PSC, SexualPartnership::Type _partnershipType) const
{
	switch(PSC)	{
	    case Entity::AGE: {
            if(getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean + 6) > 0) {
    			return getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean + 6);
	    		break;
		    } else 	{
			    return 0;
			    break;
		    }
	    } case ID: 	{
    		return numeric_limits<double>::min();
    	} case ENDSelectingCriteria:
    	    throw std::runtime_error("Invalid Sorting key");
    }
	throw std::runtime_error("Invalid Sorting key");
}


double Male::getMaxPartnerSelectVal(Entity::SelectingCriteria PSC, SexualPartnership::Type _partnershipType) const
{
	switch(PSC) {
	    case Entity::AGE: {
		    if (getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean - 6) > 0) 	{
			    return getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean - 6);
			    break;
		    } else {
			    return 0;
			    break;
		    }
	    }
	    case ID: {
		    return numeric_limits<double>::max();
	    } default:
	        throw std::runtime_error("Invalid Sorting key");
	}
}

double Male::rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums)
{
	double ageDifference = _randomNums.randNorm(averageYearsYounger[(int)_partnershipType]);
	return ageDifference;
}

bool Male::possibleMatch(SexualPartnership::Type _partnershipType, Entity *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::Last);
	assert(false);  /* check if we are using years instead of Month */
	auto minAge = Age(0, static_cast<int>(getMinPartnerSelectVal(Entity::AGE, _partnershipType)));
	auto maxAge = Age(0, static_cast<int>(getMaxPartnerSelectVal(Entity::AGE, _partnershipType)));
    return _p->getAge() >= minAge && _p->getAge() <= maxAge;
}

int Male::rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
    if(!populationSpecificParams.hasSexualBehavior(_partnershipType))
        return 0;

	assert(_partnershipType < SexualPartnership::Type::Last);

	/* person can only have 1 steady partner at a time so return 0 if person is already in Steady */
	if((_partnershipType == SexualPartnership::Type::Steady) && (!partners[(int)_partnershipType].empty()))
        return 0;

	/* rate of acquiring partner */
	double partnerRate;

    partnerRate = partnerAcqRates[(int)_partnershipType];

	/* if this person has a steady partner then adjust acquisition rate */
	if(!partners[(int)SexualPartnership::Type::Steady].empty())
	{
		/* if we're thinking of getting another partner, then lower chances if we have a steady partner */
		partnerRate *= populationSpecificParams.getPartnerAcqMultWithSteady(getRiskLevel());
	}

	/* if person is over the age of partnering discounting, then discount acquisition rate */
    auto ageYrs = getAge();

	if(ageYrs >= populationSpecificParams.getPartneringDiscStartAgeYrs())
	{
		partnerRate *= populationSpecificParams.getPartneringAcqDiscMult(ageYrs);
	}

	/** To get the number of partners to draw this month, draw from a Poisson distribution */
	int numPartners = _randomNums.randPoisson(partnerRate);

	/* if we are rolling for STEADY, make sure we have max of 1 */
	return (_partnershipType != SexualPartnership::Type::Steady) ? numPartners : std::min(1, numPartners);
}

int Male::rollNumEventsPerPartner(Entity *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
	assert((_p != nullptr));
	assert(_p->isAlive());

	double meanCoitalEvents = numActsPerMonth[(int)_partnershipType];

    if (isBlack() && (_partnershipType == SexualPartnership::Type::Steady ||
                      _partnershipType == SexualPartnership::Type::Regular)) {
        meanCoitalEvents *= 5.0;
    }

    if (isWhite() && (_partnershipType == SexualPartnership::Type::Steady ||
                      _partnershipType == SexualPartnership::Type::Regular)) {
        meanCoitalEvents *= 0.7;
    }

	/* if person is over the age of partnering discounting, then discount #acts */
    auto ageYrs = getAge();

	if(ageYrs >= populationSpecificParams.getPartneringDiscStartAgeYrs())
	{
		meanCoitalEvents *= populationSpecificParams.getPartneringActsDiscMult(ageYrs);
	}

	/* Poisson distributions range from 0 to infinity: we want to avoid 0 acts per month */
	/* If meanCoitalEvents is less than 1 (happens after discounting), set to one (force a minimum) */
	if(meanCoitalEvents < 1)
	{
		meanCoitalEvents = 1;
	}

	int numActs = _randomNums.randPoisson(meanCoitalEvents - 1) + 1;

    /* double acts per month for Black population in Steady and Regular partnerships */
    // if (isBlack() && (_partnershipType == SexualPartnership::Type::Steady ||
    //                   _partnershipType == SexualPartnership::Type::Regular)) {
    //     numActs *= 2.0;
    // }

	return numActs;
}


int Male::rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Entity *_p)
{
	assert((_p != nullptr));
	assert(_p->isAlive());
	assert(_partnershipType < SexualPartnership::Type::Last);
	ShiftedLogNormalDist duration = populationSpecificParams.
	    getSexualBehavior(_partnershipType).getPartnershipDurationMth(risk);
	return Utility::round<int>(_randomNums.randShiftedLogNormal(duration));
}

long Male::getPartnershipsToEnd(Time _currTime, EventParams & _eventParams, SexualPartnership::Type _partnershipType,
                                  std::list<SexualPartnership *> &_partnershipsToEnd, bool _fromDeath) {
    assert(_partnershipType < SexualPartnership::Type::Last);
    assert((_currTime >= Time::Zero) || _fromDeath);

    if (partners[(int) _partnershipType].empty()) {
        return 0;
    }

    //iterate through all current partnerships that had any duration to them.
    //The iterator points to class SexualPartnership
    auto iter = partners[(int) _partnershipType].begin();
    auto iterEnd = partners[(int) _partnershipType].end();
    long numEnded = 0;
    RandomNumberGenerator randomNum;


    /* Manually assigned whether breakup rates are being used instead of partnership durations */
    /* TODO: Reflect this flag in the input card. S. Seifi */
    bool usingBreakupRates = true;

    //go through all partnerships
    while (iter != iterEnd) {
        //if it's time for that partnership to end, then put that partnership is the list for deletion
        if (!usingBreakupRates) {
            if ((*iter)->checkTimeForSplit(_currTime) || _fromDeath) {
                _partnershipsToEnd.push_back(*iter);
                numEnded++;
            }

        } else {

            if (_fromDeath) {
                _partnershipsToEnd.push_back(*iter);
                numEnded++;
            } else {

                /* either MSM or MSMW */
                bool isMsm = getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
                             DemographicProfile::SexualOrientation::Msm;
                bool isMsmw = getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
                              DemographicProfile::SexualOrientation::Msmw;

                /* Homosexual or bisexual in a steady partnership */
                /* TODO: Breakup rates are coded here. Has to be added to the input card. S. Seifi*/
                if (_partnershipType == SexualPartnership::Type::Steady) {
                    if (isMsm || isMsmw) {
                        /* let's see if break up happens: breakup rate calculated by Noe */
                        /* Needed to be halved to eliminate double counting*/
                        if (_eventParams.randomNums.chance(0.5*populationSpecificParams.getBreakupRateMSM(_partnershipType))) {
                            _partnershipsToEnd.push_back(*iter);
                            numEnded++;
                        }
                    } else {
                        /* let's see if break up happens: breakup rate calculated by Noe */
                        if (_eventParams.randomNums.chance(populationSpecificParams.getBreakupRateMSW(_partnershipType))) {
                            _partnershipsToEnd.push_back(*iter);
                            numEnded++;
                        }
                    }
                } else if (_partnershipType == SexualPartnership::Type::Regular) {
                    if (isMsm || isMsmw) {
                        /* let's see if break up happens: breakup rate calculated by Noe */
                        /* Needed to be halved to eliminate double counting*/
                        if (_eventParams.randomNums.chance(0.5*populationSpecificParams.getBreakupRateMSM(_partnershipType))) {
                            _partnershipsToEnd.push_back(*iter);
                            numEnded++;
                        }
                    } else {
                        /* let's see if break up happens: breakup rate calculated by Noe */
                        if (_eventParams.randomNums.chance(populationSpecificParams.getBreakupRateMSW(_partnershipType))) {
                            _partnershipsToEnd.push_back(*iter);
                            numEnded++;
                        }
                    }
                // let's also include CSW partnerships
                } else if (_partnershipType == SexualPartnership::Type::Casual ||
                           _partnershipType == SexualPartnership::Type::Csw) {
                    if (isMsm || isMsmw) {
                        /* In Casual partnerships everyone breaks up */
                        /* Needed to be halved to eliminate double counting*/
                        if (_eventParams.randomNums.chance(0.5*1.0)) {
                            _partnershipsToEnd.push_back(*iter);
                            numEnded++;
                        }
                    } else {
                        /* In Casual partnerships everyone breaks up*/
                        if (_eventParams.randomNums.chance(1.0)) {
                            _partnershipsToEnd.push_back(*iter);
                            numEnded++;
                        }
                    }
                }
            }

        }
        iter++;
    }

    return numEnded;
}

void Male::rerollRiskGroup(EventParams &_eventParams)
{
	auto cswStatus = getDemographicProfileVal<DemographicProfile::Employment>();
	double chanceHighRisk = populationSpecificParams.getProportionHighRisk(cswStatus);
	RiskLevel oldRisk = risk;

	if(_eventParams.randomNums.chance(chanceHighRisk))
	{
		risk = RiskLevel::HIGH;
	}
	else
	{
		risk = RiskLevel::LOW;
	}

	if(oldRisk != risk)
	{
		for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
		{
            if(!populationSpecificParams.hasSexualBehavior(partnership_type)) continue;

			const SexualBehavior &sexualBehaviorParams = 
				populationSpecificParams.getSexualBehavior(partnership_type);
			auto acquisition_rate_dist = sexualBehaviorParams.getAcquisitionRatePerMonth(risk);
            auto acquisition_rate = _eventParams.randomNums.randLogNormal(acquisition_rate_dist);
            /* correction of MSM partnerships */
            if (getDemographicProfileVal<DemographicProfile::SexualOrientation>() ==
                DemographicProfile::SexualOrientation::Msm) {
                acquisition_rate *= 0.5*1.61;   // 0.5 is accounts for correcting the acquisition rate for male choosing male and 1.61 is the multiplier!
            }
			partnerAcqRates[(int)partnership_type] = acquisition_rate;

			numActsPerMonth[(int)partnership_type] =
				sexualBehaviorParams.getCoitalEventsPerMonth(risk);
			auto chance_condom_use = sexualBehaviorParams.getChanceCondomUsePerEvent(risk);
			chanceCondomUsePerEvent[(int)partnership_type] =
				_eventParams.randomNums.randBeta(chance_condom_use);
		}
	}

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Male " << getID() << " rerolls as ";

		if(risk == RiskLevel::HIGH)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "High";
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "Low";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " risk" << std::endl;
	}
}

} // namespace transm
