#ifndef _CEPAC_DLL_H
#define _CEPAC_DLL_H

#if defined(__APPLE__)
#include <dlfcn.h>
#endif

// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the CEPAC_DLL_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// CEPAC_LIB_SIG functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#if defined(WIN32)
	#include <windows.h>

	#if defined (CEPAC_DLL_EXPORTS)
		#define CEPAC_LIB_SIG __declspec(dllexport)
	#else
		//by default, we are linking to the cepac.dll library, so use dll imports
		#define CEPAC_LIB_SIG __declspec(dllimport)
	#endif
#endif

#if defined(__APPLE__)
#define CEPAC_LIB_SIG
#endif

//if we are not using this from a Windows .dll, then wipe out special dll method signatures
//use dlopen to open libtreatm.dylib
#if !(defined(CEPAC_LIB_SIG))
	#undef CEPAC_LIB_SIG
	#define CEPAC_LIB_SIG
#endif



//if we are calling this from a .cpp file, then we have to the compiler know this
//as of 9/8/08, CEPAC is written in C, but the transmission model is written in C++
//these functions are implemented in C. Added by schung5
#if defined(CPP)
extern "C" {
#endif

//each patient is in 1 of these states at all times
typedef enum {
	HIVInfectionStageUNINFECTED,
	HIVInfectionStageACUTE,
	HIVInfectionStageCHRONIC,
	HIVInfectionStageLATE_STAGE,
	ENDHIVInfectionState,
} HIVInfectionStage;

/**
	if simulatePerson(...) is called then the CEPAC model dll will generate
	a linked list of MonthEvent which will contain the results
	for that month of simulation
	added by schung5
**/
typedef struct CEPAC_LIB_SIG MonthEvent {
	long month;
	float cd4;
	int hvl;
	double cost;
	double qol;
	HIVInfectionStage infectionStage;

	struct MonthEvent *nextMth;
} MonthEvent;

//modified by schung5 on 6/22/07 -- allow CEPAC to determine initial CD4 and HVL based on incident or prevalent infection
typedef struct CEPAC_LIB_SIG CepacPerson {
	int gender;
	int age;

	//set to >0 if this person is an incident infection, else person was part of prevalent infection population
	//currently an int b/c CEPAC doesn't understand bools
	int incidentInfection;

	MonthEvent *firstMth;
} CepacPerson;

/** Sets the CEPAC parameters that the .dll will use to generate patient traces
*	@param path filename of input file to be read to initialize CEPAC
*	@return 0 if successful; non-zero value to indicate error opening or reading file
*	@author Hong Zhang
*/
CEPAC_LIB_SIG int setInputFile(const char *path);

/**
Fills in the values of a MonthEvent struct
***/
CEPAC_LIB_SIG void setMonthEvent(MonthEvent* monthEvent, long month, float cd4, int hvl, double cost, double qol, HIVInfectionStage infectionStage);

/**Allow the cepac dll to deal with its own heap b/c calling 'free' from an external C++ app causes problems.
*	@param _event a MonthEvent* that was malloc'ed within the CEPAC dll
*	@author schung5	(6/22/07)
*/
CEPAC_LIB_SIG int deleteMonthEvent(MonthEvent *_event);

/**
* @param person pointer to record specifying characteristics of person to be simulated
* @param months not used currently
* @param seed sets the seeds for the random number generators for the patient
* @return simulated months for person stored in linked list starting from person->firstMth
* @author Hong Zhang
*/
CEPAC_LIB_SIG void simulatePerson(CepacPerson * person, long months, unsigned long seed);

#if defined(CPP)
}
#endif

#endif	//#ifndef _CEPAC_DLL_H
