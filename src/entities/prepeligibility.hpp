#ifndef PREPELIGIBILITY_HPP
#define PREPELIGIBILITY_HPP

#include <array>
#include <cstddef>

namespace transm {

/**
 * The chance that someone stops PrEP this month, from <prepRetention>: its first
 * value is for the first month on PrEP, its twelfth for the twelfth, and the twelfth
 * carries on for every later month.
 *
 * @param monthsOnPrep months on PrEP so far, 1 in the first month after starting */
inline double PrepStopChance(const std::array<double, 12> &stopChances, int monthsOnPrep)
{
    int index = monthsOnPrep - 1;
    if (index < 0)
        index = 0;
    if (index > 11)
        index = 11;
    return stopChances[(std::size_t)index];
}

} // namespace transm

#endif /* PREPELIGIBILITY_HPP */
