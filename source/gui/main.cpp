#include <sstream>
#include <util/Utility.h>

#include "main.h"
#include "DisplayBox.h"

using namespace std;


IMPLEMENT_APP(MyApp)

bool MyApp::OnInit()
{
	stringstream DisplayBoxHeader;
	DisplayBoxHeader << "CEPAC Population Model Version " << Version::ToString(Utility::MODEL_VERSION);
	wxString wxDisplayBoxHeader(DisplayBoxHeader.str().c_str(), wxConvUTF8);
	DisplayBox *sizer = new DisplayBox(wxDisplayBoxHeader);
	sizer->Show(true);
	return true;
}
