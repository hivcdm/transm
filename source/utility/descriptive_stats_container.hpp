#pragma once

#include <cmath>
#include <set>
#include <type_traits>
#include <unordered_map>

namespace transm {

template<typename T>
class descriptive_stats_container
{
	using count_container = std::unordered_map<T, std::size_t>;
	using value_container = std::multiset<T>;

public:
	descriptive_stats_container() :
		sum_(0),
		mode_({ 0, false }),
		min_({ 0, false }),
		max_({ 0, false })
	{

	}

	void insert(const T &v) 
	{ 
		counts_[v]++; 
		values_.insert(v);
		sum_ += v;

		if (!mode_.second || counts_[v] > counts_[mode_.first])
		{
			mode_ = { v, true };
		}

		if (!min_.second || v < min_.first)
		{
			min_ = { v, true };
		}

		if (!max_.second || v > max_.first)
		{
			max_ = { v, true };
		}
	}

	double get_mean() const { return sum_ / values_.size(); }

	double get_stddev() const
	{ 
		double sum_square_diff = 0;
		auto mean = get_mean();

		for (const auto &v : counts_)
		{
			auto diff = static_cast<double>(v.first) - mean;
			sum_square_diff += v.second * diff * diff;
		}

		return std::sqrt(sum_square_diff / values_.size());
	}

	double get_median() const
	{ 
		std::size_t i = 0;
		double median = 0;

		for (const auto &v : values_)
		{
			if (i == values_.size() / 2)
			{
				if (values_.size() % 2 == 1) // odd, return normal median
				{
					median = static_cast<double>(v);
				}
				else // even, return average of two middle values
				{
					median += static_cast<double>(v) / 2;
				}
			}
			else if (i > values_.size() / 2)
			{
				break;
			}

			i++;
		}

		return median;
	}

	T get_mode() const { if (!mode_.second) throw std::runtime_error("no mode"); return mode_.first; }
	T get_min() const { if (!min_.second) throw std::runtime_error("no min"); return min_.first; }
	T get_max() const { if (!max_.second) throw std::runtime_error("no max"); return max_.first; }

	std::size_t get_num_samples() const { return values_.size(); }

private:
	double sum_;
	std::pair<T, bool> mode_;
	std::pair<T, bool> min_;
	std::pair<T, bool> max_;
	count_container counts_;
	value_container values_;
};

} // namespace transm