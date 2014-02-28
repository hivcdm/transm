#pragma once

#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/functional/hash.hpp>

typedef std::pair<std::size_t, std::size_t> Coordinate;

namespace std
{

template<>
struct hash<Coordinate>
{
	std::size_t operator()(Coordinate const &e) const
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
	bool operator()(Coordinate const &x, Coordinate const &y) const
	{
		return x.first == y.first && x.second == y.second;
	}
};

}

class TabularOutput
{
	typedef std::unordered_map<Coordinate, std::string> SparseTable;

public:
	TabularOutput();
	~TabularOutput();

	void PrintHeader(std::ostream &outStream);

	void PrintRow(std::ostream &outStream, bool clearAfterWriting = true);

	void SetHeaderCell(std::size_t column, std::size_t row, const std::string &value);

	template<typename T>
	void PushElement(T element)
	{
		std::stringstream elementStream;
		elementStream << element;
		PushString(elementStream.str());
	}

	void PushString(const std::string &string);

	void PushEmptyElement();

	void ClearRow();

private:
	std::size_t numHeaderRows;
	std::size_t numColumns;
	std::size_t currentColumn;
	SparseTable header;
	std::vector<std::string> currentRow;
};
