#include "partnershipdb.hpp"

namespace transm {

	PartnershipDB::PartnershipDB() {
//		multi_index_db db;
	}

	auto PartnershipDB::add_partnership(SexualPartnership& partnership) {
		auto return_tuple = PartnershipDB::db.insert(partnership);
		return return_tuple;
	}

	auto PartnershipDB::remove_partnership(SexualPartnership& partnership) {
		auto return_tuple = PartnershipDB::db.erase(db.iterator_to(partnership));
		return return_tuple;
	}

	auto PartnershipDB::find(SexualPartnership& partnership) {
		//auto element_ref = db.find<byKey>(partnership);
		//return std::make_pair(PartnershipDB::db.iterator_to<0>().find(partnership), PartnershipDB::db.iterator_to<0>().end());
		return PartnershipDB::db.end();
		//return element_ref == db.end<byKey>() ? db.iterator_to<byKey>(partnership) : nullptr;
	}

	auto PartnershipDB::find_expired(int time) {
		return std::make_pair(db.upper_bound(time), db.end());
		//auto element_ref = db.upper_bound<byEndDate>((long)time);
		//return element_ref == db.end<byEndDate>() ? element_ref : nullptr;
	}

} /* namespace transm */
