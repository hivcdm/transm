#include <vector>

#include "msmw.hpp"
#include "sexualpartnership.hpp"
#include "sexualbehavior.hpp"
#include "entitypool/entitypool.hpp"
#include "utility/utility.hpp"
#include "utility/enum_iterator.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

//each index of the array contains parameters for a different population
//(we only have 1 population for now so the size of the vector will default to 1

std::string Msmw::getEntityType() const 
{ 
    return "msmw";
}

Msmw::SubPopParams::SubPopParams() : 
  cswEndAge(0),
  maxPartnershipRejections(0)  
{
}

Msmw::SubPopParams::~SubPopParams()
{
}

double Msmw::SubPopParams::getChanceBecomeCSW() const
{
    return chanceBecomeCSW;
}

double Msmw::SubPopParams::getPartnerAcqMultWithSteady(Entity::RiskLevel _risk) const
{
    return partnerAcqMultWithSteady[(std::size_t)_risk];
}

//sexual behavior params for each type as specified by SexualPartnership::Type
const SexualBehavior &Msmw::SubPopParams::getSexualBehavior(SexualPartnership::Type _type) const
{
    return sexualBehaviorParams.at(_type);
}

//sexual behavior params for each type as specified by SexualPartnership::Type
SexualBehavior &Msmw::SubPopParams::getSexualBehavior(SexualPartnership::Type _type)
{
    return sexualBehaviorParams.at(_type);
}


double Msmw::SubPopParams::getProportionHighRisk(DemographicProfile::Employment _cswStatus) const
{
    return proportionHighRisk[(std::size_t)_cswStatus];
}
NormalDist Msmw::SubPopParams::getActivityLevel() const
{
    return activityLevel;
}

double Msmw::SubPopParams::getCircumProtectEff()  const
{
    return circumProtectEff;
}

double Msmw::SubPopParams::getCondomProtectEff()  const
{
    return condomProtectEff;
}

int Msmw::SubPopParams::getPartneringDiscStartAgeYrs() const
{
    return partneringDiscStartAgeYrs;
}

double Msmw::SubPopParams::getPartneringAcqDiscMult(int _ageYrs) const
{
    assert(Utility::within_range(_ageYrs, 0, Entity::maxYrForDeathStats));
    return partneringAcqDiscMult.at(_ageYrs - partneringDiscStartAgeYrs);
}

double Msmw::SubPopParams::getPartneringActsDiscMult(int _ageYrs) const
{
    assert(Utility::within_range(_ageYrs, 0, Entity::maxYrForDeathStats));
    return partneringActsDiscMult.at(_ageYrs - partneringDiscStartAgeYrs);
}

void Msmw::SetChanceCondomUsePerEvent(RiskLevel risk, SexualPartnership::Type partnershipType, BetaDist dist, RandomNumberGenerator &rng)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setChanceCondomUsePerEvent(risk, dist);
    chanceCondomUsePerEvent[(int)partnershipType] = rng.randBeta(dist);
}

void Msmw::SetCoitalEventsPerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, double meanEvents)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setCoitalEventsPerMonth(risk, meanEvents);
    numActsPerMonth[(int)partnershipType] = meanEvents;
}

void Msmw::SetPartnershipDuration(RiskLevel risk, SexualPartnership::Type partnershipType, ShiftedLogNormalDist dist)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setPartnershipDuration(risk, dist);
}

void Msmw::SetAverageYearsYounger(SexualPartnership::Type partnershipType, NormalDist dist)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setAverageYearsYounger(dist);
    averageYearsYounger[(int)partnershipType] = dist;
}

void Msmw::SetAcquisitionRatePerMonth(RiskLevel risk, SexualPartnership::Type partnershipType, LogNormalDist dist, RandomNumberGenerator &rng)
{
    populationSpecificParams.getSexualBehavior(partnershipType).setAcquisitionRatePerMonth(risk, dist);
    partnerAcqRates[(int)partnershipType] = rng.randLogNormal(dist);
}

double Msmw::getChanceBecomeCsw() const
{
    return populationSpecificParams.getChanceBecomeCSW();
}

void Msmw::Circumcise()
{
    circumcised = true;
}

Msmw::Msmw(EventParams &_eventParams, int _age, bool _circumcised, unsigned int _populationID, const Msmw::SubPopParams &params)
    : Entity(_age, _populationID),
    populationSpecificParams(params)
{
    //If age is out of range, set it at the closest boundary.
    if(!Utility::within_range<int>(_age, 0, Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Entity::maxYrForDeathStats)))
    {
        if(_age < 0)
        {
            _age = 0;
        }
        else
        {
            _age = Utility::convert_time(TimeGranularity::Year, TimeGranularity::Month, Entity::maxYrForDeathStats);
        }
    }

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

Msmw::~Msmw()
{
}

double Msmw::getCondomUseProb(Entity *_p, SexualPartnership::Type _partnershipType)
{
    assert((_p != nullptr));
    assert(_p->isAlive());
    assert(_partnershipType < SexualPartnership::Type::ENDType);
    return chanceCondomUsePerEvent[(int)_partnershipType];
}

double Msmw::getCircumProtectEff()
{
    return (circumcised ? populationSpecificParams.getCircumProtectEff() : 0);
}

double Msmw::getCondomProtectEff()
{
    return populationSpecificParams.getCondomProtectEff();
}

bool Msmw::isCircumcised()
{
    return circumcised;
}

