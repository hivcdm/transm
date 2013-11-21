#ifndef WIN23
#include <unistd.h>
#endif
#include "include.h"

/* Empty constructor and destructor, should never create an instance of this class */
CepacUtil::CepacUtil(void)
{
}
CepacUtil::~CepacUtil(void)
{
}

/* Constant string values for CEPAC version and file/directory information */
const char *CepacUtil::CEPAC_INPUT_VERSION = "201020540";
const char *CepacUtil::CEPAC_VERSION_STRING = "44a";
const char *CepacUtil::CEPAC_EXECUTABLE_COMPILED_DATE = "2010-08-12";
const char *CepacUtil::FILE_EXTENSION_FOR_TEMP = ".tmp";
const char *CepacUtil::FILE_EXTENSION_FOR_TRACE = ".txt";
const char *CepacUtil::FILE_EXTENSION_FOR_OUTPUT = ".out";
const char *CepacUtil::FILE_EXTENSION_FOR_INPUT = ".in";
const char *CepacUtil::FILE_EXTENSION_INPUT_SEARCH_STR = "*.in";
const char *CepacUtil::FILE_NAME_SUMMARIES = "popstats.out";

/* Vector of the file names to be run, and the inputs and results directories paths */
std::vector<std::string> CepacUtil::filesToRun;
std::string CepacUtil::inputsDirectory;
std::string CepacUtil::resultsDirectory;
bool CepacUtil::useRandomSeedByTime;

/* Random number generator class */
MTRand CepacUtil::mtRand;

/* useCurrentDirectoryForInputs determines the current directory and sets as inputs directory */
void CepacUtil::useCurrentDirectoryForInputs() {
#if defined(_WIN32)
	char buffer[512];
	_getcwd(buffer, 512);
	inputsDirectory = buffer;
#else
	char buffer[512];
	getcwd(buffer, 512);
	inputsDirectory = buffer;
#endif
} /* end useCurrentDirectoryForInputs */

/* findInputFiles locates all the .in files in the current directory and adds them
	to the filesToRun vector */
void CepacUtil::findInputFiles() {
#if defined(_WIN32)
	long hFile;
	struct _finddata_t tFileInfo;
	hFile = _findfirst( FILE_EXTENSION_INPUT_SEARCH_STR, &tFileInfo );
	int nInputFiles = 0;
	string fileName;

	//get the list of files that we have to process
	filesToRun.clear();
	do {
		fileName = (char *) tFileInfo.name;
		filesToRun.push_back(fileName);
		nInputFiles++;
	} while ( _findnext ( hFile, &tFileInfo ) == 0 );
	_findclose( hFile );
#else
	glob_t files;
	glob(FILE_EXTENSION_INPUT_SEARCH_STR, GLOB_ERR, NULL, &files);
	int nInputFiles = 0;
	string fileName;

	filesToRun.clear();
	//get the list of files that we have to process
	int i;
	for( i = 0; i < files.gl_pathc; i++) {
		fileName = (char *) files.gl_pathv[i];
		filesToRun.push_back(fileName);
		++nInputFiles;
	}
	globfree( &files);
#endif
} /* end findInputFiles */

/* createResultsDirectory creates the directory "results" as a subdirectory of the inputs one */
void CepacUtil::createResultsDirectory() {
#if defined(_WIN32)
	resultsDirectory = inputsDirectory;
	resultsDirectory.append("\\");
	resultsDirectory.append("results");
	_mkdir(resultsDirectory.c_str());
#else
	resultsDirectory = inputsDirectory;
	resultsDirectory.append("/");
	resultsDirectory.append("results");
	mkdir(resultsDirectory.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
} /* end createResultsDirectory */

/* changeDirectoryToResults changes the working directory to the results one */
void CepacUtil::changeDirectoryToResults() {
#if defined(_WIN32)
	_chdir(resultsDirectory.c_str());
#else
	chdir(resultsDirectory.c_str());
#endif
} /* end changeDirectoryToResults */

/* changeDirectoryToInputs changes the working directory to the inputs one */
void CepacUtil::changeDirectoryToInputs() {
#if defined(_WIN32)
	_chdir(inputsDirectory.c_str());
#else
	chdir(inputsDirectory.c_str());
#endif
} /* end changeDirectoryToInputs */

/* getDateString places the current date string in the specified buffer */
void CepacUtil::getDateString(char *buffer, int bufsize) {
#if defined(_WIN32)
	_strdate(buffer);
#else
	time_t currTime;
	time(&currTime);
	strftime(buffer, bufsize, "%m/%d/%y",localtime(&currTime));
#endif
} /* end getDateString */

/* getTimeString places the current system time string in the specified buffer */
void CepacUtil::getTimeString(char *buffer, int bufsize) {
#if defined(_WIN32)
	_strtime(buffer);
#else
	time_t currTime;
	time(&currTime);
	strftime(buffer, bufsize, "%H:%M:%S",localtime(&currTime));
#endif
} /* end getTimeString */

/* fileExists returns true if the specified file exists, false otherwise */
bool CepacUtil::fileExists(const char *filename) {
	FILE *file;
	//fopen_s(&file, filename, "r");
	file = fopen(filename, "r");
	if (!file)
		return false;
	fclose(file);
	return true;
} /* end fileExists */

/* openFile opens the specified file in the given mode */
FILE *CepacUtil::openFile(const char *filename, const char *mode) {
	FILE *file = fopen(filename, mode);
	return file;
} /* end openFile */

/* closeFile closes the specified file */
void CepacUtil::closeFile(FILE *file) {
	fclose(file);
} /* end closeFile */
