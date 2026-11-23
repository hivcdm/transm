#include <gtest/gtest.h>

#include <cstring>
#include <new>

#include "entities/female.hpp"
#include "entities/male.hpp"

namespace transm {

namespace {

/* Builds a person in memory filled with 0xAB, so a member the constructor forgets
 * to set reads as garbage every time instead of only when the heap is not zero. */
template <typename Person, typename... Args>
std::size_t TimesSelectedOfNew(Args &&...args)
{
    alignas(Person) unsigned char storage[sizeof(Person)];
    std::memset(storage, 0xAB, sizeof storage);
    Person *person = new (storage) Person(std::forward<Args>(args)...);
    std::size_t times = person->GetTimesSelected();
    person->~Person();
    return times;
}

DemographicProfile ProfileOf(DemographicProfile::Gender gender, DemographicProfile::SexualOrientation orientation)
{
    DemographicProfile profile;
    profile.set(DemographicProfile::Demographic::Gender, (std::size_t)gender);
    profile.set(DemographicProfile::Demographic::SexualOrientation, (std::size_t)orientation);
    return profile;
}

} // namespace

/* Population::PrintPartnerships reports how often each person was chosen as a
 * partner, so a new person must start at zero. */
TEST(EntityInitTest, NewMaleHasNotBeenSelected)
{
    EventParams events;
    Male::SubPopParams params;
    PrepParameters prep;
    auto profile = ProfileOf(DemographicProfile::Gender::Male, DemographicProfile::SexualOrientation::Msm);
    EXPECT_EQ(TimesSelectedOfNew<Male>(events, Age(30, 0), false, profile, 0u, params, prep), 0u);
}

TEST(EntityInitTest, NewFemaleHasNotBeenSelected)
{
    EventParams events;
    Female::SubPopParams params;
    PrepParameters prep;
    auto profile = ProfileOf(DemographicProfile::Gender::Female, DemographicProfile::SexualOrientation::Msw);
    EXPECT_EQ(TimesSelectedOfNew<Female>(events, Age(30, 0), profile, 0u, params, prep), 0u);
}

} // namespace transm
