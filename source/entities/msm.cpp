#include <vector>

#include "msm.hpp"
#include "sexualpartnership.hpp"
#include "sexualbehavior.hpp"
#include "entitypool/entitypool.hpp"
#include "utility/utility.hpp"
#include "utility/enum_iterator.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

std::string Msm::getEntityType() const
{
    return "msm";
}

//each index of the array contains parameters for a different population
//(we only have 1 population for now so the size of the vector will default to 1

Msm::SubPopParams::SubPopParams() :
    maxPartnershipRejections(0),
    preExposureProphylaxisEfficacy_(0)

{
}

Msm::SubPopParams::~SubPopParams()
{
}

double Msm::SubPopParams::getChanceBecomeCSW() const
{
    return chanceBecomeCSW;
}

double Msm::SubPopParams::getPartnerAcqMultWithSteady(Entity::RiskLevel _risk) const
{
    return partnerAcqMultWithSteady[(std::size_t)_risk];
}

//sexual behavior params for each type as specified by SexualPartnership::Type
const SexualBehavior &Msm::SubPopParams::getSexualBehavior(SexualPartnership::Type _type) const
{
    return sexualBehaviorParams.at(_type);
}

//sexual behavior params for each type as specified by SexualPartnership::Type
SexualBehavior &Msm::SubPopParams::getSexualBehavior(SexualPartnership::Type _type)
{
    return sexualBehaviorParams.at(_type);
}


double Msm::SubPopParams::getProportionHighRisk(DemographicProfile::Employment _cswStatus) const
{
    return proportionHighRisk[(std::size_t)_cswStatus];
}
NormalDist Msm::SubPopParams::getActivityLevel() const
{
    return activityLevel;
}

double Msm::SubPopParams::getCircumProtectEff()  const
{
    return circumProtectEff;
}

double Msm::SubPopParams::getCondomProtectEff()  const
{
    return condomProtectEff;
}

Age Msm::SubPopParams::getPartneringDiscStartAge() const
{
    return partneringDiscStartAgeYrs;
}

double Msm::SubPopParams::getPartneringAcqDiscMult(Age _ageYrs) const
{
	assert(Utility::within_range(_ageYrs, Age::Zero, Age(Entity::maxYrForDeathStats, 0)));
	return partneringAcqDiscMult.at((_ageYrs - partneringDiscStartAgeYrs).years_as_index());
}

double Msm::SubPopParams::getPartneringActsDiscMult(Age _ageYrs) const
{
	assert(Utility::within_range(_ageYrs, Age::Zero, Age(Entity::maxYrForDeathStats, 0)));
	return partneringActsDiscMult.at((_ageYrs - partneringDiscStartAgeYrs).years_as_index());
}

void Msm::SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setChanceCondomUsePerEvent(risk, dist);
    chanceCondomUsePerEvent[(int)partnershipType] = rng.randBeta(dist);
}

void Msm::SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setCoitalEventsPerMonth(risk, meanEvents);
    numActsPerMonth[(int)partnershipType] = meanEvents;
}

void Msm::SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setPartnershipDuration(risk, dist);
}

void Msm::SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setAverageYearsYounger(dist);
    averageYearsYounger[(int)partnershipType] = dist;
}

void Msm::SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setAcquisitionRatePerMonth(risk, dist);
    partnerAcqRates[(int)partnershipType] = rng.randLogNormal(dist);
}

double Msm::getChanceBecomeCsw() const
{
    return populationSpecificParams.getChanceBecomeCSW();
}

void Msm::Circumcise()
{
    circumcised = true;
}

Msm::Msm(EventParams &_eventParams, Age _age, bool _circumcised, unsigned int _populationID, const Msm::SubPopParams &params)
    : Entity(_age, _populationID),
      populationSpecificParams(params),
      times_selected_(0)
{
	_age = max(min(Age(Entity::maxYrForDeathStats, 0), _age), Age::Zero);
    dmgProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Male);
    dmgProfile.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)DemographicProfile::SexualOrientation::Homosexual);
    circumcised = _circumcised;
    //Set this male's risk level assume everyone is low risk on creation. Risk is rerolled when they roll for become sex worker
    risk = Entity::RiskLevel::LOW;

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        if(!populationSpecificParams.hasSexualBehavior(partnership_type)) continue;

        auto &sexualBehaviorParams = populationSpecificParams.getSexualBehavior(partnership_type);

        auto acquisition_rate_dist = sexualBehaviorParams.getAcquisitionRatePerMonth(risk);
        auto acquisition_rate = _eventParams.randomNums.randLogNormal(acquisition_rate_dist);
        partnerAcqRates[(int)partnership_type] = acquisition_rate;

        numActsPerMonth[(int)partnership_type] = sexualBehaviorParams.getCoitalEventsPerMonth(risk);

        auto chance_condom_use_dist = sexualBehaviorParams.getChanceCondomUsePerEvent(risk);
        auto chance_condom_use = _eventParams.randomNums.randBeta(chance_condom_use_dist);
        chanceCondomUsePerEvent[(int)partnership_type] = chance_condom_use;

        averageYearsYounger[(int)partnership_type] = sexualBehaviorParams.getAverageYearsYounger();
    }

    activityLevel = _eventParams.randomNums.randNorm_NaturalNum(populationSpecificParams.getActivityLevel());

    //activity level should not ever be 0
    if(activityLevel == 0)
    {
        activityLevel = 1;
    }
}

