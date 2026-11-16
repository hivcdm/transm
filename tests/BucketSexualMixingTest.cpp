#include <gtest/gtest.h>

#include <cstdint>
#include <memory>
#include <vector>

#include "entities/female.hpp"
#include "entitypool/bucketsexualmixing.hpp"
#include "utility/randomnumbergenerator.hpp"

namespace transm {

namespace {

class MixingTestPerson : public Female
{
public:
    using Female::Female;

    void SetRisk(RiskLevel level) { risk = level; }
};

/* FNV-1a, enough to notice any change in a long sequence of values. */
class Fingerprint
{
public:
    void Add(std::uint64_t value)
    {
        for (int byte = 0; byte < 8; byte++) {
            hash_ ^= (value >> (8 * byte)) & 0xff;
            hash_ *= 1099511628211ULL;
        }
    }

    std::uint64_t Value() const { return hash_; }

private:
    std::uint64_t hash_ = 14695981039346656037ULL;
};

/* Ages 15 to 25. Nobody starts between 18 and 22, so draws aimed at that gap
 * exercise the widening search. */
const Age MinAge(15, 0);
const Age MaxAge(25, 0);

struct Scenario
{
    EventParams events;
    Female::SubPopParams params;
    PrepParameters prep;
    DemographicProfile profile;
    std::vector<std::unique_ptr<MixingTestPerson>> people;
    std::vector<int> state; /* 1 in the bucket, 0 removed, -1 aged out */
    std::unique_ptr<BucketSexualMixing> bucket;

    Scenario()
    {
        profile.set(DemographicProfile::Demographic::Gender,
                    (std::size_t)DemographicProfile::Gender::Female);
        std::map<SexualPartnership::Type, double> assort = {
            {SexualPartnership::Type::Steady, 0.7},
            {SexualPartnership::Type::Regular, 0.4},
            {SexualPartnership::Type::Casual, 0.0},
            {SexualPartnership::Type::Csw, 1.0}};
        bucket.reset(new BucketSexualMixing(profile.getProfileID(), nullptr, 0, MinAge, MaxAge, assort));
    }

    ~Scenario()
    {
        /* The bucket holds pointers to the people, so it goes first. */
        bucket.reset();
    }

    int Add(Age age, RiskLevel risk)
    {
        people.emplace_back(new MixingTestPerson(events, age, profile, 0, params, prep));
        people.back()->SetRisk(risk);
        state.push_back(bucket->insert(people.back().get()) ? 1 : 0);
        return (int)people.size() - 1;
    }

    int IndexOf(const Entity *person) const
    {
        for (std::size_t i = 0; i < people.size(); i++)
            if (people[i].get() == person)
                return (int)i;
        return -1;
    }

