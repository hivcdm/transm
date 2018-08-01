#include "female.hpp"
#include "male.hpp"
#include "msmw.hpp"
#include "core/constants.hpp"
#include "utility/utility.hpp"

namespace transm {

std::string Female::getEntityType() const
{
    return "female";
}

int Female::rollForNewPartnershipDuration(SexualPartnership::Type, RandomNumberGenerator &, Entity *)
{
    throw std::runtime_error("not implemented for women");
}

int Female::rollForNumPartners(RandomNumberGenerator &, SexualPartnership::Type)
{
    throw std::runtime_error("not implemented for women");
}

int Female::rollNumEventsPerPartner(Entity *, RandomNumberGenerator &, SexualPartnership::Type)
{
    throw std::runtime_error("not implemented for women");
}

bool Female::possibleMatch(SexualPartnership::Type /*_partnershipType*/, Entity * /*_p*/)
{
    throw std::runtime_error("not implemented for women");
}

void Female::SetCoitalEventsPerMonth(RiskLevel, SexualPartnership::Type, double)
{

}

void Female::SetPartnershipDuration(RiskLevel, SexualPartnership::Type, ShiftedLogNormalDist)
{

}

void Female::SetAverageYearsYounger(SexualPartnership::Type, NormalDist)
{

}

void Female::SetAcquisitionRatePerMonth(RiskLevel, SexualPartnership::Type, LogNormalDist, RandomNumberGenerator &)
{

}

void Female::Circumcise()
{
    throw std::runtime_error("not implemented for women");
}

double Female::rollForAgeDifference(SexualPartnership::Type /*_partnershipType*/, RandomNumberGenerator &/*_randomNums*/)
{
    throw std::runtime_error("not implemented for women");
}

//each index of the array contains parameters for a different population
//(as of 9/8/08, we only have 1 population for now so the size of the vector will default to 1
Female::SubPopParams::SubPopParams() :
    preExposureProphylaxisEfficacy_(0),
    vaginalMicrobicideEfficacy_(0)
{
}

double Female::SubPopParams::GetChanceBecomeCSW() const
{
	return chanceBecomeCSW;
}

double Female::SubPopParams::GetProportionHighRisk(DemographicProfile::Employment _cswStatus) const
{
    return proportionHighRisk[(std::size_t)_cswStatus];
}
NormalDist Female::SubPopParams::GetActivityLevel() const
{
	return activityLevel;
}

Female::Female(EventParams &_eventParams, Age _ageMths, unsigned int _populationID, const Female::SubPopParams &params)
	: Entity(_ageMths, _populationID),
	  populationSpecificParams(params),
	  overrideChanceCondomUse_(-1),
	  times_selected_(0),
	  vaginalMicrobicideAdherence_(0),
	  vaginalMicrobicideApplicationsThisMonth(0),
	  vaginalMicrobicideUsedLastFOICalculation(false)
{
    dmgProfile.set(DemographicProfile::Demographic::Gender, (std::size_t)DemographicProfile::Gender::Female);
	activityLevel = _eventParams.randomNums.randNorm_NaturalNum(populationSpecificParams.GetActivityLevel());

	//activity level should not ever be 0
	if(activityLevel == 0)
	{
		activityLevel = 1;
	}

	risk = Entity::RiskLevel::LOW;

    for(auto risk : {RiskLevel::LOW, RiskLevel::HIGH})
    {
        for(auto partnership_type : enum_iterator<SexualPartnership::Type>())
        {
            partnershipRejectionChance_[risk][partnership_type] = 0;
        }
    }
}

Female::~Female(void)
{
}


/*virtual*/ void Female::SetPreExposureProphylaxisEfficacy(double efficacy)
{
    populationSpecificParams.SetPreExposureProphylaxisEfficacy(efficacy);
}

/*virtual*/ double Female::GetPreExposureProphylaxisEfficacy() const
{
    return populationSpecificParams.GetPreExposureProphylaxisEfficacy();
}

void Female::SetVaginalMicrobicideAdherence(double adherence)
{
    vaginalMicrobicideAdherence_ = adherence;
}

double Female::GetVaginalMicrobicideEfficacy() const
{
    return populationSpecificParams.GetVaginalMicrobicideEfficacy();
}

//in this case, this female is infected and the passed entity is an uninfected male
// FOI = transmission coeff * (1 - (condoms are used and succeed)) * (1 - (male is circumcised))
double Female::getFOI(Entity *_p, const TransmissionCoefficients &transmission_coefficients, SexualPartnership::Type _partnershipType, EventParams &_eventParams)
{
    assert(_p->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male);

    double prepEfficacy = _p->UsingPrEP() ? _p->GetPreExposureProphylaxisEfficacy() : 0;

    double circEff = 0;
    double condomUseProb = 0;
    double condomProtectEff = 0;
    bool circumcised = false;

    if(_p->getEntityType() == "male")
    {
        circEff = ((Male *)_p)->getCircumProtectEff();
        condomUseProb = ((Male *)_p)->getCondomUseProb(this, _partnershipType);
        condomProtectEff = ((Male *)_p)->getCondomProtectEff();
        circumcised = ((Male *)_p)->IsCircumcised();
    }
    else if(_p->getEntityType() == "msmw")
    {
        circEff = ((Msmw *)_p)->getCircumProtectEff();
        condomUseProb = ((Msmw *)_p)->getCondomUseProb(this, _partnershipType);
        condomProtectEff = ((Msmw *)_p)->getCondomProtectEff();
        circumcised = ((Msmw *)_p)->IsCircumcised();
    }
    else
    {
        throw std::runtime_error("invalid partner for female: " + _p->getEntityType());
    }

    //Determine if a condom was used and record
    condomUsedLastFOICalculation = _eventParams.randomNums.chance(condomUseProb);

    //Determine the condom efficacy --> 0 if no condom was used
    double condomEff = condomUsedLastFOICalculation ? condomProtectEff : 0;

    assert(_p->getDemographicProfileVal<DemographicProfile::Gender>() == DemographicProfile::Gender::Male);

    double base_foi = transmission_coefficients.at(TransmissionType::female_to_male)[(std::size_t)getHVL()];
    double FOI = base_foi * (1 - condomEff) * (1 - circEff) * (1 - prepEfficacy);

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !Transmission coefficient from " << getID() << " to " <<
		        _p->getID() << " is " << base_foi;
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ";" << std::endl << " !A condom was ";

		if(!condomUsedLastFOICalculation)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "NOT ";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "used (efficacy " << condomProtectEff;

		if(circumcised)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ");" << std::endl << " !" << _p->getID() <<
			        " is circumcised (efficacy " << circEff;
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ");" << std::endl << " !Total FOI = " << FOI << std::endl;
	}

	return FOI;
}

//currently, females don't have much of a choice. Edit these functions to give them ability have have partner preferences
double Female::getMinPartnerSelectVal(Entity::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	return numeric_limits<unsigned int>::min();
}

double Female::getMaxPartnerSelectVal(Entity::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	return numeric_limits<unsigned int>::max();
}

double Female::getChanceBecomeCsw() const
{
	return populationSpecificParams.GetChanceBecomeCSW();
}

void Female::SetChanceCondomUsePerEvent(Entity::RiskLevel /*risk*/, SexualPartnership::Type /*partnershipType*/, BetaDist /*dist*/, RandomNumberGenerator &/*rng*/)
{

}

void Female::rerollRiskGroup(EventParams &_eventParams)
{
	DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) getDemographicProfileVal(DemographicProfile::Demographic::Employment);
	double chanceHighRisk = populationSpecificParams.GetProportionHighRisk(cswStatus);

	if(_eventParams.randomNums.chance(chanceHighRisk))
	{
		risk = RiskLevel::HIGH;
	}
	else
	{
		risk = RiskLevel::LOW;
	}
}

} // namespace transm
