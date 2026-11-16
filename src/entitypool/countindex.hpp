#ifndef COUNTINDEX_HPP
#define COUNTINDEX_HPP

#include <cassert>
#include <cstddef>
#include <vector>

namespace transm {

/**
 * Non-negative counts in a row of slots (a Fenwick tree), with sums over any run of
 * slots and lookup of the slot holding the k-th item, both in O(log n).
 *
 * BucketSexualMixing keeps one per risk level, with a slot per month of age, so a
 * partner draw no longer walks every age bucket to count and pick candidates. */
class CountIndex
{
public:
	explicit CountIndex(std::size_t size = 0) { Reset(size); }

	/** Sets every slot to zero. */
	void Reset(std::size_t size)
	{
		counts_.assign(size, 0);
		tree_.assign(size + 1, 0);
		total_ = 0;
	}

	/** Replaces all slots with counts, in O(n). */
	void Assign(const std::vector<long> &counts)
	{
		counts_ = counts;
		tree_.assign(counts.size() + 1, 0);
		total_ = 0;

		for(std::size_t i = 0; i < counts.size(); i++)
		{
			assert(counts[i] >= 0);
			total_ += counts[i];
			std::size_t node = i + 1;
			tree_[node] += counts[i];
			std::size_t parent = node + (node & (~node + 1));

			if(parent < tree_.size())
				tree_[parent] += tree_[node];
		}
	}

	std::size_t Size() const { return counts_.size(); }

	long Get(std::size_t slot) const { return counts_[slot]; }

	void Set(std::size_t slot, long count)
	{
		assert(count >= 0);
		long change = count - counts_[slot];

		if(change == 0)
			return;

		counts_[slot] = count;
		total_ += change;

		for(std::size_t node = slot + 1; node < tree_.size(); node += node & (~node + 1))
			tree_[node] += change;
	}

	long Total() const { return total_; }

	/** Sum of slots [first, last]; zero when the run is empty. Out-of-range ends are clamped. */
	long Sum(long first, long last) const
	{
		if(first < 0)
			first = 0;

		if(last >= (long)counts_.size())
			last = (long)counts_.size() - 1;

		if(last < first)
			return 0;

		return Prefix((std::size_t)last + 1) - Prefix((std::size_t)first);
	}

	/**
	 * The slot holding item number rank (from 0) when items are laid out slot by slot:
	 * the smallest slot whose running total exceeds rank. Requires 0 <= rank < Total(). */
	std::size_t FindByRank(long rank) const
	{
		assert(rank >= 0 && rank < total_);
		std::size_t node = 0;
		std::size_t step = 1;

		while(step * 2 < tree_.size())
			step *= 2;

		for(; step > 0; step /= 2)
		{
			std::size_t next = node + step;

			if(next < tree_.size() && tree_[next] <= rank)
			{
				node = next;
				rank -= tree_[next];
			}
		}

		return node;
	}

private:
	/** Sum of the first n slots. */
	long Prefix(std::size_t n) const
	{
		long sum = 0;

		for(; n > 0; n -= n & (~n + 1))
			sum += tree_[n];

		return sum;
	}

	std::vector<long> counts_;
	std::vector<long> tree_;
	long total_;
};

} // namespace transm

#endif /* COUNTINDEX_HPP */
