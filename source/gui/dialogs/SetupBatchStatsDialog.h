#pragma once

#include <wx/wx.h>

class DisplayBox;

class SetupBatchStatsDialog : public wxDialog
{
public:
	SetupBatchStatsDialog(const wxString &title, DisplayBox *dbox);

	wxCheckBox *prevalenceSelect;
	wxCheckBox *prevalenceSASelect;
	wxCheckBox *incidenceSelect;
	wxCheckBox *PopulationSizeSelect;
	wxCheckBox *NumInfectedSelect;
	wxCheckBox *NumNewInfectionsSelect;

};
