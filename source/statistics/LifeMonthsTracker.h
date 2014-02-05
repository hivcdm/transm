#pragma once

#include <vector>

#include <boost/tuple/tuple.hpp>

#include "TabularOutput.h"
#include "BucketCounter.h"
#include "../entities/Person.h"

class LifeMonthsTracker : protected TabularOutput
{
public:
	LifeMonthsTracker();

	~LifeMonthsTracker();

	void SetAgeRanges(const std::vector<boost::tuple<long, int, int>> &ageGroupSizes);

	void RecordLifeMonth(Person *person);

	void PrintSummary(std::ostream &_outStream);

	void Reset() { counter_.Reset(); }

	bool IsTimeToRecord(int time) { return time >= start_time_ && (end_time_ < 0 || time <= end_time_); }

	int GetEndTime() { return end_time_; }

	void SetEndTime(int time) { end_time_ = time; }

	int GetStartTime() { return start_time_; }

	void SetStartTime(int time) { start_time_ = time; }

private:
	int start_time_;
	
	int end_time_;

	std::vector<std::pair<int, int>> age_ranges_;

	BucketCounter counter_;
};
