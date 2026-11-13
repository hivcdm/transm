#include <gtest/gtest.h>
#include <cstdio>

#include "statistics/partnernetwork.hpp"

#include "entities/female.hpp"
#include "entities/male.hpp"

namespace transm {

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

TEST(MaleParametersTest, ReadsShareTablesAndInterventionsDetach)
{
    using Type = SexualPartnership::Type;
    using Race = DemographicProfile::Race;
    using Ethnicity = DemographicProfile::Ethnicity;
    Male::SubPopParams original;
    original.SetCoitalEventsPerMonth(RiskLevel::LOW, Type::Steady, 2.0);
    original.SetChanceBecomeCsw(0.1);
    original.setAgeDiscounting(Age(40, 0), 0.1, 0.2);
    auto *races = original.MutableAllowedRaceEthnicityMap();
    races->insert(Race::Black, {Ethnicity::NonHispanic});
    races->SetRaceKeysFromMap();
    original.setBaselineRaceEthnicAssortativeness(0.3);
    auto copy = original;

    EXPECT_EQ(&original.getSexualBehavior(Type::Steady), &copy.getSexualBehavior(Type::Steady));
    copy.SetChanceBecomeCsw(0.9);
    EXPECT_DOUBLE_EQ(original.getChanceBecomeCSW(), 0.1);
    EXPECT_EQ(&original.getSexualBehavior(Type::Steady), &copy.getSexualBehavior(Type::Steady));

    copy.SetCoitalEventsPerMonth(RiskLevel::LOW, Type::Steady, 5.0);
    copy.setAgeDiscounting(Age(40, 0), 0.5, 0.6);
    copy.setRaceEthnicAssortativeness(Race::Black, Ethnicity::NonHispanic, 0.8);
    EXPECT_DOUBLE_EQ(original.getSexualBehavior(Type::Steady).getCoitalEventsPerMonth(RiskLevel::LOW), 2.0);
    EXPECT_DOUBLE_EQ(copy.getSexualBehavior(Type::Steady).getCoitalEventsPerMonth(RiskLevel::LOW), 5.0);
    EXPECT_DOUBLE_EQ(original.getPartneringAcqDiscMult(Age(40, 0)), 0.9);
    EXPECT_DOUBLE_EQ(copy.getPartneringAcqDiscMult(Age(40, 0)), 0.5);
    EXPECT_DOUBLE_EQ(original.getRaceEthnicAssortativeness(Race::Black, Ethnicity::NonHispanic), 0.3);
    EXPECT_DOUBLE_EQ(copy.getRaceEthnicAssortativeness(Race::Black, Ethnicity::NonHispanic), 0.8);

    auto snapshot = original;
    original.MutableSexualBehavior(Type::Steady).setCoitalEventsPerMonth(RiskLevel::LOW, 7.0);
    EXPECT_DOUBLE_EQ(snapshot.getSexualBehavior(Type::Steady).getCoitalEventsPerMonth(RiskLevel::LOW), 2.0);
}

TEST(FemaleParametersTest, RejectionChancesAreZeroAndIndependentlyAddressable)
{
    EventParams events;
    Female::SubPopParams defaults;
    PrepParameters prep;
    DemographicProfile profile;
    profile.set(DemographicProfile::Demographic::Gender,
                (std::size_t)DemographicProfile::Gender::Female);
    Female person(events, Age(25, 0), profile, 0, defaults, prep);

    for (auto risk : enum_iterator<RiskLevel>())
        for (auto type : enum_iterator<SexualPartnership::Type>())
            EXPECT_DOUBLE_EQ(person.GetPartnershipRejectionChance(risk, type), 0.0);

    person.SetPartnershipRejectionChance(RiskLevel::HIGH, SexualPartnership::Type::Casual, 0.7);
    EXPECT_DOUBLE_EQ(person.GetPartnershipRejectionChance(RiskLevel::HIGH, SexualPartnership::Type::Casual), 0.7);
    EXPECT_DOUBLE_EQ(person.GetPartnershipRejectionChance(RiskLevel::LOW, SexualPartnership::Type::Casual), 0.0);
    EXPECT_DOUBLE_EQ(person.GetPartnershipRejectionChance(RiskLevel::HIGH, SexualPartnership::Type::Steady), 0.0);
    EXPECT_THROW(person.GetPartnershipRejectionChance(RiskLevel::Last, SexualPartnership::Type::Steady), std::out_of_range);
}


namespace {

class NetworkTestPerson : public Female
{
public:
    using Female::Female;

