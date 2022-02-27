#ifndef TABULAROUTPUT_HPP
#define TABULAROUTPUT_HPP

#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <boost/functional/hash.hpp>

namespace transm {

using Coordinate = std::pair < int, int > ;

} // namespace transm

namespace std {

template<>
struct hash<transm::Coordinate>
{
    std::size_t operator()(transm::Coordinate const &e) const
	{
		std::size_t seed = 0;
		boost::hash_combine(seed, e.first);
		boost::hash_combine(seed, e.second);
		return seed;
	}
};

template<>
struct equal_to<transm::Coordinate>
{
    bool operator()(transm::Coordinate const &x, transm::Coordinate const &y) const
	{
		return x.first == y.first && x.second == y.second;
	}
};

} // namespace std

namespace transm {

using SparseTable = std::unordered_map<Coordinate, std::string>;

class TabularOutput
{
public:
	TabularOutput();
	~TabularOutput();

	void PrintHeader(std::ostream &outStream);
	void PrintRow(std::ostream &outStream, bool clearAfterWriting = true);
	void SetHeaderCell(int column, int row, const std::string &value);

	template<typename T>
	void PushElement(const T &value)
	{
		std::stringstream elementStream;
		elementStream << value;
		assert(currentColumn < numColumns);
		currentRow[currentColumn++] = elementStream.str();
	}

	void PushEmptyElement();
	void ClearRow();

private:
	int numHeaderRows;
	int numColumns;
	int currentColumn;
	SparseTable header;
	std::vector<std::string> currentRow;
};

} // namespace transm

#endif /* TABULAROUTPUT_HPP */