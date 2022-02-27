#ifndef BUCKET_HPP
#define BUCKET_HPP

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

        boost::hash_combine<std::string>(seed, profile_);

        return seed;
    }

    bool operator==(const Bucket &other) const
    {
        if(other.values_.size() != values_.size())
        {
            return false;
        }

        return (profile_ == other.profile_);
    }

protected:
    std::unordered_map<std::string, int> values_;
    std::string profile_;
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

#endif /* BUCKET_HPP */