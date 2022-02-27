#ifndef HASH_TUPLE_HPP
#define HASH_TUPLE_HPP

#include <tuple>

namespace std {

namespace {

template <class T>
inline void hash_combine(std::size_t &seed, const T &v)
{
    seed ^= hash<T>()(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

// Recursive template code derived from Matthieu M.
template <class Tuple, std::size_t Index = std::tuple_size<Tuple>::value - 1>
struct HashValueImpl
{
    static void apply(std::size_t &seed, const Tuple &tuple)
    {
        HashValueImpl<Tuple, Index - 1>::apply(seed, tuple);
        hash_combine(seed, get<Index>(tuple));
    }
};

template <class Tuple>
struct HashValueImpl<Tuple, 0>
{
    static void apply(std::size_t &seed, const Tuple &tuple)
    {
        hash_combine(seed, get<0>(tuple));
    }
};

} // namespace

template <typename ... TT>
struct hash<std::tuple<TT...>>
{
    size_t operator()(const std::tuple<TT...> &tt) const
    {
        size_t seed = 0;
        HashValueImpl<std::tuple<TT...> >::apply(seed, tt);
        return seed;
    }
};

} // namespace stsd


#endif /* HASH_TUPLE_HPP */