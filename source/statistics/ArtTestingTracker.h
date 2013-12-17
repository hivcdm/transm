#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <tuple>
#include <unordered_map>

#include "../cepac/SimContext.h"
#include "../data/EventParams.h"

class Person;
class Population;

class ArtTestingTracker
{
public:
	ArtTestingTracker();
	~ArtTestingTracker();

	void recordTest(Person *person, bool offered, bool accepted, SimContext::TEST_RESULT result);

	void printArtRolloutOutcomes(std::ostream &_outStream, EventParams &_eventParams, Population *_population);

private:
	std::unordered_map<std::tuple<int, int>, std::string> header;
	int numColumns, numHeaderRows;

	//const char *riskGroupNames[] = {"CSW High Risk", "CSW Low Risk", "Non-CSW High Risk Male", "Non-CSW High Risk Female", "Non-CSW Low Risk Male", "Non-CSW Low Risk Female"};
	//const char *cd4StrataNames[] = {"Very Low", "Low", "Medium Low", "Medium High", "High", "Very High"};

	void setHeader(int column, int row, const std::string &value)
	{
		header.emplace(row, column, value);
		numHeaderRows = std::max<int>(row, numHeaderRows);
		numColumns = std::max<int>(column, numColumns);
	}

	void printHeader(std::ostream &outStream)
	{
		for(int row = 0; row < numHeaderRows; ++row)
		{
			for(int column = 0; column < numColumns; +column)
			{
				std::tuple<int, int> currentPosition(row, column);
				outStream << header.count(currentPosition) ? header[currentPosition] : Constants::TAB;
			}
			outStream << std::endl;
		}
	}

	void buildHeader(const std::vector<boost::tuple<int, int> > &ageRanges, std::ostream &outStream)
	{
		setHeader(0, 0, "ART Rollout Outcomes");
		setHeader(2, 0, "Females");
		setHeader(10, 0, "Males");
		setHeader(20, 0, "CD4 Count");
		setHeader(1, 0, "Non-Sexually Active Population");
		setHeader(2, 0, "Sexually Active Population");
		setHeader(10, 0, "Non-Sexually Active Population");
		setHeader(11, 0, "Sexually Active Population");
		setHeader(15, 0, "Risk Group");
		setHeader(16, 0, "Female");
		setHeader(18, 0, "Male");
		setHeader(19, 0, "Test Result");

		setHeader(1, 2, "All ages");
		for(size_t i = 0; i < ageRanges.size(); ++i)
		{
			std::stringstream rangeString;
			rangeString << ageRanges[i].get<0>() << "-" << ageRanges[i].get<1>();
			setHeader(2, 2, rangeString.str());
		}

		setHeader(1, 2, "All ages");
		for(size_t i = 0; i < ageRanges.size(); ++i)
		{
			std::stringstream rangeString;
			rangeString << ageRanges[i].get<0>() << "-" << ageRanges[i].get<1>();
			setHeader(2, 2, rangeString.str());
		}

		setHeader(20, 2, "Number of Test Offered");
		setHeader(20, 2, "Number of Test Accepted");
	}

	void buildNumTestsHeader()
	{

	}

	void buildNumEligibleHeader()
	{

	}

	void buildNumEnrolledHeader()
	{

	}
};
