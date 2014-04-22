#include <sstream>

#include "main.h"
#include "DisplayBox.h"
#include "../util/Utility.h"

IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	std::stringstream DisplayBoxHeader;
	DisplayBoxHeader << "CEPAC Population Model Version " << Version::ToString(Utility::MODEL_VERSION);
	wxString wxDisplayBoxHeader(DisplayBoxHeader.str().c_str(), wxConvUTF8);
	DisplayBox *sizer = new DisplayBox(wxDisplayBoxHeader);
	sizer->Show(true);
	return true;
}
