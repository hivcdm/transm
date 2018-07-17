#include "sexualpartnership.hpp"
#include "utility/time.hpp"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>

#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE

namespace transm {
    namespace bmi = boost::multi_index;

    class PartnershipDB final {

    private:
	struct byEndDate {};
	struct byPartnershipType {};

	typedef bmi::multi_index_container<
	    SexualPartnership,
	    bmi::indexed_by<
		// Sequence partnerships by type
		bmi::sequenced<bmi::tag<byPartnershipType>>,
		// Index in order by end date for quick dissolution
		bmi::ordered_non_unique<
		    bmi::tag<byEndDate>,
		    bmi::mem_fun<SexualPartnership, Time, &SexualPartnership::getTimeOfDissolution>
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
	void add_partnership(SexualPartnership& partnership);
	void remove_partnership(SexualPartnership& partnership);
	auto find(SexualPartnership& partnership);
	auto find_expired(int time);
    };

} /* namespace transm */
