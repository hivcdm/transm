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

//#define PARTNERSHIPDB_DEBUG

namespace transm {
    namespace bmi = boost::multi_index;

    class PartnershipDB final {

    private:
	struct byEndDate {};
	struct byEntityId {};
	struct byPartnershipType {};

	typedef bmi::multi_index_container<
	    std::shared_ptr<SexualPartnership>,
	    bmi::indexed_by<
		// Index ordered by end date for fast monthly dissolution
		bmi::ordered_non_unique<
		    bmi::tag<byEndDate>,
		    bmi::const_mem_fun<SexualPartnership, int, &SexualPartnership::getTimeOfDissolutionAsInt>
		>,
		// Hash by entity id
		bmi::hashed_non_unique<
		    bmi::tag<byEntityId>,
		    bmi::const_mem_fun<SexualPartnership, unsigned long, &SexualPartnership::getInitiatorID>
		>,
		// Hash by entity id and partnership type
		bmi::hashed_non_unique<
		    bmi::tag<byPartnershipType>,
		    bmi::composite_key<
			SexualPartnership,
			bmi::const_mem_fun<SexualPartnership, unsigned long, &SexualPartnership::getInitiatorID>,
			bmi::const_mem_fun<SexualPartnership, std::string, &SexualPartnership::getTypeString>
		    >
		>
	    >
	> partnership_db;
	partnership_db db;

	// No copying allowed
	PartnershipDB(const PartnershipDB& that);
	PartnershipDB& operator=(const PartnershipDB& that);

    public:
	typedef bmi::index<partnership_db, byEndDate>::type end_date_index;
	typedef bmi::index<partnership_db, byEntityId>::type entity_index;
	typedef bmi::index<partnership_db, byPartnershipType>::type entity_type_index;

	using end_date_pair = std::pair<end_date_index::iterator,end_date_index::iterator>;
	using entity_pair = std::pair<entity_index::iterator,entity_index::iterator>;
	using entity_type_pair = std::pair<entity_type_index::iterator,entity_type_index::iterator>;

	PartnershipDB();
	~PartnershipDB();

	void AddPartnership(std::shared_ptr<SexualPartnership> _partnership);

	end_date_pair FindExpired(Time time);
	size_t NumExpired(Time time);
	void RemoveExpired(Time time);
	//void remove_expired(end_date_pair index_pair);

	entity_pair FindWithEntity(Entity *entity);
	void RemoveWithEntity(Entity *entity);
	//void remove_with_entity(entity_pair index_pair);

	entity_type_pair FindWithEntityAndType(Entity *entity, SexualPartnership::Type type);

	void Info() {
#ifdef PARTNERSHIPDB_DEBUG
	    std::cout << "PartnershipDB: " << db.size() << std::endl;
#endif
	}

      	// check if the partnership of this type exists between these entities
	bool PartnershipExists(Entity *partner1, Entity *partner2, SexualPartnership::Type type);

	size_t NumPartners(Entity *entity, SexualPartnership::Type _type);

        /*
	 * returns the number of partners by partnership type that are either the
	 * samerisk or different
	 */
	size_t NumPartners(Entity *entity, SexualPartnership::Type _type, bool sameRisk);

#if 0
      	void remove_partnership(SexualPartnership *partnership);

	// check if the person has a relationship of the given type
	bool partnership_of_type(Entity *partner1, SexualPartnership::Type type);

	// check if the person has any partnerships
	bool partnership_of_any_type(Entity *partner1);
#endif     
    };

} /* namespace transm */
