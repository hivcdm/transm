#pragma once

#include <wx/wx.h>

class StatusWidget : public wxPanel
{
public:
	StatusWidget(wxPanel *parent, int id, int *trackingVariable, wxString newlabel);

	wxPanel *m_parent;
	int *statusWidth;
	wxString label;

	void OnSize(wxSizeEvent &event);
	void OnPaint(wxPaintEvent &event);

};
