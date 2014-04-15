/*
 * playground.h
 *
 *  Created on: Jun 5, 2009
 *      Author: errhode
 */
#pragma once

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <wx/wx.h>
#include <wx/aboutdlg.h>

#include <core/Constants.h>

#include "dialogs/SetupBatchStatsDialog.h"
#include "widgets/statusWidget.h"
#include "widgets/verticalStatusWidget.h"

class Simulation;

class DisplayBox : public wxFrame
{
public:
	DisplayBox(const wxString &title);

	wxMenuBar *menubar;
	wxMenu *file;
	wxMenu *edit;
	wxMenu *help;
	wxTextCtrl *textctrl;
	wxTextCtrl *summaryText;
	wxCheckBox *graphicsCheckbox;
	verticalStatusWidget *prevalenceWidget;
	verticalStatusWidget *incidenceWidget;
	StatusWidget *totalProgressWidget;
	StatusWidget *singleProgressWidget;
	wxButton *runButton;

	wxCriticalSection locker;

	//The factors to be plotted in the status widgets
	//current prevalence for prevalence widget (100 * current prevalence)
	int currPrev;
	//current incidence for the incidence widget (1000 * current incidence)
	int currentIncidence;
	//percent completed for total progress widget
	int percentCompleted;
	//percentage of timesteps of current run completed for singleProgressWidget
	int currentRunProgress;

	//The list of files to be run by the simulation collected upon "File->Open"
	std::vector<std::string> filesToRun;
	std::string currentDirectory;

	//Determines which variables should be tracked in popstats files
	bool BatchStatsTrack[ENDBatchStatsVariables];

	//The functions for each of the menu commands
	void OnRun(wxCommandEvent &event);

	void OnOpen(wxCommandEvent &WXUNUSED(event));

	void OnQuit(wxCommandEvent &WXUNUSED(event));

	void OnAbout(wxCommandEvent &WXUNUSED(event));

	void OnProtect(wxCommandEvent &WXUNUSED(event));

	void OnAboutGraphViz(wxCommandEvent &WXUNUSED(event));

	void OnSetupBatchStats(wxCommandEvent &WXUNUSED(event));

	void BackgroundUpdate(Simulation &sim);

private:
	void Simulate();

	bool simRunning;

	bool updating;
};
