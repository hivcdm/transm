#include <gtest/gtest.h>

#include <random>
#include <vector>

#include "entitypool/countindex.hpp"

namespace transm {

namespace {

long BruteSum(const std::vector<long> &counts, long first, long last)
{
    long sum = 0;
    for (long i = first; i <= last; i++)
        if (i >= 0 && i < (long)counts.size())
            sum += counts[i];
    return sum;
}

/* The slot the old partner draw picked: walk slots from 0 and subtract counts. */
std::size_t BruteFind(const std::vector<long> &counts, long rank)
{
    for (std::size_t i = 0; i < counts.size(); i++) {
        if (rank < counts[i])
            return i;
        rank -= counts[i];
    }
    return counts.size();
}

void ExpectMatches(const CountIndex &index, const std::vector<long> &counts)
{
    long total = BruteSum(counts, 0, (long)counts.size() - 1);
    ASSERT_EQ(index.Size(), counts.size());
    ASSERT_EQ(index.Total(), total);
    for (std::size_t i = 0; i < counts.size(); i++)
        ASSERT_EQ(index.Get(i), counts[i]);
    for (long rank = 0; rank < total; rank++)
        ASSERT_EQ(index.FindByRank(rank), BruteFind(counts, rank)) << "rank " << rank;
}

} // namespace

TEST(CountIndexTest, StartsEmpty)
{
    CountIndex index(5);
    EXPECT_EQ(index.Total(), 0);
    EXPECT_EQ(index.Sum(0, 4), 0);
    CountIndex none;
    EXPECT_EQ(none.Size(), 0u);
    EXPECT_EQ(none.Sum(0, 10), 0);
}

TEST(CountIndexTest, SumsClampAndTreatBackwardRunsAsEmpty)
{
    CountIndex index;
    index.Assign({3, 0, 2, 5});
    EXPECT_EQ(index.Sum(0, 3), 10);
    EXPECT_EQ(index.Sum(1, 2), 2);
    EXPECT_EQ(index.Sum(2, 2), 2);
    EXPECT_EQ(index.Sum(-7, 0), 3);
    EXPECT_EQ(index.Sum(3, 99), 5);
    EXPECT_EQ(index.Sum(2, 1), 0);
    EXPECT_EQ(index.Sum(-5, -1), 0);
    EXPECT_EQ(index.Sum(4, 9), 0);
}

TEST(CountIndexTest, FindByRankSkipsEmptySlots)
{
    CountIndex index;
    index.Assign({0, 2, 0, 0, 1, 0});
    EXPECT_EQ(index.FindByRank(0), 1u);
    EXPECT_EQ(index.FindByRank(1), 1u);
    EXPECT_EQ(index.FindByRank(2), 4u);
}

/* Every size from 1 to 70 covers the power-of-two edges of the tree. */
TEST(CountIndexTest, AssignAndSetAgreeWithBruteForce)
{
    std::mt19937 rng(42);
    for (std::size_t size = 1; size <= 70; size++) {
        std::uniform_int_distribution<long> count(0, 4);
        std::uniform_int_distribution<std::size_t> slot(0, size - 1);
        std::vector<long> counts(size);
        for (auto &c : counts)
            c = rng() % 3 == 0 ? 0 : count(rng);

        CountIndex assigned;
        assigned.Assign(counts);
        CountIndex built(size);
        for (std::size_t i = 0; i < size; i++)
            built.Set(i, counts[i]);
        ExpectMatches(assigned, counts);
        ExpectMatches(built, counts);

        for (int change = 0; change < 50; change++) {
            std::size_t s = slot(rng);
            counts[s] = count(rng);
            assigned.Set(s, counts[s]);
            long first = (long)slot(rng) - 3;
            long last = first + (long)(rng() % (size + 6));
            ASSERT_EQ(assigned.Sum(first, last), BruteSum(counts, first, last));
        }
        ExpectMatches(assigned, counts);
    }
}

/* The partner draw picks rank Sum(0, first - 1) + pick to land inside [first, last]. */
TEST(CountIndexTest, RankWithinARunMatchesTheOldWalk)
{
    std::mt19937 rng(7);
    std::vector<long> counts(121);
    for (auto &c : counts)
        c = rng() % 4 == 0 ? 0 : (long)(rng() % 6);
    CountIndex index;
    index.Assign(counts);

    for (int trial = 0; trial < 2000; trial++) {
        long first = (long)(rng() % counts.size());
        long last = first + (long)(rng() % 20);
        long inRun = index.Sum(first, last);
        if (inRun == 0)
            continue;
        long pick = (long)(rng() % inRun);

        std::size_t expected = counts.size();
        long remaining = pick;
        for (long i = first; i <= last && i < (long)counts.size(); i++) {
            if (remaining < counts[i]) {
                expected = (std::size_t)i;
                break;
            }
            remaining -= counts[i];
        }
        ASSERT_EQ(index.FindByRank(index.Sum(0, first - 1) + pick), expected);
    }
}

} // namespace transm
