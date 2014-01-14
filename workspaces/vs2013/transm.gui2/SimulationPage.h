
#include "Page.h"

class wxTextCtrl;

class SimulationPage : Page
{
public:
	wxTextCtrl *inputVersion_;
	wxTextCtrl *debugLevel_;
	wxTextCtrl *baseName_;
	wxTextCtrl *numTimeSteps_;
	wxTextCtrl *randomSeed_;
	wxTextCtrl *monthOf1990_;
};