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


	//set time for partnership to dissolve
	timeOfDissolution = _eventParams.currTime + maxDuration;
	timeOfDissolutionInt = timeOfDissolution.in_months();
	assert(timeOfDissolution.in_months() >= 0);

	//save a pointer to the members of this partnership
	initiator = _person1;
	partner =_person2;

	_person1->addPartnership(this);
	_person2->addPartnership(this);

	if((_person1->trace() || _person2->trace()) && _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " of duration " << maxDuration.in_months() << std::endl;
	}
}

bool SexualPartnership::checkTimeForSplit(Time current_time)
{
	return current_time >= timeOfDissolution;
}

Entity *SexualPartnership::getInitiator() const
{
    return initiator;
}

unsigned long SexualPartnership::getInitiatorID() const
{
    return initiator->getID();
}

Entity *SexualPartnership::getPartner() const
{
    return partner;
}

unsigned long SexualPartnership::getPartnerID() const
{
    return partner->getID();
}

Entity *SexualPartnership::getOtherPartner(Entity *_member)
{
	assert(isMember(_member));

	if(_member == getInitiator())
	{
		return getPartner();
	}
	else
	{
		return getInitiator();
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
	return ((_p == initiator) || (_p == partner));
}

Entity *SexualPartnership::monthlySexualActivity(EventParams &_eventParams, InfectionsTracker *infTrack,
    const std::unordered_map<TransmissionType, std::array<double, (std::size_t)Entity::HVLStrata::Last>> &transmission_coefficients) const
{
	int eventsThisMonth = initiator->rollNumEventsPerPartner(partner, _eventParams.randomNums, type);

	if(eventsThisMonth <= 0)
	{
		eventsThisMonth = 1;
	}

	return initiator->sexualActivity(partner, eventsThisMonth, type, _eventParams, infTrack, transmission_coefficients);
}

SexualPartnership::~SexualPartnership()
{
    initiator->removePartnership(getType());
    partner->removePartnership(getType());
    initiator = nullptr;
    partner = nullptr;
}

} // namespace transm
