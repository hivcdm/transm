/*
 * verticalStatusWidget.cpp
 *
 *  Created on: Jul 21, 2009
 *      Author: errhode
 */

#ifndef CONSOLE

#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <wx/wx.h>
#include "verticalStatusWidget.h"
#include "../DisplayBox.h"

int nums[] = { 10, 20, 30, 40, 50, 60, 70, 80, 90 };
int vsize = sizeof(nums)/sizeof(nums[1]);

verticalStatusWidget::verticalStatusWidget(wxPanel *parent, int id, int *trackingVariable, double scaleMultiplier, wxString newlabel)
      : wxPanel(parent, id, wxDefaultPosition, wxSize(35, -1), wxSUNKEN_BORDER)
{

  m_parent = parent;
  statusWidth = trackingVariable;
  this->label = newlabel;
  this->scaleMult = scaleMultiplier;

  Connect(wxEVT_PAINT, wxPaintEventHandler(verticalStatusWidget::OnPaint));
  Connect(wxEVT_SIZE, wxSizeEventHandler(verticalStatusWidget::OnSize));

}

void verticalStatusWidget::OnPaint(wxPaintEvent& event)
{


  wxFont font(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false, wxT("Helvetica"));

  wxPaintDC dc(this);
  dc.SetFont(font);
  wxSize size = GetSize();
  int width = size.GetWidth();
  int height = size.GetHeight();

  int cur_level = *(this->statusWidth);

  int step = (int) ((height / (vsize + 1)) + 0.5);


  int till = (int) ((((vsize + 1) * step) / 100.0) * 100 + 0.5);
  int full = (int) ((((vsize + 1) * step) / 100.0) * cur_level + 0.5);
  //198,113,113 == reddish color ("salmon")
  //93,71,139 == purple
  dc.SetPen(wxPen(wxColour(93, 71, 139)));
  dc.SetBrush(wxBrush(wxColour(93, 71, 139)));
  dc.DrawRectangle(0, till-full, width, full);
  //141, 238, 238 == bluish ("darkslategray2")
  //113, 198, 113 == green
  dc.SetPen(wxPen(wxColour(113, 198, 113)));
  dc.SetBrush(wxBrush(wxColour(113, 198, 113)));
  dc.DrawRectangle(0, 0, width, till-full);

  //90,80,60 == dark grey
  dc.SetPen(wxPen(wxColour(255, 255, 255)));
  dc.SetBrush(wxBrush(wxColour(255, 255, 255)));
  dc.SetTextForeground(wxColour(255,255,255));
  for ( int i=1; i <= vsize; i++ ) {
  //dc.DrawLine(i*step, 0, i*step, 6);
  wxSize size = dc.GetTextExtent(wxString::Format(wxT("%d"), (int) (nums[i-1] * this->scaleMult)));
  dc.DrawText(wxString::Format(wxT("%d"), (int) (nums[i-1] * this->scaleMult)),
      2, (vsize + 1 - i)*step-size.GetHeight()/2);
  }
  //wxString::Format(wxT("Current Percent Infected"))
  size = dc.GetTextExtent(this->label);
  dc.DrawRotatedText(this->label, 16, (vsize + 1)*step/2 + size.GetWidth()/2, 90.0);
}

void verticalStatusWidget::OnSize(wxSizeEvent& event)
{
  Refresh();
}

#endif