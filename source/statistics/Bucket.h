#pragma once

#include <cassert>
#include <map>
#include <string>
#include <boost/functional/hash.hpp>

class Bucket
{
public:
	std::size_t GetValue(std::size_t i) const
	{
		return values_[i];
	}

	std::size_t Hash() const
	{
		std::size_t seed = 0;
		for(auto value : values_)
		{
			boost::hash_combine(seed, value);
		}
		return seed;
	}

	bool operator==(const Bucket &other) const
	{
		if(other.values_.size() != values_.size())
		{
			return false;
		}

		for(std::size_t i = 0; i < values_.size(); i++)
		{
			if(values_[i] != other.values_[i])
			{
				return false;
			}
		}

		return true;
	}

protected:
	std::vector<std::size_t> values_;
};

template<class T> struct bucket_hash;
template<class T> struct bucket_equal_to;

template<>
struct bucket_hash<Bucket>
{
	std::size_t operator()(const Bucket &bucket) const
	{
		return bucket.Hash();
	}
};

template<>
struct bucket_equal_to<Bucket>
{
	bool operator()(const Bucket &a, const Bucket &b) const
	{
		return a == b;
	}
};
