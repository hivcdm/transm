#pragma once

#include "include.h"

/**
	The MortalityUpdater class is a state updater that checks for the occurrence of
	chronic AIDS or non-AIDS death each month, and updates the patient's death state if so.
	The chronic AIDS death also includes TB extended mortality effects.
*/
class MortalityUpdater : public StateUpdater {
public:
	/* Constructor and Destructor */
	MortalityUpdater(Patient *patient);
	~MortalityUpdater(void);

	/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
	void performInitialUpdates();
	/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
	void performMonthlyUpdates();
};
