#pragma once

#include <cstdint>
#include <cstdlib>
#include <vector>
#include <boost/math/distributions/beta.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/normal_distribution.hpp>
#include <boost/random/poisson_distribution.hpp>
#include <pugixml.hpp>

struct NormalDist
{
    double mean;
    double stddev;
};

struct LogNormalDist
{
	static LogNormalDist FromNormal(NormalDist dist)
	{
		LogNormalDist result;

		if(dist.mean <= 0)
		{
			result.mu = 0;
			result.sigma = 0;
			result.isZeroDistrib = true;
		}
		else
		{
			result.mu = log(dist.mean) - 0.5 * log(1 + (dist.stddev * dist.stddev) / (dist.mean * dist.mean));
			result.sigma = sqrt(log(1 + (dist.stddev * dist.stddev) / (dist.mean * dist.mean)));
		}

		return result;
	}

	double mu;
	double sigma;
	bool isZeroDistrib = false;
	double getMean() const;
};

struct ShiftedLogNormalDist
{
	static ShiftedLogNormalDist FromShiftedNormal(NormalDist dist, double shift)
	{
		ShiftedLogNormalDist result;

		if(dist.mean <= 0)
		{
			result.mu = 0;
			result.sigma = 0;
			result.shift = 0;
			result.isZeroDistrib = true;
		}
		else
		{
			result.shift = shift;
			result.mu = log(dist.mean - shift) - 0.5 * log(1 + (dist.stddev * dist.stddev) / ((dist.mean - shift) * (dist.mean - shift)));
			result.sigma = sqrt(log(1 + (dist.stddev * dist.stddev) / ((dist.mean - shift) * (dist.mean - shift))));
		}

		return result;
	}

	double mu;
	double sigma;
	double shift;
	bool isZeroDistrib = false;
	double getMean() const;
};

struct BetaDist
{
    static BetaDist FromNormal(NormalDist dist) {
	BetaDist result;

	double alpha, beta;
	double sampleSize = dist.mean * (1 - dist.mean) /
	    (dist.stddev * dist.stddev) - 1;
	result.alpha = dist.mean * sampleSize;
	result.beta = (1 - dist.mean) * sampleSize;

	return result;
    }

    static NormalDist ToNormal(BetaDist dist) {
	NormalDist result;

	result.mean = dist.alpha / (dist.alpha + dist.beta);
	result.stddev = sqrt((dist.alpha*dist.beta)/
			     (pow(dist.alpha+dist.beta,2)*(dist.alpha+dist.beta+1)));

	return result;
    }

    double alpha;
    double beta;
};

/***
This is a wrapper class that draws numbers from several random number generators
Current number generators:
MersenneTwister
ISSAC
***/
class RandomNumberGenerator
{
public:
	bool chance(const double _probability);
	/***
	given a vector of relative probabilities, returns an integer between 0 and the size of the vector
	@param _indexProbabilities contains relative probabilities that a particular index will be chosen
	@returns a number between 0 and _indexProbabilities.size()
	***/
	int chooseIndex(const std::vector<double> &_indexProbabilities);

	double rand();						// returns a double between 0 and 1
	uint32_t randInt();       // integer in [0,n] for n < 2^32
	uint32_t randInt(const uint32_t &_max);        // integer in [0,n] for n < 2^32
	uint32_t randInt(const uint32_t &_min, const uint32_t &_max);        // integer in [min,max] for n < 2^32

	//draws a number from the _normDist
	double randNorm(const NormalDist &_normDist);
	//draws a number from the _normDist, but only returns natural numbers
	//  if we draw a # under 0, then draws from distribution again
	//TODO: Isn't LogNormal more correct here?  Convert the distribution to a log normal distribution?
	unsigned int randNorm_NaturalNum(const NormalDist &_normDist);

	double randLogNormal(const LogNormalDist &_logNormDist);
	double randShiftedLogNormal(const ShiftedLogNormalDist &_shiftedLogNormDist);
	int randPoisson(double mu);
	double randExponential(double _mean);
	double randBeta(const BetaDist &_betaDist);

	//getters and setters
	uint32_t getSeed();
	//reset the generator w/ the current seed
	void reset();
	//reset the generator and use a diff seed
	void reset(unsigned int _seed);

	//constructors
	RandomNumberGenerator();
	RandomNumberGenerator(unsigned int _seed);

private:
	//the current seed for this random number generator
	uint32_t seed;

	//random number generators
	//seeding the the Mersenne Twister w/ the current time
	boost::mt19937 mtRand;			//Mersenne Twister
	double mtRand_OneOverMaxMult;	//used to generate a number between 0.0 and 1.0 for mtRand
	//division is slower than mult so use 1/mtRand.max()

	//	QTIsaac<UINT32> isaac;	//Isaac
};
