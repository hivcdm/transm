#include "Female.h"
#include "Male.h"
#include "../core/Constants.h"
#include "../utility/Utility.h"

int Female::rollForNewPartnershipDuration(SexualPartnership::Type, RandomNumberGenerator &, Person *)
{
    throw std::runtime_error("not implemented for women");
}

int Female::rollForNumPartners(RandomNumberGenerator &, SexualPartnership::Type)
{
    throw std::runtime_error("not implemented for women");
}

int Female::rollNumEventsPerPartner(Person *, RandomNumberGenerator &, SexualPartnership::Type)
{
    throw std::runtime_error("not implemented for women");
}

bool Female::possibleMatch(SexualPartnership::Type /*_partnershipType*/, Person * /*_p*/)
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

double Female::SubPopParams::GetTransmitPerEventCoeff(HVLStrata _hvl) const
{
	return transmitPerEventCoeffs.at(_hvl);
}

Female::Female(EventParams &_eventParams, int _ageMths, unsigned int _populationID, const Female::SubPopParams &params)
	: Person(_ageMths, _populationID),
	populationSpecificParams(params),
    overrideChanceCondomUse_(-1),
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

	risk = Person::LOW;

    for(auto risk : {LOW, HIGH})
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


//-----------< START methods that will populate static fields or store to file >----------------//


//-----------< END methods that will populate static fields or store to file >----------------//

/** These methods are inherited from Person **/


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

double Female::getFOI(Person *_p, SexualPartnership::Type _partnershipType, EventParams &_eventParams)
{
    assert(_p->getDemographicProfileVal(DemographicProfile::Demographic::Gender) == (std::size_t)DemographicProfile::Gender::Male);
	Male *m = (Male *)_p;
	//transmission coeff				  (1 - (condoms are used and succeed)) * (1 - (male is circumcised))
	double circEff = m->getCircumProtectEff();
	//Determine if a condom was used and record
	condomUsedLastFOICalculation = _eventParams.randomNums.chance(m->getCondomUseProb(this, _partnershipType));
	//Determine the condom efficacy --> 0 if no condom was used
	double condomEff = 0;

	if(condomUsedLastFOICalculation)
	{
		condomEff = m->getCondomProtectEff();
	}

    double prepEfficacy = _p->UsingPrEP() ? _p->GetPreExposureProphylaxisEfficacy() : 0;

    double FOI = getTransmissionCoeff() * (1 - condomEff) * (1 - circEff) * (1 - prepEfficacy);

    if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && (trace() || _p->trace()))
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " !Transmission coefficient from " << getID() << " to " <<
		        _p->getID() << " is " << getTransmissionCoeff();
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ";" << std::endl << " !A condom was ";

		if(!condomUsedLastFOICalculation)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "NOT ";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "used (efficacy " << m->getCondomProtectEff();

		if(m->isCircumcised())
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ");" << std::endl << " !" << m->getID() <<
			        " is circumcised (efficacy " << circEff;
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << ");" << std::endl << " !Total FOI = " << FOI << std::endl;
	}

	return FOI;
}

//currently, females don't have much of a choice. Edit these functions to give them ability have have partner preferences
double Female::getMinPartnerSelectVal(Person::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	return numeric_limits<unsigned int>::min();
}

double Female::getMaxPartnerSelectVal(Person::SelectingCriteria /*_PSC*/,
                                      SexualPartnership::Type /*_partnershipType*/) const
{
	return numeric_limits<unsigned int>::max();
}

double Female::getTransmissionCoeff()
{
	assert(Utility::withinRange(hvl, HVL_ZERO, HVL_LATESTAGE));
	return populationSpecificParams.GetTransmitPerEventCoeff(hvl);
}

double Female::getChanceBecomeCsw() const
{
	return populationSpecificParams.GetChanceBecomeCSW();
}

void Female::SetChanceCondomUsePerEvent(Person::RiskLevel /*risk*/, SexualPartnership::Type /*partnershipType*/, BetaDist /*dist*/, RandomNumberGenerator &/*rng*/)
{

}

void Female::rerollRiskGroup(EventParams &_eventParams)
{
	DemographicProfile::Employment cswStatus = (DemographicProfile::Employment) getDemographicProfileVal(DemographicProfile::Demographic::Employment);
	double chanceHighRisk = populationSpecificParams.GetProportionHighRisk(cswStatus);

	if(_eventParams.randomNums.chance(chanceHighRisk))
	{
		risk = HIGH;
	}
	else
	{
		risk = LOW;
	}

    /*
	if(_eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled && trace())
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " % Female " << getID() << " rerolls as ";

		if(risk == HIGH)
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "High";
		}
		else
		{
            _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << "Low";
		}

        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " risk" << std::endl;
	}
    */
}

void Female::saveState(ostream &_outStream, long currTime)
{
	_outStream << "gend:f," << std::endl;
	Person::saveState(_outStream, currTime);
}
