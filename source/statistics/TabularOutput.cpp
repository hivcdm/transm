#include "TabularOutput.h"
#include "../Constants.h"

TabularOutput::TabularOutput() :
    numHeaderRows(0),
	numColumns(0),
	currentColumn(0)
{

}

TabularOutput::~TabularOutput()
{

}

void TabularOutput::PrintHeader(std::ostream &outStream)
{
	for(int row = 0; row < numHeaderRows; ++row)
	{
		for(int column = 0; column < numColumns; ++column)
		{
			Coordinate currentPosition(row, column);
			outStream << header.count(currentPosition) ? header[currentPosition] : Constants::TAB;
		}

		outStream << std::endl;
	}
}

void TabularOutput::PrintRow(std::ostream &outStream, bool clearAfterWriting)
{
	assert(numColumns > 0);

	for(size_t i = 0; i < numColumns; i++)
	{
		outStream << currentRow[i];
		if(i < numColumns - 1)
		{
			outStream << Constants::TAB;
		}
	}

	outStream << std::endl;

	if(clearAfterWriting)
	{
		ClearRow();
	}
}

void TabularOutput::SetHeaderCell(int column, int row, const std::string &value)
{
	assert(column > 0);
	assert(row > 0);

	numHeaderRows = std::max<int>(row, numHeaderRows);
	numColumns = std::max<int>(column, numColumns);

	currentRow.resize(numColumns);

	header.emplace(Coordinate(row, column), value);
}

void TabularOutput::PushElement(const std::string &element)
{
	currentRow[currentColumn++] = element;
	assert(currentColumn <= numColumns);
}

void TabularOutput::PushEmptyElement()
{
	++currentColumn;
	assert(currentColumn <= numColumns);
}

void TabularOutput::ClearRow()
{
	assert(numColumns > 0);

	currentColumn = 0;
	std::fill(currentRow.begin(), currentRow.end(), "");
}
