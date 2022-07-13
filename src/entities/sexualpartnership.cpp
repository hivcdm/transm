#include <cmath>
#include <string>

#include "sexualpartnership.hpp"
#include "entity.hpp"
#include "entitytypes.hpp"
#include "parameters/eventparams.hpp"
#include "statistics/populationstatisticsold.hpp"

namespace transm {

const std::map<SexualPartnership::Type, std::string> SexualPartnership::TypeStrings =
{
	{SexualPartnership::Type::Steady, "Steady"},
	{SexualPartnership::Type::Regular, "Regular"},
	{SexualPartnership::Type::Casual, "Casual"},
	{SexualPartnership::Type::Csw, "Csw"}
};

SexualPartnership::SexualPartnership(Entity *_person1,
                                     Entity *_person2,
                                     EventParams &_eventParams,
                                     SexualPartnership::Type _partnershipType):partners()
{
	/* save the type of partnership this is */
	type = _partnershipType;

	/* save time of partnership formation */
	timePartnerFormation = _eventParams.currTime;

	/* calculate when this partnership will dissolve. determined by _person1 */
	auto maxDuration =  TimeSpan(0, _person1->rollForNewPartnershipDuration(_partnershipType, _eventParams.randomNums, _person2));

	if(maxDuration < TimeSpan::Month)
	{
		maxDuration = TimeSpan(0, 0);
	}

	/* if this is true, than this Couple is part of the prevalent population. */
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

	/* set time for partnership to dissolve */
	timePartnerDissolution = _eventParams.currTime + maxDuration;

	/* save the members of this partnership */
	partners[0] = _person1;
	partners[1] = _person2;

	/* give each person pointer to this couple so that we can simulate this partnership...
	   all partnerships are stored within the individual Person objects
	   We have made it this way to save on the time it takes to insert and delete objects from a large set of partnerships
	   We give a copy to both of the partners in case one of the partners dies. That way we can end all
	   partnerships that person was involved in */
	partners[0]->addPartnership(this);
	partners[1]->addPartnership(this);
	assert(timePartnerDissolution.in_months() >= 0);
}

bool SexualPartnership::checkTimeForSplit(Time current_time) const
{
	return current_time >= timePartnerDissolution;
}

Entity *SexualPartnership::getPartner1()
{
	return partners[0];
}

Entity *SexualPartnership::getPartner2()
{
	return partners[1];
}

Entity *SexualPartnership::getOtherPartner(Entity *_member)
{
	assert(isMember(_member));

	if(_member == getPartner1())
		return getPartner2();
	else
		return getPartner1();
}

SexualPartnership::Type SexualPartnership::getType()
{
	return type;
}

bool SexualPartnership::isMember(Entity *_p)
{
	return ((_p == partners[0]) || (_p == partners[1]));
}

Entity *SexualPartnership::monthlySexualActivity(EventParams &_eventParams, 
    InfectionsTracker *infTrack, 
    const std::unordered_map<TransmissionType, std::array<double, (std::size_t)HVLStrata::Last>> &transmission_coefficients)
{
	int eventsThisMonth = partners[0]->rollNumEventsPerPartner(partners[1], _eventParams.randomNums, type);

	if(eventsThisMonth <= 0)
	{
		eventsThisMonth = 1;
	}

	if ((partners[0]->getEntityType() == "female" && partners[1]->getEntityType() == "msm") ||
		(partners[1]->getEntityType() == "female" && partners[0]->getEntityType() == "msm"))
		assert(0);

	return partners[0]->sexualActivity(partners[1], eventsThisMonth, type, _eventParams, infTrack, transmission_coefficients);
}

SexualPartnership::~SexualPartnership()
{
	partners[0]->removePartnership(this);
	partners[1]->removePartnership(this);
	partners[0] = nullptr;
	partners[1] = nullptr;
}

} // namespace transm
