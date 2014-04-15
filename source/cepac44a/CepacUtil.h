#pragma once

#include "include.h"

/*
	CepacUtil contains the platform specific utility functions for hadling files/directories,
	generating random numbers, and determining the current date/time.
*/
class CepacUtil
{
public:
	/* Empty constructor and destructor, should never create an instance of this class */
	CepacUtil(void);
	~CepacUtil(void);

	/* Constant values for CEPAC version and file/directory information */
	static const char *CEPAC_INPUT_VERSION;
	static const char *CEPAC_VERSION_STRING;
	static const char *CEPAC_EXECUTABLE_COMPILED_DATE;
	static const char *FILE_EXTENSION_FOR_TEMP;
	static const char *FILE_EXTENSION_FOR_TRACE;
	static const char *FILE_EXTENSION_FOR_OUTPUT;
	static const char *FILE_EXTENSION_FOR_INPUT;
	static const char *FILE_EXTENSION_INPUT_SEARCH_STR;
	static const char *FILE_NAME_SUMMARIES;

	/* Vector of the file names to be run, and the inputs and results directories paths */
	static std::vector<std::string> filesToRun;
	static std::string inputsDirectory;
	static std::string resultsDirectory;

	/* Functions for handling directories and locating the input files */
	static void useCurrentDirectoryForInputs();
	static void findInputFiles();
	static void createResultsDirectory();
	static void changeDirectoryToResults();
	static void changeDirectoryToInputs();

	/* Functions for returning the current system date and time */
	static void getDateString(char *buffer, int bufsize);
	static void getTimeString(char *buffer, int bufsize);

	/* Functions and state variables for generating uniform and gaussian random numbers */
	static void setRandomSeedType(bool useTimeSeed);
	static double getRandomDouble(int callSiteId, Patient *patient);
	static double getRandomGaussian(double mean, double stdDev, int callSiteId, Patient *patient);
	static bool useRandomSeedByTime;
	static MTRand mtRand;

	/* Probability modification functions */
	static double probToRate(double prob);
	static double rateToProb(double rate);
	static double probRateMultiply(double prob, double rateMult);
	static double probToLogit(double prob);
	static double logitToProb(double logit);
	static double probLogitAdjustment(double prob, double logitAdjust);

	/* Functions for opening and closing files */
	static bool fileExists(const char *filename);
	static FILE *openFile(const char *filename, const char *mode);
	static void closeFile(FILE *file);
};

/* setRandomSeedType sets up the random number generator to use seed by time or fixed seed */
inline void CepacUtil::setRandomSeedType(bool useTimeSeed) {
	useRandomSeedByTime = useTimeSeed;
	if (useRandomSeedByTime)
		mtRand.seed((unsigned int) time(0));
	else
		mtRand.seed(8675309);
} /* end setRandomSeedType */

/* getRandomDouble returns a random number within the range [0,1) */
inline double CepacUtil::getRandomDouble(int callSiteId, Patient *patient) {
	// If using fixed seed, reseed by callSite, patient number, and month number
	if (!useRandomSeedByTime) {
		int patientNum = patient->getGeneralState()->patientNum;
		int monthNum = patient->getGeneralState()->monthNum;
		int newSeed = 8675309 ^ (callSiteId << 5) ^ patientNum ^ (monthNum << 11);
		mtRand.seed(newSeed);
	}
	return mtRand();
} /* end getRandomDouble */

/* getRandomGaussian returns a random normally distributed value with the specified mean
	and standard deviation */
inline double CepacUtil::getRandomGaussian(double mean, double stdDev, int callSiteId, Patient *patient) {
	// If using fixed seed, reseed by callSite, patient number, and month number
	if (!useRandomSeedByTime) {
		int patientNum = patient->getGeneralState()->patientNum;
		int monthNum = patient->getGeneralState()->monthNum;
		int newSeed = 8675309 ^ (callSiteId << 5) ^ patientNum ^ (monthNum << 11);
		mtRand.seed(newSeed);
	}
	// Polar form of Box-Muller transformation
    double x1, x2, w, y1, y2;
	do {
		x1 = 2.0 * mtRand() - 1.0;
		x2 = 2.0 * mtRand() - 1.0;
		w = x1 * x1 + x2 * x2;
	} while ( w >= 1.0 );
	w = sqrt( (-2.0 * log( w ) ) / w );
	y1 = x1 * w;
	y2 = x2 * w;

	return (mean + (y2 * stdDev));
} /* end getRandomGaussian */

/* probToRate converts a probability to a rate */
inline double CepacUtil::probToRate(double prob) {
	return (-1 * log(1 - prob));
} /* end probToRate */

/* rateToProb converts a rate to a probability */
inline double CepacUtil::rateToProb(double rate) {
	return (1 - exp(-1 * rate));
} /* end rateToProb */

/* probRateMultiply modifies a probability by a rate multiplier */
inline double CepacUtil::probRateMultiply(double prob, double rateMult) {
	// Formula is derived from conversion to rate, perform multiply, and convert back to prob
	if (rateMult == 0)
		return 0;
	if (rateMult == 1)
		return prob;
	return (1 - pow(1 - prob, rateMult));
} /* end probRateMultiply */

inline double CepacUtil::probToLogit(double prob) {
	return log(prob / (1 - prob));
}

inline double CepacUtil::logitToProb(double logit) {
	return 1 / (1 + exp(-1 * logit));
}

inline double CepacUtil::probLogitAdjustment(double prob, double logitAdjust) {
	return logitToProb(probToLogit(prob) + logitAdjust);
}