//in this case, the male is infected and female is uninfected
double Msmw::getFOI(Entity *_p, const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &transmission_coefficients, SexualPartnership::Type _partnershipType, EventParams &_eventParams)
{
    //note: in the case of male->female transmission, circumcision makes no difference
    //transmission coeff				1-	(condoms are used and succeed)
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

    double base_foi = 0;

    if(_p->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Female)
    {
        base_foi = transmission_coefficients.at(TransmissionType::male_to_female)[(std::size_t)getHVL()];
    }
    else
    {
        base_foi = transmission_coefficients.at(TransmissionType::male_to_male)[(std::size_t)getHVL()];
    }

    double FOI = base_foi * (1 - condomEff);

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


double Msmw::getMinPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
    switch(_PSC)
    {
    case Entity::AGE:
    {
        if(getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean + 6) > 0)
        {
            return getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean + 6);
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


double Msmw::getMaxPartnerSelectVal(Entity::SelectingCriteria _PSC, SexualPartnership::Type _partnershipType) const
{
    switch(_PSC)
    {
    case Entity::AGE:
    {
        if(getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean - 6) > 0)
        {
            return getAge(TimeGranularity::Month) - (12 * averageYearsYounger[(int)_partnershipType].mean - 6);
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

double Msmw::rollForAgeDifference(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums)
{
    double ageDifference = _randomNums.randNorm(averageYearsYounger[(int)_partnershipType]);
    return ageDifference;
}

bool Msmw::possibleMatch(SexualPartnership::Type _partnershipType, Entity *_p)
{
    assert((_p != nullptr));
    assert(_p->isAlive());
    assert(_partnershipType < SexualPartnership::Type::ENDType);
    assert(false);  // check if we are using years instead of Month
    int minAge = static_cast<int>(getMinPartnerSelectVal(Entity::AGE, _partnershipType));
    int maxAge = static_cast<int>(getMaxPartnerSelectVal(Entity::AGE, _partnershipType));
    return Utility::within_range(_p->getAge(TimeGranularity::Month), minAge, maxAge);
}

int Msmw::rollForNumPartners(RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
    if(!populationSpecificParams.hasSexualBehavior(_partnershipType)) return 0;

    assert(_partnershipType < SexualPartnership::Type::ENDType);

    //person can only have 1 steady partner at a time so return 0 if person is already in Steady
    if((_partnershipType == SexualPartnership::Type::Steady || _partnershipType == SexualPartnership::Type::SteadyMsm) && (!partners[(int)_partnershipType].empty()))
    {
        return 0;
    }

    //rate of acquiring partner
    double partnerRate;
    partnerRate = partnerAcqRates[(int)_partnershipType];

    //if this person has a steady partner then adjust acquisition rate
    if(!partners[(int)SexualPartnership::Type::Steady].empty() || !partners[(int)SexualPartnership::Type::Steady].empty())
    {
        //if we're thinking of getting another partner, then lower chances if we have a steady partner
        partnerRate *= populationSpecificParams.getPartnerAcqMultWithSteady(getRiskLevel());
    }

    //if person is over the age of partnering discounting, then discount acquisition rate
    int ageYrs = getAge(TimeGranularity::Year);

    if(ageYrs >= populationSpecificParams.getPartneringDiscStartAgeYrs())
    {
        partnerRate *= populationSpecificParams.getPartneringAcqDiscMult(ageYrs);
    }

    /** To get the number of partners to draw this month, draw from a Poisson distribution */
    int numPartners = _randomNums.randPoisson(partnerRate);
    //if we are rolling for STEADY, make sure we have max of 1
    return (_partnershipType != SexualPartnership::Type::Steady && _partnershipType != SexualPartnership::Type::SteadyMsm) ? numPartners : min(1, numPartners);
}

int Msmw::rollNumEventsPerPartner(Entity *_p, RandomNumberGenerator &_randomNums, SexualPartnership::Type _partnershipType)
{
    assert((_p != nullptr));
    assert(_p->isAlive());

    double meanCoitalEvents = numActsPerMonth[(int)_partnershipType];

    //if person is over the age of partnering discounting, then discount #acts
    int ageYrs = getAge(TimeGranularity::Year);

    if(ageYrs >= populationSpecificParams.getPartneringDiscStartAgeYrs())
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


int Msmw::rollForNewPartnershipDuration(SexualPartnership::Type _partnershipType, RandomNumberGenerator &_randomNums, Entity *_p)
{
    assert((_p != nullptr));
    assert(_p->isAlive());
    assert(_partnershipType < SexualPartnership::Type::ENDType);
    ShiftedLogNormalDist duration = populationSpecificParams.getSexualBehavior(_partnershipType).getPartnershipDurationMth(risk);
    return (int)(_randomNums.randShiftedLogNormal(duration) + .5);
}

void Msmw::rerollRiskGroup(EventParams &_eventParams)
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
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Msmw " << getID() << " rerolls as ";

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

void Msmw::saveState(ostream &_outStream, long currTime)
{
    _outStream << "gend:m," << std::endl;
    Entity::saveState(_outStream, currTime);
    _outStream << "," << std::endl << "circ:" << circumcised << "," << std::endl;
    //partner acquisition rates
    bool firstInSequence = true;
    _outStream << "partAcqR:[";

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        if(!firstInSequence)
        {
            _outStream << ",";
        }

        firstInSequence = false;
        _outStream << partnerAcqRates[(int)partnership_type];
    }

    _outStream << "]," << std::endl;
    //Acts per month
    firstInSequence = true;
    _outStream << "actsPerMth:[";

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        if(!firstInSequence)
        {
            _outStream << ",";
        }

        firstInSequence = false;
        _outStream << numActsPerMonth[(int)partnership_type];
    }

    _outStream << "]," << std::endl;
    //Acts per month
    firstInSequence = true;
    _outStream << "probCndm:[";

    for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
    {
        if(!firstInSequence)
        {
            _outStream << ",";
        }

        firstInSequence = false;
        _outStream << chanceCondomUsePerEvent[(int)partnership_type];
    }

    _outStream << "]";
}

} // namespace transm
