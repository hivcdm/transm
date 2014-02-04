/*
 * playground.cpp
 *
 *  Created on: Jun 5, 2009
 *      Author: errhode
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <thread>
#if defined(WIN32)
#include <direct.h>
#else
#include <dirent.h>
#endif
#include "DisplayBox.h"
#include "../Sim.h"
#include "trans1.xpm"
#include <wx/aboutdlg.h>
#include <wx/utils.h>
#include <wx/dir.h>
#include <wx/regex.h>

#include "../cepac/include.h"
#include "../util/Util.h"
#include "../statistics/TransmissionSummaryStats.h"

//The ID codes for the menu options -- these need to be unique!
int ID_RUN = 1;
int ID_ABOUT = 2;
int ID_PROTECT = 3;
int ID_OPEN = 4;
int ID_ABOUTGRAPHVIZ = 5;
int ID_CHECKBOX = 6;
int ID_BATCHSTATS = 7;
int ID_FIXEDSEED = 8;

DisplayBox::DisplayBox(const wxString &title)
	: wxFrame(NULL, wxID_ANY, title, wxPoint(-1, -1), wxSize(600, 600))
{
	//Set frame icon
	SetIcon(wxIcon(trans1_xpm));
	menubar = new wxMenuBar;
	file = new wxMenu;
	file->Append(ID_RUN, wxT("&Run"));
	file->Append(ID_OPEN, wxT("&Open"));
	file->Append(wxID_EXIT, wxT("&Quit"));
	help = new wxMenu;
	help->Append(ID_ABOUTGRAPHVIZ, wxT("&Generating GraphViz"));
	help->Append(ID_PROTECT, wxT("&Protect"));
	help->Append(ID_ABOUT, wxT("&About..."));
	menubar->Append(file, wxT("&File"));
	menubar->Append(help, wxT("&Help"));
	SetMenuBar(menubar);
	//Initialize the markers on the status bars
	this->percentCompleted = 0;
	this->currPrev = 0;
	this->currentIncidence = 0;
	this->currentRunProgress = 0;

	//Initialize the batchstats booleans in the setup dialog
	for(int i = 0; i < ENDBatchStatsVariables; i++)
	{
		this->BatchStatsTrack[i] = false;
	}

	//The overall main panel
	wxPanel *mainPanel = new wxPanel(this, wxID_ANY);
	//This panel stores all panels relating to the current run (not the overall runs)
	wxPanel *singleRunPanel = new wxPanel(mainPanel, wxID_ANY);
	//This panel stores the text box and the status panel for the single runs vertically
	wxPanel *singleRunSubPanel = new wxPanel(singleRunPanel, wxID_ANY);
	//These are the panels containing the status widgets/vertical status widgets
	wxPanel *prevalencePanel = new wxPanel(singleRunPanel, wxID_ANY);
	wxPanel *summaryStatusPanel = new wxPanel(mainPanel, wxID_ANY);
	wxPanel *singleStatusPanel = new wxPanel(singleRunPanel, wxID_ANY);
	//The panel containing the checkbox for turning on graphics (and any future options?)
	wxPanel *checkboxPanel = new wxPanel(mainPanel, wxID_ANY);
	//The panel containing buttons to set up popstats and to run the simulation
	wxPanel *buttonsPanel = new wxPanel(mainPanel, wxID_ANY);
	//The two text boxes for displaying output to the use
	textctrl = new wxTextCtrl(singleRunSubPanel, -1, wxT("Individual Runs Status\n\n"), wxPoint(-1, -1), wxSize(),
	                          wxTE_MULTILINE);
	summaryText = new wxTextCtrl(mainPanel, -1,
	                             wxT("CEPAC Transmission Model\n Go to File->Open to select your folder of inputs\n"), wxPoint(-1, -1), wxSize(),
	                             wxTE_MULTILINE);
	//Graphics checkbox: value is false (don't generate) by default
	this->graphicsCheckbox = new wxCheckBox(checkboxPanel, ID_CHECKBOX,
	                                        wxT("Generate GraphViz files: See \"Help - Generating GraphViz\" for more information"), wxPoint(0, 0));
	this->graphicsCheckbox->SetValue(false);
	//Add two buttons to buttonsPanel
	wxBoxSizer *hButtonBox = new wxBoxSizer(wxHORIZONTAL);
	hButtonBox->Add(new wxButton(buttonsPanel, ID_BATCHSTATS, wxT("Set up BatchStats Output")), 1, wxALIGN_LEFT | wxALL, 5);
	runButton = new wxButton(buttonsPanel, ID_RUN, wxT("Run"));
	hButtonBox->Add(runButton, 2, wxALIGN_RIGHT | wxALL, 5);
	buttonsPanel->SetSizer(hButtonBox);
	prevalenceWidget = new verticalStatusWidget(prevalencePanel, wxID_ANY, &(this->currPrev), 1,
	        wxString::Format(wxT("Current Percent Infected")));
	wxBoxSizer *hPrevBox = new wxBoxSizer(wxHORIZONTAL);
	hPrevBox->Add(prevalenceWidget, 1, wxEXPAND | wxALL, 5);
	incidenceWidget = new verticalStatusWidget(prevalencePanel, wxID_ANY, &(this->currentIncidence), 0.5,
	        wxString::Format(wxT("Current Yearly Incidence Rate as percentage of population")));
	hPrevBox->Add(incidenceWidget, 1, wxEXPAND | wxALL, 5);
	prevalencePanel->SetSizer(hPrevBox);
	totalProgressWidget = new StatusWidget(summaryStatusPanel, wxID_ANY, &(this->percentCompleted),
	                                       wxString::Format(wxT("Percent of Runs Completed")));
	wxBoxSizer *hSumBox = new wxBoxSizer(wxHORIZONTAL);
	hSumBox->Add(totalProgressWidget, 1, wxEXPAND);
	summaryStatusPanel->SetSizer(hSumBox);
	singleProgressWidget = new StatusWidget(singleStatusPanel, wxID_ANY, &(this->currentRunProgress),
	                                        wxString::Format(wxT("Completion Status of Current Run")));
	wxBoxSizer *hSingleBox = new wxBoxSizer(wxHORIZONTAL);
	hSingleBox->Add(singleProgressWidget, 1, wxEXPAND);
	singleStatusPanel->SetSizer(hSingleBox);
	//The vertical portion of the single Run Panel
	wxBoxSizer *singleRunPanelSizer1 = new wxBoxSizer(wxVERTICAL);
	singleRunPanelSizer1->Add(textctrl, 2, wxEXPAND | wxALL, 5);
	singleRunPanelSizer1->Add(singleStatusPanel, 0, wxEXPAND | wxALL, 5);
	singleRunSubPanel->SetSizer(singleRunPanelSizer1);
	//The horizontal (i.e. total) panel of the single Run Panel
	wxBoxSizer *singleRunPanelSizer2 = new wxBoxSizer(wxHORIZONTAL);
	singleRunPanelSizer2->Add(singleRunSubPanel, 4, wxEXPAND | wxALL, 0);
	singleRunPanelSizer2->Add(prevalencePanel, 0, wxEXPAND | wxALL, 0);
	singleRunPanel->SetSizer(singleRunPanelSizer2);
	wxBoxSizer *sizer1 = new wxBoxSizer(wxVERTICAL);
	sizer1->Add(summaryText, 1, wxEXPAND | wxALL, 5);
	sizer1->Add(summaryStatusPanel, 0, wxEXPAND | wxALL, 5);
	sizer1->Add(singleRunPanel, 3, wxEXPAND | wxALL, 0);
	sizer1->Add(checkboxPanel, 0, wxEXPAND | wxALL, 5);
	sizer1->Add(buttonsPanel, 0, wxEXPAND | wxALL, 5);
	mainPanel->SetSizer(sizer1);
	Connect(ID_RUN, wxEVT_COMMAND_BUTTON_CLICKED,
	        wxCommandEventHandler(DisplayBox::OnRun));
	Connect(ID_RUN, wxEVT_COMMAND_MENU_SELECTED,
	        wxCommandEventHandler(DisplayBox::OnRun));
	Connect(ID_BATCHSTATS, wxEVT_COMMAND_BUTTON_CLICKED,
	        wxCommandEventHandler(DisplayBox::OnSetupBatchStats));
	Connect(ID_OPEN, wxEVT_COMMAND_MENU_SELECTED,
	        wxCommandEventHandler(DisplayBox::OnOpen));
	Connect(ID_ABOUTGRAPHVIZ, wxEVT_COMMAND_MENU_SELECTED,
	        wxCommandEventHandler(DisplayBox::OnAboutGraphViz));
	Connect(wxID_EXIT, wxEVT_COMMAND_MENU_SELECTED,
	        wxCommandEventHandler(DisplayBox::OnQuit));
	Connect(ID_PROTECT, wxEVT_COMMAND_MENU_SELECTED,
	        wxCommandEventHandler(DisplayBox::OnProtect));
	Connect(ID_ABOUT, wxEVT_COMMAND_MENU_SELECTED,
	        wxCommandEventHandler(DisplayBox::OnAbout));
	Centre();
}

void DisplayBox::BackgroundUpdate(Sim &sim)
{
	{
		wxCriticalSectionLocker lock(locker);
		if(!simRunning)
		{
			return;
		}
	}
	bool running = sim.Step();
	{
		wxCriticalSectionLocker lock(locker);
		if(simRunning)
		{
			simRunning = running;
		}
		updating = false;
	}
}

void DisplayBox::OnRun(wxCommandEvent &WXUNUSED(event))
{
	if(this->graphicsCheckbox->GetValue())
	{
		*(this->summaryText) <<
		                     wxT("WARNING: GraphViz files will be generated!  If your input file has a starting cohort size greater than 100 or a birth rate greater than 0, no GraphViz files will be generated.\n");
	}
	else
	{
		*(this->summaryText) << wxT("No graphics will be generated.\n");
	}

	if(runButton->GetLabel() == "Run")
	{
		runButton->SetLabel("Stop");
	}
	else
	{
		{
			wxCriticalSectionLocker lock(locker);
			simRunning = false;
		}

		runButton->SetLabel("Run");

		return;
	}

	//Reset the status bars to 0
	this->percentCompleted = 0;
	this->totalProgressWidget->Refresh();
	this->totalProgressWidget->Update();
	this->currentRunProgress = 0;
	this->singleProgressWidget->Refresh();
	this->singleProgressWidget->Update();

	//Make sure input files have been selected
	if(this->filesToRun.empty())
	{
		*(this->summaryText) << wxT("No files to run -- Open a directory first!\n");
	}

	//Create the results directory (for CEPAC output) and CEPAC popstats file
	CepacUtil::createResultsDirectory();
	SummaryStats *cepacSummaryStats = new SummaryStats("cepacPopstats.out");
	TransmissionSummaryStats *transSummaryStats = new TransmissionSummaryStats("summaryStats.out");
	double totalFiles = (double) this->filesToRun.size();
	int i;

	for(i = 0; i < totalFiles; i++)
	{
		//Initialize single progress widget
		this->currentRunProgress = 0;
		this->singleProgressWidget->Refresh();
		this->singleProgressWidget->Update();
		wxString filename(filesToRun[i].c_str(), wxConvUTF8);
		*(this->summaryText) << wxT("Running File: ") << filename << wxT("\n");
		//Run simulation on selected file
		*(this->textctrl) << wxT("Running simulation...\n");

		Sim s(filesToRun[i]);
		s.Initialize();

		simRunning = true;

		while(true)
		{
			{
				wxCriticalSectionLocker lock(locker);
				if(!simRunning)
				{
					break;
				}
			}

			updating = true;
			std::thread backgroundThread(&DisplayBox::BackgroundUpdate, this, std::ref(s));

			while(true)
			{
				{
					wxCriticalSectionLocker lock(locker);
					if(!updating)
					{
						break;
					}
				}
				wxYield();
				UpdateWindowUI();
				Update();

				currPrev = s.GetPrevalence();
				currentIncidence = s.GetIncidence();
				currentRunProgress = (100.0 * s.GetTime()) / s.GetTotalTime() + 0.5;
			}

			backgroundThread.join();

			while(!s.GetEventParams()->outputMessageQueue.empty())
			{
				*(this->textctrl) << s.GetEventParams()->outputMessageQueue.front();
				s.GetEventParams()->outputMessageQueue.pop_front();
			}

			textctrl->Refresh();
			textctrl->Update();
		}

		*(this->textctrl) << wxT("Done!\n");
		this->percentCompleted = (100 * (i + 1)) / totalFiles + 0.5;
		this->totalProgressWidget->Refresh();
		this->totalProgressWidget->Update();
	}

	//Finalize CEPAC summary stats and print the popstats file
	cepacSummaryStats->finalizeStats();

	try
	{
		cepacSummaryStats->writeSummariesFile();
		transSummaryStats->writeSummariesFile();
	}
	catch(string errorString)
	{
		wxString wxErrorString(errorString.c_str(), wxConvUTF8);
		*(this->textctrl) << wxErrorString << wxT("\n");
	}

	delete cepacSummaryStats;
	delete transSummaryStats;
	//Clear files to run once they've been run
	this->filesToRun.clear();
	*(this->summaryText) << wxT("All runs completed!  Open another directory to run more...\n");

	runButton->SetLabel("Run");
}

void DisplayBox::OnOpen(wxCommandEvent &WXUNUSED(event))
{
	//Clear filesToRun before adding a directory of new files.
	this->filesToRun.clear();
	wxDirDialog *OpenDialog = new wxDirDialog(this, _("Choose a directory of valid parameter files (.xml)"));

	// Creates a "open file" dialog with 4 file types
	if(OpenDialog->ShowModal() == wxID_OK)  // if the user clicked "Open" instead of "Cancel"
	{
		wxDir *directory = new wxDir(OpenDialog->GetPath());

		if(directory->IsOpened())
		{
			if(directory->HasFiles(wxT("*.xml")))
			{
				wxString *filename = new wxString(wxT(""));
				directory->GetFirst(filename, wxT("*.xml"));
				//Set the current working directory to be the chosen path; modified for multiple operating systems
				this->currentDirectory = std::string(OpenDialog->GetPath().mb_str());
				CepacUtil::inputsDirectory = this->currentDirectory;
				CepacUtil::changeDirectoryToInputs();

				do
				{
					*(this->summaryText) << OpenDialog->GetPath();
#if defined(WIN32)
					*(this->summaryText) << wxT("\\");
#endif
#if defined(__APPLE__)
					*(this->summaryText) << wxT("/");
#endif
					*(this->summaryText) << *filename << wxT("\n");
					wxString fullFile = OpenDialog->GetPath();
#if defined(WIN32)
					fullFile.append(wxT("\\"));
#endif
#if defined(__APPLE__)
					fullFile.append(wxT("/"));
#endif
					fullFile.append(*filename);
					//check to see if the last two characters of file name are digits
					wxRegEx re(wxT(".*_seq([[:digit:]]{2})[.]xml$"));

					//add files only if they are not in a sequence or are the first file in the sequence
					if(re.Matches(*filename))
					{
						long seqNum; //the number in the sequence
						re.GetMatch(*filename, 1).ToLong(&seqNum);

						if(seqNum != 1)
						{
							continue;
						}
					}

					this->filesToRun.push_back(std::string(fullFile.mb_str()));
					/*#if defined(WIN32)
										_chdir(this->currentDirectory.c_str());
					#else
										chdir(this->currentDirectory.c_str());
					#endif*/
				}
				while(directory->GetNext(filename));

				//Only you can prevent memory leaks!
				delete filename;
			}
		}

		//Stop memory leaks now!
		delete directory;
	}

	// Clean up after ourselves
	OpenDialog->Destroy();
}

