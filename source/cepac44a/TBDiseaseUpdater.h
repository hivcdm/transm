#pragma once

#include "include.h"

/* 
	TBDiseaseUpdater is a state updater that handles the occurence of acute TB and the 
	disease progression of reactivation, reinfection, relapse, and spontaneous resolution.  
	It also handles the effects of TB treatments and the switch of
	treatment after failure (though this may be moved into the clinic visit at some point).
*/
class TBDiseaseUpdater : public StateUpdater {
public:
	/* Constructor and Destructor */
	TBDiseaseUpdater(Patient *patient);
	~TBDiseaseUpdater(void);

	/* performInitialUpdates perform all of the state and statistics updates upon patient creation */
	void performInitialUpdates();
	/* performMonthlyUpdates perform all of the state and statistics updates for a simulated month */
	void performMonthlyUpdates();

private:
	/* performNoHistoryTBUpdates determines if a first TB infection occurs from the no history state*/
	void performNoHistoryTBUpdates();
	/* performLatentTBUpdates determines if TB reactivates or a reinfection occurs from the latent state */
	void performLatentTBUpdates();
	/* performActiveTBUpdates determines if a spontaneous resolution occurs from the active state */
	void performActiveTBUpdates();
	/* performOnTreatmentTBUpdates determines any disease changes while on treatment */
	void performOnTreatmentTBUpdates();
	/* performHistActiveTBUpdates determines if TB relapse occurs from the history of active state */
	void performHistActiveTBUpdates();
};
