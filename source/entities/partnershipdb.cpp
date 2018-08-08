#include "partnershipdb.hpp"

#define PARTNERSHIPDB_DEBUG
//#define PARTNERSHIPDB_DEBUG_VERBOSE

namespace transm {

    PartnershipDB::PartnershipDB() {
    }

    PartnershipDB::~PartnershipDB() {
    }

    // add single partnership to the db
    void PartnershipDB::AddPartnership(SexualPartnership _partnership)
    {
	db.insert(_partnership);
    }

    PartnershipDB::end_date_pair PartnershipDB::FindExpired(Time time)
    {
	end_date_index& index = db.get<byEndDate>();
	end_date_pair iter = index.equal_range(time.in_months());
#ifdef PARTNERSHIPDB_DEBUG
	std::cout << __func__ << "  " << index.size() << std::endl;
#endif
	return iter;
    }

    void PartnershipDB::RemoveExpired(Time time)
    {
	end_date_index& index = db.get<byEndDate>();
	std::pair<end_date_index::iterator, end_date_index::iterator> iter =
	    index.equal_range(time.in_months());
#ifdef PARTNERSHIPDB_DEBUG
	std::cout << __func__ << " before " << index.size() << " removing " << index.count(time.in_months()) << std::endl;
#endif
	index.erase(iter.first, iter.second);
#ifdef PARTNERSHIPDB_DEBUG
	std::cout << __func__ << " after " << index.size() << std::endl;
#endif
    }

    PartnershipDB::initiator_pair PartnershipDB::FindWithInitiator(Entity *entity)
    {
	initiator_index& index = db.get<byInitiatorId>();
	initiator_pair iter = index.equal_range(entity->getID());
#ifdef PARTNERSHIPDB_DEBUG_VERBOSE
	std::cout << __func__ << "  " << index.size() << std::endl;
#endif
	return iter;
    }

    void PartnershipDB::RemoveWithInitiator(Entity *entity)
    {
	initiator_index& index = db.get<byInitiatorId>();
	initiator_pair iter = index.equal_range(entity->getID());

#ifdef PARTNERSHIPDB_DEBUG_VERBOSE
	std::cout << __func__ << " before  " << index.size() << " removing " << index.count(entity->getID()) << std::endl;
#endif
	index.erase(iter.first, iter.second);
#ifdef PARTNERSHIPDB_DEBUG_VERBOSE
	std::cout << __func__ << " after  " << index.size() << std::endl;

#endif
    }

    PartnershipDB::partner_pair PartnershipDB::FindWithPartner(Entity *entity)
    {
	partner_index& index = db.get<byPartnerId>();
	partner_pair iter = index.equal_range(entity->getID());
#ifdef PARTNERSHIPDB_DEBUG_VERBOSE
	std::cout << __func__ << "  " << index.size() << std::endl;
#endif
	return iter;
    }

    void PartnershipDB::RemoveWithPartner(Entity *entity)
    {
	partner_index& index = db.get<byPartnerId>();
	partner_pair iter = index.equal_range(entity->getID());

#ifdef PARTNERSHIPDB_DEBUG_VERBOSE
	std::cout << __func__ << " before  " << index.size() << " removing " << index.count(entity->getID()) << std::endl;
#endif
	index.erase(iter.first, iter.second);
#ifdef PARTNERSHIPDB_DEBUG_VERBOSE
	std::cout << __func__ << " after  " << index.size() << std::endl;
#endif
    }

    PartnershipDB::entity_type_pair PartnershipDB::FindWithEntityAndType(Entity *entity, SexualPartnership::Type type)
    {
	entity_type_index& index = db.get<byPartnershipType>();
	entity_type_pair iter = index.equal_range(make_tuple(entity->getID(),
	    SexualPartnership::GetTypeString(type)));
	return iter;
    }

    bool PartnershipDB::PartnershipExists(Entity *partner1, Entity *partner2, SexualPartnership::Type type)
    {
	auto partnerships = FindWithEntityAndType(partner1, type);
	for (auto iter = partnerships.first; iter != partnerships.second; iter++) {
	    if ((*iter).isMember(partner2))
		return true;
	}
	return false;
    }

    size_t PartnershipDB::NumPartners(Entity *entity, SexualPartnership::Type _type)
    {
	size_t numPartners = 0;

	initiator_index& index1 = db.get<byInitiatorId>();
	numPartners += index1.count(entity->getID());

	partner_index& index2 = db.get<byPartnerId>();
	numPartners += index2.count(entity->getID());

	// it's faster to get it from the partner counter owned by the entity, but it's not working!
	//assert(numPartners == entity->getNumPartners(_type));
	//numPartners = entity->getNumPartners(_type);

	return numPartners;
    }

    size_t PartnershipDB::NumPartners(Entity *entity, SexualPartnership::Type _type,
	bool sameRisk)
    {
	size_t numPartners = 0;
	Entity *partner;

	auto partnerships = FindWithEntityAndType(entity, _type);
	for (auto iter = partnerships.first; iter != partnerships.second; iter++)
	{
	    SexualPartnership partnership = *iter;
	    if ((partner = partnership.getInitiator()) != entity)
	    {
		partner = partnership.getPartner();
	    }

	    bool isSameRisk = (entity->getRiskLevel() == partner->getRiskLevel());
	    if(isSameRisk == sameRisk)
	    {
		numPartners += 1;
	    }
	}
	return numPartners;
    }

    void PartnershipDB::Info()
    {
#ifdef PARTNERSHIPDB_DEBUG
	std::cout << "PartnershipDB: " << db.size() << std::endl;
#endif
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