void DisplayBox::OnQuit(wxCommandEvent &WXUNUSED(event))
{
	{
		wxCriticalSectionLocker lock(locker);
		simRunning = false;
	}

	Close(true);
}

void DisplayBox::OnAbout(wxCommandEvent &WXUNUSED(event))
{
	wxAboutDialogInfo info;
	stringstream Version;
	Version << "Version " << Util::MODEL_VERSION << " (input sheet version " << Util::INPUT_VERSION << ", CEPAC version " <<
	        CepacUtil::CEPAC_VERSION_STRING << " (input version " << CepacUtil::CEPAC_INPUT_VERSION << "))";
	wxString wxVersion(Version.str().c_str(), wxConvUTF8);
	info.SetName(_("CEPAC Population Dynamics"));
	info.SetVersion(wxVersion);
	info.SetDescription(_("An agent based model of HIV transmission"));
	info.AddDeveloper(_("the CEPAC Research Group"));
	info.AddDeveloper(_("Massachusetts General Hospital"));
	info.AddDeveloper(_("Harvard School of Public Health"));
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4428)
#endif
	info.SetCopyright(wxT("\u00A9 2008-2014"));
#ifdef _MSC_VER
#pragma warning(pop)
#endif
	wxAboutBox(info);
}

void DisplayBox::OnProtect(wxCommandEvent &WXUNUSED(event))
{
	//wxLaunchDefaultBrowser(wxT("http://www.youtube.com/watch?v=4O7kCoJ-pTw"), wxBROWSER_NEW_WINDOW);
	wxMessageBox(_T("Always use protection... remember to back up your work!"),
	             _T("Protect Yourself"), wxOK | wxICON_INFORMATION, this);
}

