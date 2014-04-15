#pragma once

#include "include.h"

/*
	HIVInfectionUpdater handles the transition from acute to chronic HIV and updates all
	associated patient state and statistics.  It only does work during the specified
	transition month.
*/
class HIVInfectionUpdater : public StateUpdater {
public:
	/* Constructor and Destructor */
	HIVInfectionUpdater(Patient *patient);
	~HIVInfectionUpdater(void);

	/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
	void performInitialUpdates();
	/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
	void performMonthlyUpdates();

	/* performHIVNewInfectionUpdates handles the new infection of an HIV negative patient*/
	//This function has to be public so that the Transmission model can access it
	void performHIVNewInfectionUpdates();

private:
	/* performHIVNegativeUpdates determines if HIV negative patients become infected */
	void performHIVNegativeUpdates();
	/* performAcuteToChronicHIVUpdates handles the transition from acute to chronic HIV */
	void performAcuteToChronicHIVUpdates();
	/* performPediatricDiseaseUpdates determines maternal updates and early to late childhood transition */
	void performPediatricDiseaseUpdates();
};
