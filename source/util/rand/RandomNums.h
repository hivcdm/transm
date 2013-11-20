#pragma once

#include <cstdlib>
#include <vector>
#include "boost/random/mersenne_twister.hpp"
#include "boost/random/normal_distribution.hpp"
#include "boost/random/poisson_distribution.hpp"
#include "boost/math/distributions/beta.hpp"
//#include "IsaacRand.h"

using namespace std;
typedef unsigned long uint32;

class NormalDist {
public :
	double mean;
	double stddev;
};

class LogNormalDist {
public :
	double mu;
	double sigma;
	bool isZeroDistrib;
	double getMean() const;
	LogNormalDist(){isZeroDistrib=false;}
};

class ShiftedLogNormalDist{
public :
	double mu;
	double sigma;
	double shift;
	bool isZeroDistrib;
	double getMean() const;
	ShiftedLogNormalDist(){isZeroDistrib=false;}
};

class BetaDist {
public:
	double alpha;
	double beta;
};
/***
This is a wrapper class that draws numbers from several random number generators
Current number generators:
	MersenneTwister
	ISSAC
***/
class RandomNums {

	//the current seed for this random number generator
	uint32 seed;

	//random number generators
	//seeding the the Mersenne Twister w/ the current time
	boost::mt19937 mtRand;			//Mersenne Twister
	double mtRand_OneOverMaxMult;	//used to generate a number between 0.0 and 1.0 for mtRand
									//division is slower than mult so use 1/mtRand.max()

//	QTIsaac<UINT32> isaac;	//Isaac

public :
	bool chance(const double _probability);
	/***
	given a vector of relative probabilities, returns an integer between 0 and the size of the vector
	@param _indexProbabilities contains relative probabilities that a particular index will be chosen
	@returns a number between 0 and _indexProbabilities.size()
	***/
	int chooseIndex(const vector<double> &_indexProbabilities);

	double rand();						// returns a double between 0 and 1
	uint32 randInt( );      // integer in [0,n] for n < 2^32
	uint32 randInt( const uint32& _max );      // integer in [0,n] for n < 2^32
	uint32 randInt( const uint32& _min, const uint32& _max );      // integer in [min,max] for n < 2^32

	//draws a number from the _normDist
	double randNorm( const NormalDist &_normDist );
	//draws a number from the _normDist, but only returns natural numbers
	//  if we draw a # under 0, then draws from distribution again
	//TODO: Isn't LogNormal more correct here?  Convert the distribution to a log normal distribution?
	unsigned long int randNorm_NaturalNum(const NormalDist &_normDist);

	double randLogNormal(const LogNormalDist &_logNormDist);
	double randShiftedLogNormal(const ShiftedLogNormalDist &_shiftedLogNormDist);
	int randPoisson(double mu);
	double randExponential(double _mean);
	double randBeta (const BetaDist &_betaDist);

	//getters and setters
	uint32 getSeed();
	//reset the generator w/ the current seed
	void reset();
	//reset the generator and use a diff seed
	void reset(unsigned long _seed);

	//constructors
	RandomNums();
	RandomNums(unsigned long _seed);
};

