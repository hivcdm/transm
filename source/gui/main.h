/*
 * main.h
 *
 *  Created on: Jun 5, 2009
 *      Author: errhode
 */

#ifndef MAIN_H_
#define MAIN_H_
#ifndef CONSOLE

#include <wx/wx.h>

class MyApp : public wxApp
{
  public:
    virtual bool OnInit();
};

#endif
#endif /* MAIN_H_ */
