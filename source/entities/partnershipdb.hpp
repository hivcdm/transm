/*
 * PartnershipDB.h
 *
 *  Created on: 09 mag 2016
 *      Author: giulio
 */

#include "SexualPartnership.h"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/identity.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/mem_fun.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index/sequenced_index.hpp>


#ifndef SOURCE_ENTITIES_CLASSIFIERS_PARTNERSHIPDB_H_
#define SOURCE_ENTITIES_CLASSIFIERS_PARTNERSHIPDB_H_

#define BOOST_MULTI_INDEX_ENABLE_SAFE_MODE

namespace DB {
	namespace bmi = boost::multi_index;

	class PartnershipDB final {

	private:
		struct byKey {};
		struct byEndDate {};
/*
		typedef multi_index_container<
				SexualPartnership,
				indexed_by<
					hashed_unique<
						tag<
							byKey
						>,
						identity<
							SexualPartnership>
						>
					>,
					indexed_by<
						ordered_non_unique<
							tag<
								byEndDate
							>,
							boost::multi_index::mem_fun<SexualPartnership, int, &SexualPartnership::getDissolutionTime
							>
						>
					>
				>
				multi_index_db;*/
		/*bmi::member<
			SexualPartnership, const int, &SexualPartnership::prova>
			SexualPartnership, const int, &SexualPartnership::timePartnerDissolution
			SexualPartnership, int, &SexualPartnership::getDissolutionTime
		>*/
		typedef bmi::multi_index_container<
				SexualPartnership,
					bmi::indexed_by<
						bmi::ordered_non_unique<
							bmi::tag<
								byEndDate
							>,
							bmi::member<
								SexualPartnership, int, &SexualPartnership::timePartnerDissolution
							>
						>
					>
				>
				partnership_db;

		partnership_db db;

		// No copying allowed
		PartnershipDB(const PartnershipDB& that);
		PartnershipDB& operator=(const PartnershipDB& that);

	public:
		PartnershipDB();
		~PartnershipDB();
		auto add_partnership(SexualPartnership& partnership);
		auto remove_partnership(SexualPartnership& partnership);
		auto find(SexualPartnership& partnership);
		auto find_expired(int time);
	};

} /* namespace DB */

#endif /* SOURCE_ENTITIES_CLASSIFIERS_PARTNERSHIPDB_H_ */