Msm::~Msm()
{
}

/*virtual*/ void Msm::SetPreExposureProphylaxisEfficacy(double efficacy)
{
    populationSpecificParams.SetPreExposureProphylaxisEfficacy(efficacy);
}

/*virtual*/ double Msm::GetPreExposureProphylaxisEfficacy() const
{
    return populationSpecificParams.GetPreExposureProphylaxisEfficacy();
}

double Msm::getCondomUseProb(Entity *_p, SexualPartnership::Type _partnershipType)
{
    assert((_p != nullptr));
    assert(_p->isAlive());
    assert(_partnershipType < SexualPartnership::Type::ENDType);
    return chanceCondomUsePerEvent[(int)_partnershipType];
}

double Msm::getCircumProtectEff()
{
    return (circumcised ? populationSpecificParams.getCircumProtectEff() : 0);
}

double Msm::getCondomProtectEff()
{
    return populationSpecificParams.getCondomProtectEff();
}

bool Msm::isCircumcised()
{
    return circumcised;
}

//in this case, the male is infected and passed entity is an uninfected male
double Msm::getFOI(Entity *_p, const TransmissionCoefficients &transmission_coefficients, SexualPartnership::Type _partnershipType, EventParams &_eventParams)
{
    assert(Utility::valid_probability(getCondomProtectEff()));
    assert((_p != nullptr));
    assert(_p->isAlive());
    assert(_partnershipType < SexualPartnership::Type::ENDType);
    //Determine if a condom was used and record
    double chanceCondomUse = getCondomUseProb(_p, _partnershipType);
    if(_p->HasOverrideChanceCondomUse())
    {
        chanceCondomUse = _p->GetOverrideChanceCondomUse();
    }
    assert(Utility::valid_probability(chanceCondomUse));
    condomUsedLastFOICalculation = _eventParams.randomNums.chance(chanceCondomUse);
    //Determine the condom efficacy --> 0 if no condom was used
    double condomEff = 0;

    if(condomUsedLastFOICalculation)
    {
        condomEff = getCondomProtectEff();
    }

    assert(_p->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Male);

    double prepEfficacy = _p->UsingPrEP() ? _p->GetPreExposureProphylaxisEfficacy() : 0;
    double base_foi = transmission_coefficients.at(TransmissionType::male_to_male)[(std::size_t)getHVL()];
    double FOI = base_foi * (1 - condomEff) * (1 - prepEfficacy);

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
    {
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !Transmission coefficient from " << getID() << " to " <<
            _p->getID() << " is " << base_foi;
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


double Msm::getMinPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
    switch(_PSC)
    {
    case Entity::AGE:
    {
        if(getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean + 6) > 0)
        {
            return getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean + 6);
            break;
        }
        else
        {
            return 0;
            break;
        }
    }
    case ID:
    {
        return numeric_limits<double>::min();
    }
    case SEXUAL_ACTIVITY_LEVEL:
        throw std::runtime_error("not implemented");
    case ENDSelectingCriteria:
        throw std::runtime_error("Invalid Sorting key");
    }
    throw std::runtime_error("Invalid Sorting key");
}


double Msm::getMaxPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
    switch(_PSC)
    {
    case Entity::AGE:
    {
		if (getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean - 6) > 0)
        {
			return getAge().in_months() - (12 * averageYearsYounger[(int)_partnershipType].mean - 6);
            break;
        }
        else
        {
            return 0;
            break;
        }
    }
    case ID:
    {
        return numeric_limits<double>::max();
    }
    case SEXUAL_ACTIVITY_LEVEL:
    default:
        throw std::runtime_error("Invalid Sorting key");
    }
}

