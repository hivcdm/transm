#include "RandomNums.h"
#include "../Util.h"
#include <ctime>
//-----------< Begin Constructors >-----------------//
RandomNums::RandomNums()
{
	mtRand.seed(static_cast<uint32_t>(time(nullptr)));
	mtRand_OneOverMaxMult = 1.0 / mtRand.max();
}

RandomNums::RandomNums(unsigned int _seed)
{
	reset(_seed);
	mtRand_OneOverMaxMult = 1.0 / mtRand.max();
}

//-----------< End Constructors >-----------------//


//-----------< Begin rand num functions >-----------------//


int RandomNums::chooseIndex(const std::vector<double> &_indexProbabilities)
{
	assert(_indexProbabilities.size() > 0);
	double choice = rand();	//this is dice roll to see which pool we will draw from
	double cumulativeProb = 0;				//this stores CDF for the current index
	size_t currIndex;	//the index that we are currently considering

	//cycle through vector until the CDF is greater than than what we rolled
	for(currIndex = 0; currIndex < _indexProbabilities.size() - 1; currIndex++)
	{
		cumulativeProb += _indexProbabilities.at(currIndex);

		if(cumulativeProb >= choice)
		{
			break;
		}
	}

	//shouldn't trigger this, but include this here to avoid OutOfBounds exception just in case
	if(currIndex == _indexProbabilities.size())
	{
		currIndex--;
	}

	return static_cast<int>(currIndex);
}

bool RandomNums::chance(double _probability)
{
	//assert(Util::validProbability(_probability));
	if(_probability <= 0)
	{
		return false;
	}

	if(_probability >= 1)
	{
		return true;
	}

	double d = rand();
	return (d <= _probability);
}

double RandomNums::rand()
{
	return mtRand() * mtRand_OneOverMaxMult;
}

uint32_t RandomNums::randInt()
{
	return mtRand();
}

uint32_t RandomNums::randInt(const uint32_t &_max)
{
	return Util::round<uint32_t>(rand() * _max);
}

uint32_t RandomNums::randInt(const uint32_t &_min, const uint32_t &_max)
{
	assert(_max >= _min);
	return _min + randInt(_max - _min);
}

double RandomNums::randExponential(double _mean)
{
	if(_mean == 0)
	{
		return 0;
	}

	return (-log(rand())) / (1 / _mean);
}

//generates a random number using the Masaglia Polar Method
double RandomNums::randNorm(const NormalDist &_normDist)
{
	assert(_normDist.stddev >= 0);
	double x, y, sq = 0;

	if((_normDist.mean == 0) && (_normDist.stddev == 0))
	{
		return 0;
	}

	do
	{
		x = (rand() * 2) - 1;	//a number in [-1,1]
		y = (rand() * 2) - 1;	//a number in [-1,1]
		sq = (x * x + y * y);
	} while(sq >= 1);

	return _normDist.mean + _normDist.stddev * x * sqrt(-2 * log(sq) / sq);
}

double RandomNums::randLogNormal(const LogNormalDist &_logNormDist)
{
	if(_logNormDist.isZeroDistrib)
	{
		return 0;
	}

	NormalDist normDist;
	normDist.mean = _logNormDist.mu;
	normDist.stddev = _logNormDist.sigma;
	return exp(randNorm(normDist));
}

double RandomNums::randShiftedLogNormal(const ShiftedLogNormalDist &_shiftedLogNormDist)
{
	if(_shiftedLogNormDist.isZeroDistrib)
	{
		return 0;
	}

	LogNormalDist logNormDist;
	logNormDist.mu = _shiftedLogNormDist.mu;
	logNormDist.sigma = _shiftedLogNormDist.sigma;
	return randLogNormal(logNormDist) + _shiftedLogNormDist.shift;
}

double RandomNums::randBeta(const BetaDist &_betaDist)
{
	assert(_betaDist.alpha > 0);
	assert(_betaDist.beta > 0);
	boost::math::beta_distribution<> betaDist(_betaDist.alpha, _betaDist.beta);
	return boost::math::quantile(betaDist, rand());
}

double LogNormalDist::getMean() const
{
	if(isZeroDistrib)
	{
		return 0;
	}

	return exp(mu + ((sigma * sigma) / 2));
}

double ShiftedLogNormalDist::getMean() const
{
	if(isZeroDistrib)
	{
		return 0;
	}

	return exp(mu + ((sigma * sigma) / 2)) + shift;
}

unsigned int RandomNums::randNorm_NaturalNum(const NormalDist &_normDist)
{
	assert(_normDist.stddev >= 0);

	if((_normDist.mean == 0) && (_normDist.stddev == 0))
	{
		return 0;
	}

	int tries = 1000;

	do
	{
		double rd = RandomNums::randNorm(_normDist);

		if(rd >= 0)
		{
			//Return a double as an int will always return the floor of the double.  We want to round to the nearest integer.
			//Adding 0.5 assures that the floor of the new number will be the nearest integer of the old number
			rd = rd + 0.5;
			return static_cast<unsigned int>(rd);
		}

		tries--;
	} while(tries > 0);

	std::cerr <<
		"RandomNums::randNorm_NaturalNum: We could not get a number greater or equal to zero after 1000 tries. Check your distribution N("
		<< _normDist.mean << "," << _normDist.stddev << ").  Function will return 0." << std::endl;
	//Util::exitWithPrompt(-1);
	return 0;
}

//adapted from Charles Stanton's 'Java Demos for Probability and Statistics site'
//http://www.math.csusb.edu/faculty/stanton/m262/
int RandomNums::randPoisson(double _mu)
{
	assert(_mu >= 0);

	if(_mu == 0)
	{
		return 0;
	}

	double eMu = exp(-_mu);
	double product = 1.0;
	int count = -1;

	do
	{
		product *= rand();
		count++;
	} while(product >= eMu);

	return count;
}

//-----------< End rand num functions >-----------------//

//-----------< Begin Getters and Setters >---------------//
unsigned int RandomNums::getSeed()
{
	return seed;
}

void RandomNums::reset()
{
	reset(seed);
}

void RandomNums::reset(unsigned int _seed)
{
	seed = _seed;
	mtRand.seed((boost::mt19937::result_type) _seed);
	//	isaac = QTIsaac<UINT32>(mtRand.randInt(), mtRand.randInt(),mtRand.randInt());	//Isaac
}
//-----------< End Getters and Setters >---------------//