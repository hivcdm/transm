#include "LifeMonthsTracker.h"

#include "LifeMonthsBucket.h"
#include "../Population.h"

const std::string BUCKETS[] =
{
	"hiv_status"
};

LifeMonthsTracker::LifeMonthsTracker()
    : start_time_(-1),
      end_time_(-1)
{
	std::vector<std::string> tracked;
	tracked.push_back("life_months");
    
	std::vector<std::string> buckets;
	for(auto bucket : BUCKETS)
	{
		buckets.push_back(bucket);
	}
    
	counter_ = BucketCounter(buckets, tracked);
    
	Reset();
}

LifeMonthsTracker::~LifeMonthsTracker()
{
    
}

void LifeMonthsTracker::SetAgeRanges(const std::vector<boost::tuple<long, int, int>> &ageRangeSizes)
{
	age_ranges_.clear();
	int numAgeRanges = static_cast<int>(ageRangeSizes.size());
    
	for(int i = 0; i < numAgeRanges; ++i)
	{
		int minAge = boost::tuples::get<Population::MIN_AGE_IN_MONTHS>(ageRangeSizes.at(i));
		int maxAge = boost::tuples::get<Population::MAX_AGE_IN_MONTHS>(ageRangeSizes.at(i));
		age_ranges_.push_back(std::make_pair(minAge, maxAge));
	}
}

void LifeMonthsTracker::RecordLifeMonth(Person *person)
{
	counter_.Increment(LifeMonthsBucket(*person, age_ranges_), "life_months");
}

void LifeMonthsTracker::PrintSummary(std::ostream &_outStream)
{
    for(int i = 0; i < Person::ENDHIVStatus; i++)
    {
        auto count = counter_.GetCount("life_months", std::make_pair("hiv_status", i));
        _outStream << "HIV Status " << i << " " << count << std::endl;
    }
}