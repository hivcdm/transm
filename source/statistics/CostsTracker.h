/*
 * CostsTracker.h
 *
 *  Created on: Aug 20, 2010
 *      Author: errhode
 */

#ifndef COSTSTRACKER_H_
#define COSTSTRACKER_H_

#include <fstream>
#include <vector>

class CostsTracker{

public:
	//If editing this, also edit CostSourcesStr
	enum CostSources {
		CIRCUMCISION,
		CONDOMS,
		//PrEP,
		CEPAC,
		EndCostSources
	};

	static const char CostSourcesStr[CostsTracker::EndCostSources][24];

	class MonthlyCosts{
	public:
		MonthlyCosts();

		double Costs[EndCostSources];

		double getTotalCosts();
	};

private:
	std::vector<MonthlyCosts*> allCosts;

	double totalCosts[CostsTracker::EndCostSources];

	//Here's where the cost for each circumcision, each condom, etc is stored
	//double costPerSource[CostsTracker::EndCostSources];

	//Helper function for printCosts
	void printCostHeaders(std::ostream &_outStream);

public:
	/* Constructor and destructor*/
	CostsTracker();
	~CostsTracker();

	//Total costs for each source
	double getTotalCostsPerSource(CostsTracker::CostSources _costSource);

	//Total costs for each time step
	double getTotalCostsPerTime(long _time);

	//Total costs of all sources
	double getTotalCosts();

	//Add a cost
	void addCost(double _cost, CostsTracker::CostSources _costSource, long _currTime);

	//print all costs (call at end of simulation)
	void printCosts(std::ostream &_outStream);

};


#endif /* COSTSTRACKER_H_ */
