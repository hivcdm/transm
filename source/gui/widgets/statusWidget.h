/*
 * statusWidget.h
 *
 *  Created on: Jun 8, 2009
 *      Author: errhode
 */

#ifndef STATUSWIDGET_H_
#define STATUSWIDGET_H_

#include <wx/wx.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

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


#endif /* STATUSWIDGET_H_ */
