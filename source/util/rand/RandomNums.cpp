#include "RandomNums.h"
#include "../Util.h"
#include <ctime>
//-----------< Begin Constructors >-----------------//
RandomNums::RandomNums() {
	this->mtRand.seed(time(NULL));
	this->mtRand_OneOverMaxMult = 1.0/mtRand.max();
}

RandomNums::RandomNums(unsigned long _seed) {
	this->reset(_seed);
	this->mtRand_OneOverMaxMult = 1.0/mtRand.max();
}

//-----------< End Constructors >-----------------//


//-----------< Begin rand num functions >-----------------//


int RandomNums::chooseIndex(const vector<double> &_indexProbabilities) {
	assert (_indexProbabilities.size() > 0);

	double choice = this->rand();	//this is dice roll to see which pool we will draw from
	double cumulativeProb = 0;				//this stores CDF for the current index
	int currIndex;	//the index that we are currently considering

	//cycle through vector until the CDF is greater than than what we rolled
	for(currIndex = 0; currIndex < _indexProbabilities.size()-1; currIndex++ ) {
		cumulativeProb += _indexProbabilities.at(currIndex);
		if(cumulativeProb >= choice )
			break;
	}

	//shouldn't trigger this, but include this here to avoid OutOfBounds exception just in case
	if ( currIndex == _indexProbabilities.size()) {
		currIndex--;
	}

	return currIndex;
}

bool RandomNums::chance(double _probability) {
	//assert(Util::validProbability(_probability));
	if(_probability <= 0) return false;
	if(_probability >= 1) return true;

	double d = this->rand();
	return (d <= _probability);
}

double RandomNums::rand() {
	return mtRand()*mtRand_OneOverMaxMult;
}

uint32 RandomNums::randInt() {
	return mtRand();
}

uint32 RandomNums::randInt( const uint32& _max ) {
	return Util::round(this->rand() * _max);
}

uint32 RandomNums::randInt( const uint32& _min, const uint32& _max ) {

	assert (_max >= _min);
	const uint32 diff = _max-_min;
	return _min + this->randInt(diff);
}

double RandomNums::randExponential(double _mean) {
	if(_mean == 0) return 0;
	return (-log(this->rand()))/ (1/_mean);
}

//generates a random number using the Masaglia Polar Method
double RandomNums::randNorm( const NormalDist &_normDist ) {
	assert ( _normDist.stddev >= 0);

	double x,y,sq = 0;

	if( (_normDist.mean == 0) && (_normDist.stddev == 0)) return 0;
	do {
		x = (this->rand() * 2) - 1;	//a number in [-1,1]
		y = (this->rand() * 2) - 1;	//a number in [-1,1]
		sq = (x*x + y*y);
	} while ( sq >= 1 );

	return _normDist.mean + _normDist.stddev* x*sqrt( -2 * log(sq)/sq) ;
}

double RandomNums::randLogNormal(const LogNormalDist &_logNormDist) {
	if(_logNormDist.isZeroDistrib)
		return 0;
	NormalDist normDist;
	normDist.mean = _logNormDist.mu;
	normDist.stddev = _logNormDist.sigma;

	return exp(this->randNorm(normDist));
}

double RandomNums::randShiftedLogNormal(const ShiftedLogNormalDist &_shiftedLogNormDist){
	if(_shiftedLogNormDist.isZeroDistrib)
		return 0;
	LogNormalDist logNormDist;
	logNormDist.mu = _shiftedLogNormDist.mu;
	logNormDist.sigma = _shiftedLogNormDist.sigma;

	return this->randLogNormal(logNormDist)+_shiftedLogNormDist.shift;
}

double RandomNums::randBeta(const BetaDist &_betaDist){
	assert(_betaDist.alpha>0);
	assert(_betaDist.beta>0);

	boost::math::beta_distribution<> betaDist(_betaDist.alpha,_betaDist.beta);
	return boost::math::quantile(betaDist, this->rand());
}

double LogNormalDist::getMean() const{
	if(isZeroDistrib)
		return 0;
	return exp(mu + ((sigma*sigma)/2));
}

double ShiftedLogNormalDist::getMean() const{
	if(isZeroDistrib)
		return 0;
	return exp(mu+((sigma*sigma)/2))+shift;
}

unsigned long int RandomNums::randNorm_NaturalNum(const NormalDist &_normDist){
	assert ( _normDist.stddev >= 0);

	if( (_normDist.mean == 0) && (_normDist.stddev == 0)) return 0;

	int tries = 1000;
	do {
		double rd = RandomNums::randNorm(_normDist);
		if(rd >= 0){
			//Return a double as an int will always return the floor of the double.  We want to round to the nearest integer.
			//Adding 0.5 assures that the floor of the new number will be the nearest integer of the old number
			rd = rd + 0.5;
			return rd;
		}
		tries--;
	} while(tries > 0);

	cerr << "RandomNums::randNorm_NaturalNum: We could not get a number greater or equal to zero after 1000 tries. Check your distribution N(" << _normDist.mean << "," << _normDist.stddev << ").  Function will return 0." << endl;
	//Util::exitWithPrompt(-1);

	return 0;
}

//adapted from Charles Stanton's 'Java Demos for Probability and Statistics site'
//http://www.math.csusb.edu/faculty/stanton/m262/
int RandomNums::randPoisson(double _mu) {
	assert (_mu >= 0);

	if( _mu == 0) return 0;

	double eMu = exp(-_mu);
	double product = 1.0;
	int count = -1;

    do  {
		product *= this->rand();
		count++;
    } while(product >= eMu);

	return count;

}

//-----------< End rand num functions >-----------------//

//-----------< Begin Getters and Setters >---------------//
unsigned long RandomNums::getSeed() {
	return this->seed;
}

void RandomNums::reset() {
	this->reset(this->seed);
}

void RandomNums::reset(unsigned long _seed) {
	this->seed = _seed;
	mtRand.seed((boost::mt19937::result_type) _seed);
//	isaac = QTIsaac<UINT32>(mtRand.randInt(), mtRand.randInt(),mtRand.randInt());	//Isaac
}
//-----------< End Getters and Setters >---------------//
