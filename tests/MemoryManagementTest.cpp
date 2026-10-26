#include <gtest/gtest.h>

#include "entities/female.hpp"
#include "entities/male.hpp"

using namespace transm;

namespace {

DemographicProfile MaleProfile()
{
    DemographicProfile profile;
    profile.set(DemographicProfile::Demographic::Gender,
                (std::size_t)DemographicProfile::Gender::Male);
    return profile;
}

} // namespace

TEST(PrepParametersTest, CopiesShareTablesButKeepDefaultsAndChangesIndependent)
{
    PrepParameters original;
    DemographicProfile any;
    auto male = MaleProfile();
    original.SetDefaultAccess({0.1, 0.2});
    original.SetDefaultAdherence({1.0, 0.0, 0.0, 0.0});
    original.SetDefaultReturnToCare(0.3);
    original.SetEfficacy(0.8);
    original.SetEnabled(true);
    auto copy = original;

    EXPECT_EQ(&original.GetAccess(any), &copy.GetAccess(any));
    copy.SetDefaultAdherence({0.0, 1.0, 0.0, 0.0});
    EXPECT_DOUBLE_EQ(original.GetAdherence(any)[0], 1.0);
    EXPECT_DOUBLE_EQ(copy.GetAdherence(any)[1], 1.0);
    EXPECT_EQ(&original.GetAccess(any), &copy.GetAccess(any));

    copy.SetProfileAccess(male, {0.5});
    copy.SetProfileAdherence(male, {0.0, 0.0, 1.0, 0.0});
    std::array<double, 12> retention{};
    retention[0] = 0.7;
    copy.SetProfileRetention(male, retention);
    copy.SetProfileReturnToCare(male, 0.9);
    EXPECT_EQ(original.GetAccess(male), (std::vector<double>{0.1, 0.2}));
    EXPECT_EQ(copy.GetAccess(male), (std::vector<double>{0.5}));
    EXPECT_DOUBLE_EQ(original.GetAdherence(male)[0], 1.0);
    EXPECT_DOUBLE_EQ(copy.GetAdherence(male)[2], 1.0);
    EXPECT_DOUBLE_EQ(original.GetRetention(male)[0], 0.0);
    EXPECT_DOUBLE_EQ(copy.GetRetention(male)[0], 0.7);
    EXPECT_DOUBLE_EQ(original.GetReturnToCare(male), 0.3);
    EXPECT_DOUBLE_EQ(copy.GetReturnToCare(male), 0.9);

    auto snapshot = original;
    original.SetDefaultAccess({0.6});
    original.SetEfficacy(0.4);
    original.SetEnabled(false);
    EXPECT_EQ(snapshot.GetAccess(any), (std::vector<double>{0.1, 0.2}));
    EXPECT_DOUBLE_EQ(snapshot.GetEfficacy(), 0.8);
    EXPECT_TRUE(snapshot.Enabled());
}

TEST(PrepParametersTest, OverlappingProfilesKeepMapOrderAndFirstInsertedValue)
{
    PrepParameters parameters;
    DemographicProfile any;
    auto male = MaleProfile();
    std::map<DemographicProfile, double> reference;
    reference.emplace(any, 0.1);
    reference.emplace(male, 0.2);
    parameters.SetProfileAccess(any, {0.1});
    parameters.SetProfileAccess(male, {0.2});
    parameters.SetProfileAccess(male, {0.9});

    double expected = 0.0;
    for (const auto &entry : reference)
        if (male.match(entry.first))
            expected = entry.second;
    EXPECT_DOUBLE_EQ(parameters.GetAccess(male).at(0), expected);
}
