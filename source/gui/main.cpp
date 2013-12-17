/*
 * main.cpp
 *
 *  Created on: Jun 5, 2009
 *      Author: errhode
 */

//Reinclude and compile in visual C++ in debug mode if you want to do memory leak detection
/*#include <vld.h>
#include <vldapi.h>*/
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "main.h"
#include "DisplayBox.h"
#include "../util/Util.h"


using namespace std;


IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	stringstream DisplayBoxHeader;
	DisplayBoxHeader << "CEPAC Population Model Version " << Util::MODEL_VERSION;
	wxString wxDisplayBoxHeader(DisplayBoxHeader.str().c_str(), wxConvUTF8);

    DisplayBox *sizer = new DisplayBox(wxDisplayBoxHeader);
    sizer->Show(true);

    return true;
}