double Msm::rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums)
{
    double ageDifference = _randomNums.randNorm(averageYearsYounger[(int)_partnershipType]);
    return ageDifference;
}

bool Msm::possibleMatch(SexualPartnership::Type _partnershipType, Entity *_p)
{
    assert((_p != nullptr));
    assert(_p->isAlive());
    assert(_partnershipType < SexualPartnership::Type::ENDType);
    assert(false);  // check if we are using years instead of Month
    auto minAge = Age(0, static_cast<int>(getMinPartnerSelectVal(Entity::AGE, _partnershipType)));
    auto maxAge = Age(0, static_cast<int>(getMaxPartnerSelectVal(Entity::AGE, _partnershipType)));
    return _p->getAge() >= minAge && _p->getAge() <= maxAge;
}

int Msm::rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
    if(!populationSpecificParams.hasSexualBehavior(_partnershipType)) return 0;

    assert(_partnershipType < SexualPartnership::Type::ENDType);

    //person can only have 1 steady partner at a time so return 0 if person is already in Steady
    if((_partnershipType == SexualPartnership::Type::SteadyMsm) && !isSingle())
    {
        return 0;
    }

    //rate of acquiring partner
    double partnerRate;
    partnerRate = partnerAcqRates[(int)_partnershipType];

    //if this person has a steady partner then adjust acquisition rate
    if(!isSingle())
    {
        //if we're thinking of getting another partner, then lower chances if we have a steady partner
        partnerRate *= populationSpecificParams.getPartnerAcqMultWithSteady(getRiskLevel());
    }

    //if person is over the age of partnering discounting, then discount acquisition rate
    auto ageYrs = getAge();

    if(ageYrs >= populationSpecificParams.getPartneringDiscStartAge())
    {
        partnerRate *= populationSpecificParams.getPartneringAcqDiscMult(ageYrs);
    }

    /** To get the number of partners to draw this month, draw from a Poisson distribution */
    int numPartners = _randomNums.randPoisson(partnerRate);
    //if we are rolling for STEADY, make sure we have max of 1
    return (_partnershipType != SexualPartnership::Type::SteadyMsm) ? numPartners : std::min(1, numPartners);
}

int Msm::rollNumEventsPerPartner(Entity *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
    assert((_p != nullptr));
    assert(_p->isAlive());

    double meanCoitalEvents = numActsPerMonth[(int)_partnershipType];

    //if person is over the age of partnering discounting, then discount #acts
    auto ageYrs = getAge();

    if(ageYrs >= populationSpecificParams.getPartneringDiscStartAge())
    {
        meanCoitalEvents *= populationSpecificParams.getPartneringActsDiscMult(ageYrs);
    }

    //Poisson distributions range from 0 to infinity: we want to avoid 0 acts per month
    //If meanCoitalEvents is less than 1 (happens after discounting), set to one (force a minimum)
    if(meanCoitalEvents < 1)
    {
        meanCoitalEvents = 1;
    }

    int numActs = _randomNums.randPoisson(meanCoitalEvents - 1) + 1;
    return numActs;
}


int Msm::rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Entity *_p)
{
    assert((_p != nullptr));
    assert(_p->isAlive());
    assert(_partnershipType < SexualPartnership::Type::ENDType);
    ShiftedLogNormalDist duration = populationSpecificParams.getSexualBehavior(_partnershipType).getPartnershipDurationMth(risk);
    return (int)(_randomNums.randShiftedLogNormal(duration) + .5);
}

void Msm::rerollRiskGroup(EventParams &_eventParams)
{
    DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) getDemographicProfileVal(DemographicProfile::Demographic::Employment);
    double chanceHighRisk = populationSpecificParams.getProportionHighRisk(cswStatus);
    Entity::RiskLevel oldRisk = risk;

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
            auto acquisition_rate = sexualBehaviorParams.getAcquisitionRatePerMonth(risk);
            partnerAcqRates[(int)partnership_type] =
                _eventParams.randomNums.randLogNormal(acquisition_rate);
            numActsPerMonth[(int)partnership_type] =
                sexualBehaviorParams.getCoitalEventsPerMonth(risk);
            auto chance_condom_use = sexualBehaviorParams.getChanceCondomUsePerEvent(risk);
            chanceCondomUsePerEvent[(int)partnership_type] =
                _eventParams.randomNums.randBeta(chance_condom_use);
        }
    }

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
    {
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Msm " << getID() << " rerolls as ";

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
