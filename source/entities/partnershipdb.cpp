#include "partnershipdb.hpp"

namespace transm {

    PartnershipDB::PartnershipDB() {
    }

    PartnershipDB::~PartnershipDB() {
    }

    // add single partnership to the db
    void PartnershipDB::add_partnership(Entity *_person1, Entity *_person2, EventParams &_eventParams,
        SexualPartnership::Type _partnershipType)
    {
	db.emplace(_person1, _person2, _eventParams, _partnershipType);
    }


    PartnershipDB::end_date_pair PartnershipDB::find_expired(Time time)
    {
	end_date_index& index = db.get<byEndDate>();
	end_date_pair iter = index.equal_range(time.in_months());
	return iter;
    }

    void PartnershipDB::remove_expired(Time time)
    {
	end_date_index& index = db.get<byEndDate>();
	std::pair<end_date_index::iterator, end_date_index::iterator> iter =
	    index.equal_range(time.in_months());
	index.erase(iter.first, iter.second);
    }

    PartnershipDB::entity_pair PartnershipDB::find_with_entity(Entity *entity)
    {
	entity_index& index = db.get<byEntityId>();
	entity_pair iter = index.equal_range(entity->getID());
	return iter;
    }

    void PartnershipDB::remove_with_entity(Entity *entity)
    {
	entity_index& index = db.get<byEntityId>();
	std::pair<entity_index::iterator, entity_index::iterator> iter = index.equal_range(entity->getID());

	index.erase(iter.first, iter.second);
    }
    
    PartnershipDB::entity_type_pair PartnershipDB::find_with_entity_and_type(Entity *entity, SexualPartnership::Type type)
    {
	entity_type_index& index = db.get<byPartnershipType>();
	entity_type_pair iter = index.equal_range(make_tuple(entity->getID(),
	    SexualPartnership::GetTypeString(type)));
	return iter;
    }

    bool PartnershipDB::partnership_exists(Entity *partner1, Entity *partner2, SexualPartnership::Type type)
    {
	auto partnerships = find_with_entity_and_type(partner1, type);
	for (auto iter = partnerships.first; iter != partnerships.second; iter++) {
	    if (iter->isMember(partner2))
		return true;
	}
	return false;
    }

    size_t PartnershipDB::num_partners(Entity *entity, SexualPartnership::Type _type)
    {
	size_t numPartners = 0;

	entity_type_index& index = db.get<byPartnershipType>();
        numPartners = index.count(make_tuple(entity->getID(),
	    SexualPartnership::GetTypeString(_type)));

	// It's faster to get it from the partner counter owned by the entity
#if 0
	assert(numPartners == entity->getNumPartners(_type));
#endif	
	return numPartners;
    }

    size_t PartnershipDB::num_partners(Entity *entity, SexualPartnership::Type _type,
	bool sameRisk)
    {
	size_t numPartners = 0;
	Entity *partner;

	auto partnerships = find_with_entity_and_type(entity, _type);
	for (auto iter = partnerships.first; iter != partnerships.second; iter++)
	{
	    if(iter->getPartner1() == entity)
	    {
		partner = iter->getPartner2();
	    }
	    else
	    {
		partner = iter->getPartner1();
	    }

	    bool isSameRisk = (entity->getRiskLevel() == partner->getRiskLevel());
	    if(isSameRisk == sameRisk)
	    {
		numPartners += 1;
	    }
	}
	return numPartners;
    }

#if 0
    bool partnership_of_type(Entity *partner1, SexualPartnership::Type type)
    {
	entity_type_index& index = db.get<byPartnershipType>();
	entity_type_pair iter = index.equal_range(make_tuple(entity->getID(),
	    SexualPartnership::GetTypeString(type)));
	
	auto entity_partnerships = db.get<byEntity>(entity.id);
	for (auto partnership : entity_partnerships) {
	    if (type == partnership.getType(type))
		return true;
	}
	return false;
    }

    bool partnership_of_any_type(Entity *partner1)
    {
	return !(db.get<byEntity>(entity.id).empty());
    }

      // remove single partnership from the db
    void PartnershipDB::remove_partnership(SexualPartnership *partnership)
    {
        end_date_index& index = db.get<byID>();
	end_date_index::iterator iter = index.find(partnership);

	index.erase(iter);
	/*
typedef index<employee_set,name>::type employee_set_by_name;
employee_set_by_name& name_index=es.get<name>();

employee_set_by_name::iterator it=name_index.find("Anna Jones");
employee anna=*it;
anna.name="Anna Smith";      // she just got married to Calvin Smith
name_index.replace(it,anna); // update her record
	 */
    }

    auto PartnershipDB::find(SexualPartnership& partnership) {
	//auto element_ref = db.find<byKey>(partnership);
	//return std::make_pair(PartnershipDB::db.iterator_to<0>().find(partnership),
	//    PartnershipDB::db.iterator_to<0>().end());
	return PartnershipDB::db.end();
	//return element_ref == db.end<byKey>() ? db.iterator_to<byKey>(partnership) : nullptr;
    }

#endif

} /* namespace transm */
