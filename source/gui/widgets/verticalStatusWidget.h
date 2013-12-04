/*
 * verticalStatusWidget.h
 *
 *  Created on: Jul 21, 2009
 *      Author: errhode
 */

#ifndef VERTICALSTATUSWIDGET_H_
#define VERTICALSTATUSWIDGET_H_
#ifndef CONSOLE

#include <wx/wx.h>
#include <iostream>
#include <stdlib.h>
#include <stdio.h>

class verticalStatusWidget : public wxPanel
{
public:
/*
 * @parameters: parent -- the parent panel containing this widget, id -- the id for this widget,
 * trackingVariable -- a pointer to the variable used to determine the "fullness" of the scale (should be a number between 0 and 100),
 * scaleMultiplier -- a multiplier for the scale listed on the widget (1 --> 10, 20, 30, 40, etc; 10 --> 100, 200, etc.),
 * newlabel -- label for the widget
 *
 */
  verticalStatusWidget(wxPanel *parent, int id, int *trackingVariable, double scaleMultiplier, wxString newlabel );

  //Parent panel
  wxPanel *m_parent;
  //Tracking variable
  int *statusWidth;
  //English label
  wxString label;
  //Scale Multiplier
  double scaleMult;


  void OnSize(wxSizeEvent& event);
  void OnPaint(wxPaintEvent& event);

};

#endif
#endif /* VERTICALSTATUSWIDGET_H_ */
