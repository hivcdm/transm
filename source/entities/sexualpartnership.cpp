#include <cmath>
#include <string>

#include "sexualpartnership.hpp"
#include "entity.hpp"
#include "male.hpp"
#include "parameters/eventparams.hpp"
#include "statistics/populationstatistics.hpp"

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

SexualPartnership::SexualPartnership(Entity *_person1, Entity *_person2, EventParams &_eventParams,
                                     SexualPartnership::Type _partnershipType)
{
	//save the type of partnership this is
	type = _partnershipType;
	//save time of partnership formation
	timePartnerFormation = _eventParams.currTime;
	//calculate when this partnership will dissolve. determined by _person1
	int maxDuration =  _person1->rollForNewPartnershipDuration(_partnershipType, _eventParams.randomNums, _person2);

	if(maxDuration < 1)
	{
		maxDuration = 0;
	}

	//if this is true, than this Couple is part of the prevalent population.
	if(_eventParams.currTime == 0)
	{
		if(maxDuration >= 1)
		{
			maxDuration = _eventParams.randomNums.randInt(1, maxDuration);
		}
		else
		{
			//Must be at least 1 so that it will be dissolved in time 1
			maxDuration = 1;
		}
	}

    if((_person1->trace() || _person2->trace()) && _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
	{
        _eventParams.trace_files[EventParams::TraceFile::Type::SinglePerson] << " of duration " << maxDuration << std::endl;
	}

	//set time for partnership to dissolve
	timePartnerDissolution = _eventParams.currTime + maxDuration;
	//save the members of this partnership
	partners[0] = _person1;
	partners[1] = _person2;
	//give each person pointer to this couple so that we can simulate this partnership...
	// all partnerships are stored within the individual Person objects
	// We have made it this way to save on the time it takes to insert and delete objects from a large set of partnerships
	// We give a copy to both of the partners in case one of the partners dies. That way we can end all
	//   partnerships that person was involved in
	partners[0]->addPartnership(this);
	partners[1]->addPartnership(this);
	assert(timePartnerDissolution >= 0);
}

bool SexualPartnership::checkTimeForSplit(long _currTime)
{
	return (_currTime >= timePartnerDissolution);
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
	{
		return getPartner2();
	}
	else
	{
		return getPartner1();
	}
}

SexualPartnership::Type SexualPartnership::getType()
{
	return type;
}

int SexualPartnership::getDissolutionTime()
{
	return timePartnerDissolution;
}
bool SexualPartnership::isMember(Entity *_p)
{
	return ((_p == partners[0]) || (_p == partners[1]));
}

Entity *SexualPartnership::monthlySexualActivity(EventParams &_eventParams, InfectionsTracker *infTrack, const std::unordered_map<TransmissionType, std::array<double, Entity::ENDHVLStrata>> &transmission_coefficients)
{
	int eventsThisMonth = partners[0]->rollNumEventsPerPartner(partners[1], _eventParams.randomNums, type);

	if(eventsThisMonth <= 0)
	{
		eventsThisMonth = 1;
	}

	return partners[0]->sexualActivity(partners[1], eventsThisMonth, type, _eventParams, infTrack, transmission_coefficients);
}

void SexualPartnership::printPartners(std::ostream &_outStream, const std::string &_prefix)
{
	_outStream << _prefix << "Sexual Relationship(" << TypeStrings.at(type) << ")" << std::endl;
	partners[0]->print(_outStream, Constants::TAB);
	_outStream << std::endl;
	partners[1]->print(_outStream, Constants::TAB);
}

void SexualPartnership::saveState(std::ostream &_outStream, int personID, long currTime)
{
	//Saves the type of partnership, the id of partner, and months left in partnership
	auto partnerID = static_cast<int>(partners[0]->getID()) == personID ? partners[1]->getID() : partners[0]->getID();
	_outStream << "{type:" << (int)type << ", partID:" << partnerID << ",tLeft:" << timePartnerDissolution - currTime << "}";
}
SexualPartnership::~SexualPartnership()
{
	partners[0]->removePartnership(this);
	partners[1]->removePartnership(this);
	partners[0] = nullptr;
	partners[1] = nullptr;
}

} // namespace transm
