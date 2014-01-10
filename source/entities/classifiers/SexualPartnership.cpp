#include "SexualPartnership.h"
#include "../Person.h"
#include "../Male.h"
#include "../../data/EventParams.h"
#include "../../statistics/PopStats.h"
#include "../../util/XMLUtil.h"
#include <cmath>
#include <string>

const std::string SexualPartnership::TypeEnumStrs[ENDType] = {"Steady","Regular","Casual","CSW"};

EnumCls<SexualPartnership::Type> SexualPartnership::TypeEnum(SexualPartnership::TypeEnumStrs, SexualPartnership::ENDType);

SexualPartnership::SexualPartnership() {
	cerr << "Called: SexualPartnership::SexualPartnership()" << endl;
	assert(Constants::SHOULD_NOT_BE_CALLING_ME);
}

SexualPartnership::SexualPartnership(Person * _person1, Person * _person2, EventParams& _eventParams, SexualPartnership::Type _partnershipType){
	//save the type of partnership this is
	this->type = _partnershipType;
	//save time of partnership formation
	this->timePartnerFormation = _eventParams.currTime;

	//calculate when this partnership will dissolve. determined by _person1
	int maxDuration =  _person1->rollForNewPartnershipDuration(_partnershipType, _eventParams.randomNums,_person2);

	if (maxDuration < 1){
		maxDuration = 0;
	}

	//if this is true, than this Couple is part of the prevalent population.
	if(_eventParams.currTime == 0){
		if (maxDuration >= 1){
			maxDuration = _eventParams.randomNums.randInt(1, maxDuration);
		} else {
			//Must be at least 1 so that it will be dissolved in time 1
			maxDuration = 1;
		}
	}

	if ((_person1->trace() || _person2->trace()) && _eventParams.outputTrace[EventParams::SINGLEPERSON]){
		_eventParams.traceStreams[EventParams::SINGLEPERSON] << " of duration " << maxDuration << endl;
	}

	//set time for partnership to dissolve
	this->timePartnerDissolution = _eventParams.currTime + maxDuration;

	//save the members of this partnership
	this->partners[0] = _person1;
	this->partners[1] = _person2;

	//give each person pointer to this couple so that we can simulate this partnership...
	// all partnerships are stored within the individual Person objects
	// We have made it this way to save on the time it takes to insert and delete objects from a large set of partnerships
	// We give a copy to both of the partners in case one of the partners dies. That way we can end all
	//   partnerships that person was involved in
	this->partners[0]->addPartnership(this);
	this->partners[1]->addPartnership(this);

	assert(this->timePartnerDissolution >= 0);
}

bool SexualPartnership::checkTimeForSplit(long _currTime) {
	return (_currTime >= this->timePartnerDissolution);
}

Person * SexualPartnership::getPartner1() {
	return this->partners[0];
}

Person * SexualPartnership::getPartner2() {
	return this->partners[1];
}

Person* SexualPartnership::getOtherPartner(Person* _member) {
	assert(this->isMember(_member));
	if( _member == this->getPartner1())
		return this->getPartner2();
	else
		return this->getPartner1();
}

SexualPartnership::Type SexualPartnership::getType() {
	return this->type;
}

int SexualPartnership::getDissolutionTime(){
	return this->timePartnerDissolution;
}
bool SexualPartnership::isMember(Person *_p) {
	return  ((_p == this->partners[0]) || (_p == this->partners[1]));
}

Person* SexualPartnership::monthlySexualActivity(EventParams& _eventParams, InfectionsTracker *infTrack) {
	int eventsThisMonth = this->partners[0]->rollNumEventsPerPartner(partners[1], _eventParams.randomNums, this->type);
	if(eventsThisMonth <= 0)
		eventsThisMonth = 1;

	return this->partners[0]->sexualActivity(this->partners[1], eventsThisMonth, this->type, _eventParams, infTrack);
}

void SexualPartnership::printPartners(ostream& _outStream, string _prefix) {
	_outStream << _prefix << "Sexual Relationship(" << *(SexualPartnership::TypeEnum.toString(this->type)) << ")" << endl;
	partners[0]->print(_outStream, Constants::TAB);
	_outStream << endl;
	partners[1]->print(_outStream, Constants::TAB);
}

void SexualPartnership::saveState(ostream & _outStream, int personID, long currTime){
	//Saves the type of partnership, the id of partner, and months left in partnership
	int partnerID = this->partners[0]->getID()==personID?this->partners[1]->getID():this->partners[0]->getID();
	_outStream << "{type:" << this->type << ", partID:" << partnerID << ",tLeft:" << this->timePartnerDissolution-currTime << "}";
}
SexualPartnership::~SexualPartnership()  {
	partners[0]->removePartnership(this);
	partners[1]->removePartnership(this);

	partners[0] = NULL;
	partners[1] = NULL;
}
