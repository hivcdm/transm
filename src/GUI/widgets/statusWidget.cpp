/*
 * statusWidget.cpp
 *
 *  Created on: Jun 8, 2009
 *      Author: errhode
 */

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <wx/wx.h>
#include "statusWidget.h"
#include "../DisplayBox.h"

int num[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90 };
int asize = sizeof(num)/sizeof(num[1]);

StatusWidget::StatusWidget(wxPanel *parent, int id, int *trackingVariable, wxString newlabel)
      : wxPanel(parent, id, wxDefaultPosition, wxSize(-1, 35), wxSUNKEN_BORDER)
{

  m_parent = parent;
  statusWidth = trackingVariable;
  this->label = newlabel;

  Connect(wxEVT_PAINT, wxPaintEventHandler(StatusWidget::OnPaint));
  Connect(wxEVT_SIZE, wxSizeEventHandler(StatusWidget::OnSize));

}

void StatusWidget::OnPaint(wxPaintEvent& event)
{


  wxFont font(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false, wxT("Helvetica"));

  wxPaintDC dc(this);
  dc.SetFont(font);
  wxSize size = GetSize();
  int width = size.GetWidth();

  //DisplayBox *dbox = (DisplayBox *) (m_parent->GetParent())->GetParent();

  int cur_prev = *(this->statusWidth);

  //*(dbox->textctrl) << wxT("\t Prevalence is ") << cur_prev << wxT("\n");

  int step = (int) (width / 10.0 + 0.5);


  int till = (int) ((width / 100.0) * 100);
  int full = (int) ((width / 100.0) * cur_prev);
  dc.SetPen(wxPen(wxColour(198, 113, 113)));
  dc.SetBrush(wxBrush(wxColour(198, 113, 113)));
  dc.DrawRectangle(0, 0, full, 30);
  dc.SetPen(wxPen(wxColour(141, 238, 238)));
  dc.SetBrush(wxBrush(wxColour(141, 238, 238)));
  dc.DrawRectangle(full, 0, till-full, 30);

  dc.SetPen(wxPen(wxColour(90, 80, 60)));
  for ( int i=1; i <= asize; i++ ) {
  //dc.DrawLine(i*step, 0, i*step, 6);
  wxSize size = dc.GetTextExtent(wxString::Format(wxT("%d"), num[i-1]));
  dc.DrawText(wxString::Format(wxT("%d"), num[i-1]),
      i*step-size.GetWidth()/2, 2);
  }
  //wxString::Format(wxT("Current Percent Infected"))
  size = dc.GetTextExtent(this->label);
  dc.DrawText(this->label, 5*step - size.GetWidth()/2, 16);
}

void StatusWidget::OnSize(wxSizeEvent& event)
{
  Refresh();
}