void DisplayBox::OnAboutGraphViz(wxCommandEvent &WXUNUSED(event))
{
	wxMessageBox(
	    _T("IF YOU DON'T KNOW IF YOU SHOULD USE THIS FEATURE, YOU PROBABLY SHOULDN'T!  If you know how to process them, the files generated when the checkbox is selected can be used to generate images of the population dynamics.  If you don't know how to process them, well, then you will just have some extra files.  No harm, no foul, but best to save your hard drive space by not checking the box.\n\n---------------------------\n\nWhen you select the checkbox, the simulation model will generate input files for the GraphViz program if and only if the following criteria are met in your parameter files:\n\n  - The initial population size must be less than or equal to 100\n  - The birth rate must be 0 (no persons added to the population)\n\n\nIn order to generate images from the .gv files, you must have the GraphViz program installed on your machine.  You can download it at: www.graphviz.org\n\n Follow the instructions on the GraphViz website or in the GraphViz user guide to convert the input files into graphical output.  If you want to combine them into an animation, you must use some sort of external animation tool.  Contact erhode@partners.org if you would like help with this."),
	    _T("Generating GraphViz Files"), wxOK | wxICON_INFORMATION, this);
}

void DisplayBox::OnSetupBatchStats(wxCommandEvent &WXUNUSED(event))
{
	SetupBatchStatsDialog *setupBatchStats = new SetupBatchStatsDialog(_T("BatchStats variables"), this);
	setupBatchStats->Show(true);
	*(this->summaryText) <<
	                     wxT("BatchStats files will be generated to track the following variables over time amongst all runs in the batch:\n\t");

	if(setupBatchStats->prevalenceSelect->GetValue())
	{
		*(this->summaryText) << wxT("Prevalence, ");
		this->BatchStatsTrack[PREVALENCE] = true;
	}
	else
	{
		this->BatchStatsTrack[PREVALENCE] = false;
	}

	if(setupBatchStats->prevalenceSASelect->GetValue())
	{
		*(this->summaryText) << wxT("SA Prevalence, ");
		this->BatchStatsTrack[PREVALENCESA] = true;
	}
	else
	{
		this->BatchStatsTrack[PREVALENCESA] = false;
	}

	if(setupBatchStats->incidenceSelect->GetValue())
	{
		*(this->summaryText) << wxT("Incidence, ");
		this->BatchStatsTrack[INCIDENCE] = true;
	}
	else
	{
		this->BatchStatsTrack[INCIDENCE] = false;
	}

	if(setupBatchStats->PopulationSizeSelect->GetValue())
	{
		*(this->summaryText) << wxT("Population Size, ");
		this->BatchStatsTrack[POPULATION] = true;
	}
	else
	{
		this->BatchStatsTrack[POPULATION] = false;
	}

	if(setupBatchStats->NumInfectedSelect->GetValue())
	{
		*(this->summaryText) << wxT("Number Currently Infected, ");
		this->BatchStatsTrack[CURRENTLYINFECTED] = true;
	}
	else
	{
		this->BatchStatsTrack[CURRENTLYINFECTED] = false;
	}

	if(setupBatchStats->NumNewInfectionsSelect->GetValue())
	{
		*(this->summaryText) << wxT("Number of New Infections");
		this->BatchStatsTrack[NEWINFECTIONS] = true;
	}
	else
	{
		this->BatchStatsTrack[NEWINFECTIONS] = false;
	}

	*(this->summaryText) << wxT("\n");
}

