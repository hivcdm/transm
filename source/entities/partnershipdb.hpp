#include "sexualpartnership.hpp"
#include "entity.hpp"
#include "utility/time.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/random_access_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE

namespace transm {
    namespace bmi = boost::multi_index;

    class PartnershipDB final {

    private:
	struct byEndDate {};
	struct byPartnershipType {};
	struct byEntityId {};

	typedef bmi::multi_index_container<
	    SexualPartnership,
	    bmi::indexed_by<
		// Index ordered by end date for fast monthly dissolution
		bmi::ordered_non_unique<
		    bmi::tag<byEndDate>,
		    bmi::mem_fun<SexualPartnership, int, &SexualPartnership::getTimeOfDissolutionInMonths>
		>,
		// Index by partnership type
		bmi::ordered_non_unique<
		    bmi::tag<byPartnershipType>,
		    bmi::mem_fun<SexualPartnership, SexualPartnership::Type,  &SexualPartnership::getType>
		>,
		// Index by entity id for fast dissolution upon entity death
		bmi::ordered_unique<
		    bmi::tag<byEntityId>,
		    bmi::const_mem_fun<Entity, unsigned long, &Entity::getID>
		>
	    >
	> partnership_db;
	partnership_db db;

	// No copying allowed
	PartnershipDB(const PartnershipDB& that);
	PartnershipDB& operator=(const PartnershipDB& that);

    public:
	PartnershipDB();
	~PartnershipDB();
	void add_partnership(SexualPartnership *partnership);
	void remove_partnership(SexualPartnership *partnership);
	auto find(SexualPartnership& partnership);
	auto find_expired(Time time);
	auto remove_expired(Time time);
	auto find_with_entity(Entity *entity);
	auto remove_entity_partnerships(Entity *entity);

	// check if the partnership of this type exists between these entities
	bool partnership_exists(Entity *partner1, Entity *partner2, SexualPartnership::Type type);

	// check if the person has a relationship of the given type
	bool partnership_of_type(Entity *partner1, SexualPartnership::Type type);

	// check if the person has any partnerships
	bool partnership_of_any_type(Entity *partner1);
    };

} /* namespace transm */
