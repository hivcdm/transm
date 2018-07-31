#include <cmath>
#include <string>

#include "sexualpartnership.hpp"
#include "entity.hpp"
#include "male.hpp"
#include "parameters/eventparams.hpp"
#include "statistics/populationstatisticsold.hpp"

namespace transm {

const std::map<SexualPartnership::Type, std::string> SexualPartnership::TypeStrings =
{
    {SexualPartnership::Type::Steady, "Steady"},
    {SexualPartnership::Type::Regular, "Regular"},
    {SexualPartnership::Type::Casual, "Casual"},
    {SexualPartnership::Type::Csw, "Csw"},
    {SexualPartnership::Type::SteadyMsm, "SteadyMsm"},
    {SexualPartnership::Type::RegularMsm, "RegularMsm"},
    {SexualPartnership::Type::CasualMsm, "CasualMsm"},
    {SexualPartnership::Type::CswMsm, "CswMsm"}
};

const std::string SexualPartnership::GetTypeString(SexualPartnership::Type type)
{
    return TypeStrings.at(type);
}

SexualPartnership::SexualPartnership(Entity *_person1, Entity *_person2, EventParams &_eventParams,
                                     SexualPartnership::Type _partnershipType)
{
	//save the type of partnership this is
	type = _partnershipType;
	//save time of partnership formation
	timeOfFormation = _eventParams.currTime;
	//calculate when this partnership will dissolve. determined by _person1
	auto maxDuration =  TimeSpan(0, _person1->rollForNewPartnershipDuration(_partnershipType, _eventParams.randomNums, _person2));

	if(maxDuration < TimeSpan::Month)
	{
		maxDuration = TimeSpan(0, 0);
	}

	//if this is true, than this Couple is part of the prevalent population.
	if(_eventParams.currTime.in_months() == 0)
	{
		if(maxDuration >= TimeSpan::Month)
		{
			maxDuration = TimeSpan(0, (int)_eventParams.randomNums.randInt(1, (std::uint32_t)maxDuration.in_months()));
		}
		else
		{
			//Must be at least 1 so that it will be dissolved in time 1
			maxDuration = TimeSpan::Month;
		}
	}

    if((_person1->trace() || _person2->trace()) && _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " of duration " << maxDuration.in_months() << std::endl;
	}

	//set time for partnership to dissolve
	timeOfDissolution = _eventParams.currTime + maxDuration;
	timeOfDissolutionInt = timeOfDissolution.in_months();
	assert(timeOfDissolution.in_months() >= 0);

	//save the members of this partnership
	partners[0] = _person1;
	partners[1] = _person2;
}

bool SexualPartnership::checkTimeForSplit(Time current_time)
{
	return current_time >= timeOfDissolution;
}

Entity *SexualPartnership::getPartner1() const
{
	return partners[0];
}

unsigned long SexualPartnership::getInitiatorID() const
{
    return partners[0]->getID();
}

Entity *SexualPartnership::getPartner2() const
{
	return partners[1];
}

Entity *SexualPartnership::getOtherPartner(Entity *_member)
{
	assert(isMember(_member));

	if(_member == getPartner1())
	{
		return getPartner2();
	}
	else
	{
		return getPartner1();
	}
}

SexualPartnership::Type SexualPartnership::getType() const
{
	return type;
}

std::string SexualPartnership::getTypeString() const
{
    return TypeStrings.at(type);
}

bool SexualPartnership::isMember(Entity *_p) const
{
	return ((_p == partners[0]) || (_p == partners[1]));
}

Entity *SexualPartnership::monthlySexualActivity(EventParams &_eventParams, 
    InfectionsTracker *infTrack, 
    const std::unordered_map<TransmissionType, std::array<double, (std::size_t)Entity::HVLStrata::Last>> &transmission_coefficients) const
{
	int eventsThisMonth = partners[0]->rollNumEventsPerPartner(partners[1], _eventParams.randomNums, type);

	if(eventsThisMonth <= 0)
	{
		eventsThisMonth = 1;
	}

	return partners[0]->sexualActivity(partners[1], eventsThisMonth, type, _eventParams, infTrack, transmission_coefficients);
}

SexualPartnership::~SexualPartnership()
{
    partners[0]->removePartnership(getType());
    partners[1]->removePartnership(getType());
    partners[0] = nullptr;
    partners[1] = nullptr;
}

} // namespace transm
