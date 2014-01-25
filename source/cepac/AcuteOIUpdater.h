#pragma once

#include "include.h"

/**
	The AcuteOIUpdater class tests for the occurrence of acute OIs each month and updates
	all the patient state and statistics if they occur.
*/
class AcuteOIUpdater : public StateUpdater {
public:
	/* Constructor and Destructor */
	AcuteOIUpdater(Patient *patient);
	~AcuteOIUpdater(void);

	/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
	void performInitialUpdates();
	/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
	void performMonthlyUpdates();

private:
	/* determineAcuteOI returns the acute OI that occurs this month or OI_NONE if no OI occurs */
	SimContext::OI_TYPE determineAcuteOI();
	/* determineDeathByOI determines the risk for acute OI death */
	void determineDeathByOI(SimContext::OI_TYPE oiType);
};
