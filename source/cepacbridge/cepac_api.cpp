/*
 * cepac_api.cpp
 *
 *  Created on: Oct 27, 2008
 *      Author: errhode
 */

#if defined(__APPLE__)
#include <dlfcn.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include "cepac_api.h"

#if defined(__APPLE__)

int setInputFile(const char *path){
void* cepac_handle = dlopen("/Users/errhode/Documents/workspace2/treatm/Debug/libtreatm.dylib", RTLD_NOW);

typedef int (*foofunc) ( const char* );
foofunc setInFile = (foofunc)dlsym(cepac_handle, "setInputFile");
int rtrn = setInFile(path);
dlclose(cepac_handle);

return rtrn;
}

int deleteMonthEvent(MonthEvent *_event){
	//void* cepac_handle = dlopen("/Users/errhode/Documents/workspace2/treatm/Debug/libtreatm.dylib", RTLD_NOW);

	//typedef int (*foofunc) ( MonthEvent* );
	//foofunc Foo = (foofunc)dlsym(cepac_handle, "deleteMonthEvent");
	//int rtrn = Foo(_event);
	//dlclose(cepac_handle);

	//return rtrn;
	free(_event);
	return 0;
}

void simulatePerson(CepacPerson * person, long months, unsigned long seed){
	void* cepac_handle = dlopen("/Users/errhode/Documents/workspace2/treatm/Debug/libtreatm.dylib", RTLD_NOW);

	typedef void (*foofunc) ( CepacPerson* , long, unsigned long );
	foofunc simPerson = (foofunc)dlsym(cepac_handle, "simulatePerson");

	typedef int (*barfunc) ( const char* );
	barfunc setInFile = (barfunc)dlsym(cepac_handle, "setInputFile");

	setInFile("31i_SA_inputs_initCD4_664.in");

	simPerson(person, months, seed);
	dlclose(cepac_handle);
}

#endif
