#include <sstream>

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
	for(int row = 1; row < numHeaderRows + 1; ++row)
	{
		for(int column = 1; column < numColumns + 1; ++column)
		{
			Coordinate currentPosition(row, column);

			if(header.count(currentPosition))
			{
				outStream << header[currentPosition];
			}

			if(column < numColumns)
			{
				outStream << Constants::TAB;
			}
		}

		outStream << std::endl;
	}
}

void TabularOutput::PrintRow(std::ostream &outStream, bool clearAfterWriting)
{
	assert(numColumns > 0);

	for(int i = 0; i < numColumns; i++)
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

void TabularOutput::SetHeaderCell(std::size_t column, std::size_t row, const std::string &value)
{
	assert(column > 0);
	assert(row > 0);
	numHeaderRows = std::max<size_t>(row, numHeaderRows);
	numColumns = std::max<size_t>(column, numColumns);
	currentRow.resize(numColumns);
	header[Coordinate(row, column)] = value;
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
