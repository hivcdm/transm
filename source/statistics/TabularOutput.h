#pragma once

#include <unordered_map>
#include <utility>
#include <boost/functional/hash.hpp>
#include <fstream>

typedef std::pair<int, int> Coordinate;

namespace std {

template<>
struct hash<Coordinate>
{
	std::size_t operator()(Coordinate const& e) const
	{
		std::size_t seed = 0;
		boost::hash_combine(seed, e.first);
		boost::hash_combine(seed, e.second);
		return seed;
	}
};

template<>
struct equal_to<Coordinate>
{
	bool operator()(Coordinate const& x, Coordinate const& y) const
	{
		return x.first == y.first && x.second == y.second;
	}
};

}

typedef std::unordered_map<Coordinate, std::string> SparseTable;

class TabularOutput
{
public:
	TabularOutput();
	~TabularOutput();

	void PrintHeader(std::ostream &outStream);
	void PrintRow(std::ostream &outStream, bool clearAfterWriting = true);
	void SetHeaderCell(int column, int row, const std::string &value);
	void PushElement(const std::string &element);
	void PushEmptyElement();
	void ClearRow();

private:
	int numHeaderRows;
	int numColumns;
	int currentColumn;
	SparseTable header;
	std::vector<std::string> currentRow;
};