    int rollForNewPartnershipDuration(SexualPartnership::Type, RandomNumberGenerator &, Entity *) override
    {
        return 12;
    }
};

class MembershipTestPerson : public Female
{
public:
    using Female::Female;

    bool HasMemberships() const { return !FVindices.empty(); }
};

} // namespace

TEST(NetworkTest, KeepsIsolatesParallelEdgesAndFocusAttributes)
{
    EventParams events;
    events.currTime = Time::from_months(720);
    Female::SubPopParams defaults;
    PrepParameters prep;
    DemographicProfile profile;
    profile.parse("SA:FEMALE:MSW:SINGLE:NON_CSW:WHITE:NON_HISPANIC");
    NetworkTestPerson first(events, Age(25, 0), profile, 0, defaults, prep);
    NetworkTestPerson second(events, Age(25, 0), profile, 0, defaults, prep);
    NetworkTestPerson isolate(events, Age(25, 0), profile, 0, defaults, prep);
    first.recordFOCUSScreening(0);
    SexualPartnership one(&first, &second, events, SexualPartnership::Type::Regular);
    SexualPartnership two(&first, &second, events, SexualPartnership::Type::Regular);

    Network network(720);
    network.AddIsolatedEntity(&first);
    network.AddIsolatedEntity(&second);
    network.AddIsolatedEntity(&isolate);
    network.UpdatePartnership(&first, &second, &one);
    network.UpdatePartnership(&second, &first, &one);
    network.UpdatePartnership(&first, &second, &two);
    network.UpdatePartnership(&second, &first, &two);
    const std::string prefix = testing::TempDir() + "transm-memory-test-";
    const std::string filename = prefix + "PartnershipNetwork_720.graphml";
    network.SetFilenamePrefix(prefix);
    network.Write();

    pugi::xml_document document;
    ASSERT_TRUE(document.load_file(filename.c_str()));
    EXPECT_EQ(document.select_nodes("/graphml/graph/node").size(), 3u);
    EXPECT_EQ(document.select_nodes("/graphml/graph/edge").size(), 2u);
    auto value = [&](const char *node, const char *name) {
        std::string key_query = std::string("/graphml/key[@attr.name='") + name + "']";
        std::string key = document.select_node(key_query.c_str()).node().attribute("id").value();
        std::string query = std::string("/graphml/graph/node[@id='") + node + "']/data[@key='" + key + "']";
        return std::string(document.select_node(query.c_str()).node().child_value());
    };
    EXPECT_EQ(value("n0", "focus_screened"), "1");
    EXPECT_EQ(value("n0", "focus_screen_count"), "1");
    EXPECT_EQ(value("n0", "is_isolate"), "0");
    EXPECT_EQ(value("n2", "is_isolate"), "1");
    EXPECT_EQ(value("n1", "focus_screened"), "0");

    // A second write must refresh attributes, even when the topology is unchanged.
    first.recordFOCUSScreening(0);
    network.Write();
    ASSERT_TRUE(document.load_file(filename.c_str()));
    EXPECT_EQ(value("n0", "focus_screen_count"), "2");
    EXPECT_EQ(std::remove(filename.c_str()), 0);
}

TEST(FullVectorTest, MissingLookupsDoNotInsertAndRemovalKeepsReverseIndices)
{
    EventParams events;
    Female::SubPopParams defaults;
    PrepParameters prep;
    DemographicProfile profile;
    profile.set(DemographicProfile::Demographic::Gender,
                (std::size_t)DemographicProfile::Gender::Female);
    MembershipTestPerson first(events, Age(25, 0), profile, 0, defaults, prep);
    MembershipTestPerson second(events, Age(25, 0), profile, 0, defaults, prep);
    FullVector members;
    FullVector absent;
    EXPECT_TRUE(first.getFVindices(&absent).empty());
    EXPECT_FALSE(absent.exists(&first));
    EXPECT_FALSE(first.HasMemberships());

    members.push_back(&first);
    members.push_back(&second);
    members.push_back(&first);
    EXPECT_EQ(first.getFVindices(&members), (std::vector<unsigned int>{0, 2}));
    EXPECT_TRUE(members.remove(&first));
    ASSERT_EQ(members.size(), 1);
    EXPECT_EQ(members[0], &second);
    EXPECT_EQ(second.getFVindices(&members), (std::vector<unsigned int>{0}));
    EXPECT_TRUE(first.getFVindices(&members).empty());
    EXPECT_FALSE(first.HasMemberships());
    EXPECT_EQ(members.selectout(0), &second);
    EXPECT_FALSE(second.HasMemberships());
}

} // namespace transm
