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


    std::pair<PartnershipDB::end_date_type::iterator,PartnershipDB::end_date_type::iterator> PartnershipDB::find_expired(Time time)
    {
	end_date_type& index = db.get<byEndDate>();
	std::pair<end_date_type::iterator, end_date_type::iterator> iter = index.equal_range(time.in_months());
	return iter;
    }

    void PartnershipDB::remove_expired(Time time)
    {
	end_date_type& index = db.get<byEndDate>();
	std::pair<end_date_type::iterator, end_date_type::iterator> iter = index.equal_range(time.in_months());
	index.erase(iter.first, iter.second);
    }

#if 0
      // remove single partnership from the db
    void PartnershipDB::remove_partnership(SexualPartnership *partnership)
    {
        end_date_type& index = db.get<byID>();
	end_date_type::iterator iter = index.find(partnership);

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

    auto PartnershipDB::find_with_entity(Entity *entity)
    {
	auto entity_partnerships = db.get<byEntity>(entity.id);
    }

    auto PartnershipDB::remove_entity_partnerships(Entity *entity)
    {
	auto entity_partnerships = db.get<byEntity>(entity.id);
	db.erase(entity_partnerships);
    }

    bool partnership_exists(Entity *partner1, Entity *partner2, SexualPartnership::Type type)
    {
	auto entity_partnerships = find_with_entity(partner1);
	for (auto partnership : entity_partnerships) {
	    if (partnership.isMember(partner2) && type == partnership.getType(type))
		return true;
	}
	return false;
    }

    bool partnership_of_type(Entity *partner1, SexualPartnership::Type type)
    {
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
#endif

} /* namespace transm */
