#pragma once

#include <map>
#include <string>
#include <boost/functional/hash.hpp>

namespace transm {

class Bucket
{
public:
	bool HasKeyValue(std::string key, int value) const
	{
        std::unordered_map<std::string,int>::const_iterator itr = values_.find(key);
		if (itr != values_.end())
        {
            if (itr->second == value)
                return true;
            else
                return false;
        }
        return false;
	}

	size_t Hash() const
	{
		size_t seed = 0;
        for (auto itr = values_.begin(); itr != values_.end(); itr++)
        {
            boost::hash_combine<std::string>(seed, itr->first);
        }

        return seed;
	}

	bool operator==(const Bucket &other) const
	{
		if(other.values_.size() != values_.size())
		{
			return false;
		}

        for (auto itr = values_.begin(), itr_other = other.values_.begin();
             itr != values_.end(); itr++, itr_other++)
        {
			if ((itr->first == itr_other->first) &&
                (itr->second == itr_other->second))
			{
				return true;
			}
		}

		return false;
	}

protected:
	std::unordered_map<std::string, int> values_;
};

template<class T> struct bucket_hash;
template<class T> struct bucket_equal_to;

template<>
struct bucket_hash<Bucket>
{
	size_t operator()(const Bucket &bucket) const
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

} // namespace transm
