/*
 * SetupBatchStatsDialog.cpp
 *
 *  Created on: Oct 28, 2009
 *      Author: errhode
 */

#include "SetupBatchStatsDialog.h"
#include "../DisplayBox.h"

int ID_SETUPOK = 1;
int ID_PREVSELECT = 2;
int ID_INCIDSELECT = 3;
int ID_POPSIZESELECT = 4;
int ID_NUMINFECTSELECT = 5;
int ID_NEWINFECTSELECT = 6;
int ID_PREVSASELECT = 7;

SetupBatchStatsDialog::SetupBatchStatsDialog(const wxString &title, DisplayBox *dbox)
	: wxDialog(nullptr, -1, title, wxDefaultPosition, wxSize(350, 280))
{
	wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *hbox = new wxBoxSizer(wxHORIZONTAL);
	wxPanel *textPanel = new wxPanel(this, -1);
	new wxStaticText(textPanel, -1,
	                 wxT(" Select the batchstats files you want generated. \n (There will be one file per variable, tracking each \n variable over time and over all runs.)"));
	//Store all options for popstats files here
	wxPanel *checkboxPanel = new wxPanel(this, -1);
	wxBoxSizer *checkboxSizer = new wxBoxSizer(wxVERTICAL);
	this->prevalenceSelect = new wxCheckBox(checkboxPanel, ID_PREVSELECT, wxT("Prevalence"));
	this->prevalenceSASelect = new wxCheckBox(checkboxPanel, ID_PREVSASELECT,
	        wxT("Prevalence among the sexually active population"));
	this->incidenceSelect = new wxCheckBox(checkboxPanel, ID_INCIDSELECT, wxT("Incidence Rate (per year)"));
	this->PopulationSizeSelect = new wxCheckBox(checkboxPanel, ID_POPSIZESELECT, wxT("Total population size"));
	this->NumInfectedSelect = new wxCheckBox(checkboxPanel, ID_NUMINFECTSELECT, wxT("Number Infected"));
	this->NumNewInfectionsSelect = new wxCheckBox(checkboxPanel, ID_NEWINFECTSELECT, wxT("Number of new infections"));
	this->prevalenceSelect->SetValue(dbox->BatchStatsTrack[PREVALENCE]);
	this->prevalenceSASelect->SetValue(dbox->BatchStatsTrack[PREVALENCESA]);
	this->incidenceSelect->SetValue(dbox->BatchStatsTrack[INCIDENCE]);
	this->PopulationSizeSelect->SetValue(dbox->BatchStatsTrack[POPULATION]);
	this->NumInfectedSelect->SetValue(dbox->BatchStatsTrack[CURRENTLYINFECTED]);
	this->NumNewInfectionsSelect->SetValue(dbox->BatchStatsTrack[NEWINFECTIONS]);
	checkboxSizer->Add(this->prevalenceSelect, 0, wxALIGN_LEFT | wxALL, 2);
	checkboxSizer->Add(this->prevalenceSASelect, 0, wxALIGN_LEFT | wxALL, 2);
	checkboxSizer->Add(this->incidenceSelect, 0, wxALIGN_LEFT | wxALL, 2);
	checkboxSizer->Add(this->PopulationSizeSelect, 0, wxALIGN_LEFT | wxALL, 2);
	checkboxSizer->Add(this->NumInfectedSelect, 0, wxALIGN_LEFT | wxALL, 2);
	checkboxSizer->Add(this->NumNewInfectionsSelect, 0, wxALIGN_LEFT | wxALL, 2);
	checkboxPanel->SetSizer(checkboxSizer);
	wxButton *okButton = new wxButton(this, wxID_OK, wxT("Ok"), wxDefaultPosition, wxSize(70, 30));
	hbox->Add(okButton, 1);
	//vbox->Add(setupPanel, 1);
	vbox->Add(textPanel, 0, wxALIGN_CENTER | wxALL, 5);
	vbox->Add(checkboxPanel, 0, wxALIGN_CENTER | wxALL, 5);
	vbox->Add(hbox, 0, wxALIGN_CENTER | wxTOP | wxBOTTOM, 10);
	SetSizer(vbox);
	Centre();
	ShowModal();
	Destroy();
}
