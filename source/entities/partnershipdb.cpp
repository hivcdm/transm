#include "partnershipdb.hpp"

namespace transm {

    PartnershipDB::PartnershipDB() {
    }

    // add single partnership to the db
    void PartnershipDB::add_partnership(std::shared_ptr<SexualPartnership> partnership) {
	db.insert(partnership);
    }

    // remove single partnership from the db
    void PartnershipDB::remove_partnership(std::shared_ptr<SexualPartnership> partnership) {
	db.erase(db.iterator_to(partnership));
    }

    auto PartnershipDB::find(SexualPartnership& partnership) {
	//auto element_ref = db.find<byKey>(partnership);
	//return std::make_pair(PartnershipDB::db.iterator_to<0>().find(partnership),
	//    PartnershipDB::db.iterator_to<0>().end());
	return PartnershipDB::db.end();
	//return element_ref == db.end<byKey>() ? db.iterator_to<byKey>(partnership) : nullptr;
    }

    std::pair<end_date_type::iterator,end_date_type::iterator> PartnershipDB::find_expired(Time time) {
	return db.get<byEndType>.equal_range(time.in_months());
    }

    auto PartnershipDB::remove_expired_partnerships(Time time)
    {
	auto expired_partnerships = std::make_pair(db.equal_range(time.in_months()));
	db.erase(expired_partnerships);
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


} /* namespace transm */
