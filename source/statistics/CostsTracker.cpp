/*
 * CostsTracker.cpp
 *
 *  Created on: Aug 20, 2010
 *      Author: errhode
 */
#include "CostsTracker.h"
#include <vector>

const char CostsTracker::CostSourcesStr[CostsTracker::EndCostSources][24] =
{
	"Circumcision",
	"Condoms",
	// "Prep",
	"CEPAC",
};

CostsTracker::MonthlyCosts::MonthlyCosts()
{
	//Initialize all costs to 0
	for(int i = 0; i < CostsTracker::EndCostSources; i++)
	{
		this->Costs[i] = 0;
	}
}

double CostsTracker::MonthlyCosts::getTotalCosts()
{
	double totalMonthlyCost = 0;

	for(int i = 0; i < CostsTracker::EndCostSources; i++)
	{
		totalMonthlyCost += this->Costs[i];
	}

	return totalMonthlyCost;
}

CostsTracker::CostsTracker()
{
	//Initialize all costs to 0
	for(int i = 0; i < CostsTracker::EndCostSources; i++)
	{
		this->totalCosts[i] = 0;
	}
}

CostsTracker::~CostsTracker()
{
	//Delete all of the MonthlyCosts
	while(this->allCosts.size() > 0)
	{
		MonthlyCosts *mCosts = this->allCosts.back();
		this->allCosts.pop_back();
		delete mCosts;
	}
}

//Total costs for each source
double CostsTracker::getTotalCostsPerSource(CostsTracker::CostSources _costSource)
{
	return this->totalCosts[_costSource];
}

//Total costs for each time step
double CostsTracker::getTotalCostsPerTime(int _time)
{
	if(_time >= static_cast<int>(allCosts.size()))
	{
		//Default to 0 if time hasn't occurred
		return 0;
	}
	else
	{
		return allCosts.at(_time)->getTotalCosts();
	}
}

//Total costs of all sources
double CostsTracker::getTotalCosts()
{
	double totalCost = 0;

	for(int i = 0; i < CostsTracker::EndCostSources; i++)
	{
		totalCost += this->totalCosts[i];
	}

	return totalCost;
}

//Add a cost
void CostsTracker::addCost(double _cost, CostsTracker::CostSources _costSource, int _currTime)
{
	//Check to see if a MonthlyCost already exists for this time
	while(static_cast<size_t>(_currTime) >= this->allCosts.size())
	{
		//Else create one
		MonthlyCosts *monthlyCosts = new MonthlyCosts();
		this->allCosts.push_back(monthlyCosts);
	}

	//Add the cost to the monthlyCost
	this->allCosts.at(_currTime)->Costs[_costSource] += _cost;
	//Add the cost to the totalCosts
	this->totalCosts[_costSource] += _cost;
}

//print all costs (call at end of simulation)
void CostsTracker::printCosts(std::ostream &_outStream)
{
	this->printCostHeaders(_outStream);

	//Print out each month
	for(size_t month = 0; month < this->allCosts.size(); month++)
	{
		//Print time
		_outStream << month << "\t";

		//Print each monthly cost by source
		for(int source = 0; source < CostsTracker::EndCostSources; source++)
		{
			_outStream << this->allCosts.at(month)->Costs[source] << "\t";
		}

		//Print total monthly cost
		_outStream << this->getTotalCostsPerTime(month) << std::endl;
	}

	//Print out the totals
	_outStream << "Total\t";

	//By source
	for(int source = 0; source < CostsTracker::EndCostSources; source++)
	{
		_outStream << this->totalCosts[source] << "\t";
	}

	//Overall total
	_outStream << this->getTotalCosts() << std::endl;
}

//Helper function for printCosts
void CostsTracker::printCostHeaders(std::ostream &_outStream)
{
	_outStream << "Costs" << std::endl;
	_outStream << "Month\t";

	for(int i = 0; i < CostsTracker::EndCostSources; i++)
	{
		_outStream << this->CostSourcesStr[i] << "\t";
	}

	_outStream << "Total" << std::endl;
}