    int PickWithState(RandomNumberGenerator &rng, int wanted) const
    {
        std::vector<int> candidates;
        for (std::size_t i = 0; i < state.size(); i++)
            if (state[i] == wanted)
                candidates.push_back((int)i);
        if (candidates.empty())
            return -1;
        return candidates[rng.randInt(0, (uint32_t)candidates.size() - 1)];
    }
};

/* Drives a bucket through a long seeded sequence of draws, removals, inserts and
 * monthly ageing, and fingerprints every person drawn and every count read. */
std::uint64_t RunMixingSequence(std::uint32_t seed)
{
    Scenario s;
    RandomNumberGenerator setup(seed);
    RandomNumberGenerator ops(seed + 1);
    RandomNumberGenerator draws(seed + 2);
    Fingerprint fingerprint;

    for (int i = 0; i < 400; i++) {
        int months = (int)setup.randInt(0, 3 * 12);
        Age age = setup.chance(0.5) ? MinAge + TimeSpan(0, months) : MaxAge - TimeSpan(0, months);
        s.Add(age, setup.chance(0.3) ? RiskLevel::HIGH : RiskLevel::LOW);
    }

    for (int step = 0; step < 20000; step++) {
        double roll = ops.rand();
        auto type = (SexualPartnership::Type)ops.randInt(0, (uint32_t)SexualPartnership::Type::Last - 1);
        RiskLevel risk = ops.chance(0.5) ? RiskLevel::HIGH : RiskLevel::LOW;

        if (roll < 0.70) {
            /* A window that may start below the bucket and may be empty. */
            int lower = (int)ops.randInt(0, 30 * 12);
            int width = (int)ops.randInt(0, 24);
            bool remove = ops.chance(0.2);
            Entity *drawn = s.bucket->drawMemberInAgeRange(draws, Age::from_months(lower),
                                                           Age::from_months(lower + width), risk, type, remove);
            int index = s.IndexOf(drawn);
            fingerprint.Add(1);
            fingerprint.Add((std::uint64_t)(index + 1));
            if (remove && index >= 0)
                s.state[index] = 0;
        } else if (roll < 0.75) {
            Entity *drawn = s.bucket->drawMember(draws, type, ops.chance(0.5));
            int index = s.IndexOf(drawn);
            fingerprint.Add(2);
            fingerprint.Add((std::uint64_t)(index + 1));
            if (index >= 0 && !s.bucket->exists(drawn))
                s.state[index] = 0;
        } else if (roll < 0.85) {
            int index = s.PickWithState(ops, 0);
            if (index >= 0 && s.people[index]->getAge() <= MaxAge)
                s.state[index] = s.bucket->insert(s.people[index].get()) ? 1 : 0;
            fingerprint.Add(3);
            fingerprint.Add((std::uint64_t)(index + 1));
        } else if (roll < 0.93) {
            int index = s.PickWithState(ops, 1);
            if (index >= 0 && s.bucket->erase(s.people[index].get()))
                s.state[index] = 0;
            fingerprint.Add(4);
            fingerprint.Add((std::uint64_t)(index + 1));
        } else if (roll < 0.97) {
            int index = s.Add(MinAge, ops.chance(0.3) ? RiskLevel::HIGH : RiskLevel::LOW);
            fingerprint.Add(5);
            fingerprint.Add((std::uint64_t)index);
        } else {
            for (Entity *expired : s.bucket->ageOneTimeStep()) {
                int index = s.IndexOf(expired);
                s.state[index] = -1;
                fingerprint.Add(6);
                fingerprint.Add((std::uint64_t)index);
            }
            /* People outside the bucket do not age with it; age them too. */
            for (std::size_t i = 0; i < s.state.size(); i++)
                if (s.state[i] == 0)
                    s.people[i]->ageOneTimeUnit();
        }

        if (step % 100 == 0) {
            fingerprint.Add(s.bucket->size());
            fingerprint.Add(s.bucket->sizeRandom());
            fingerprint.Add(s.bucket->sizeRisk(RiskLevel::LOW));
            fingerprint.Add(s.bucket->sizeRisk(RiskLevel::HIGH));
            fingerprint.Add(s.bucket->sizeByAge(Age(18, 0), Age(22, 0)));
        }
    }

    return fingerprint.Value();
}

} // namespace

/* The expected values were recorded from the linear-scan implementation on
 * develop before the partner draw was indexed. Any change to which person is
 * drawn, to the random numbers consumed, or to the counts changes them. */
TEST(BucketSexualMixingTest, DrawSequenceMatchesLinearScanImplementation)
{
    EXPECT_EQ(RunMixingSequence(11), 32017235737798244ULL);
    EXPECT_EQ(RunMixingSequence(2024), 14866555337071737514ULL);
    EXPECT_EQ(RunMixingSequence(36291434), 11938410000401853062ULL);
}

TEST(BucketSexualMixingTest, WidensWindowUntilSomeoneCanBeDrawn)
{
    Scenario s;
    RandomNumberGenerator rng(5);
    int only = s.Add(Age(24, 0), RiskLevel::LOW);

    Entity *drawn = s.bucket->drawMemberInAgeRange(rng, Age(16, 0), Age(16, 1), RiskLevel::LOW,
                                                   SexualPartnership::Type::Casual, false);
    EXPECT_EQ(drawn, s.people[only].get());
    EXPECT_TRUE(s.bucket->exists(drawn));

    drawn = s.bucket->drawMemberInAgeRange(rng, Age(16, 0), Age(16, 1), RiskLevel::LOW,
                                           SexualPartnership::Type::Casual, true);
    EXPECT_EQ(drawn, s.people[only].get());
    EXPECT_FALSE(s.bucket->exists(drawn));
    EXPECT_EQ(s.bucket->drawMemberInAgeRange(rng, Age(16, 0), Age(16, 1), RiskLevel::LOW,
                                             SexualPartnership::Type::Casual, false), nullptr);
}

TEST(BucketSexualMixingTest, AssortativeDrawOnlyReturnsTheRequestedRisk)
{
    Scenario s;
    RandomNumberGenerator rng(9);
    for (int i = 0; i < 20; i++)
        s.Add(Age(20, i % 12), i % 4 == 0 ? RiskLevel::HIGH : RiskLevel::LOW);

    /* Csw has assortativeness 1, so every draw comes from the requested risk. */
    for (int i = 0; i < 200; i++) {
        Entity *drawn = s.bucket->drawMemberInAgeRange(rng, MinAge, MaxAge, RiskLevel::HIGH,
                                                       SexualPartnership::Type::Csw, false);
        ASSERT_NE(drawn, nullptr);
        EXPECT_EQ(drawn->getRiskLevel(), RiskLevel::HIGH);
    }
}

} // namespace transm
