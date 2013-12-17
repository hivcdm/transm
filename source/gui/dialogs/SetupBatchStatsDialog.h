/*
 * SetupBatchStatsDialog.h
 *
 *  Created on: Oct 28, 2009
 *      Author: errhode
 */

#ifndef SETUPPOPSTATSDIALOG_H_
#define SETUPPOPSTATSDIALOG_H_

#include <wx/wx.h>
#include "../../Constants.h"

class DisplayBox;

class SetupBatchStatsDialog : public wxDialog
{
public:
  SetupBatchStatsDialog(const wxString& title, DisplayBox *dbox);

  wxCheckBox *prevalenceSelect;
  wxCheckBox *prevalenceSASelect;
  wxCheckBox *incidenceSelect;
  wxCheckBox *PopulationSizeSelect;
  wxCheckBox *NumInfectedSelect;
  wxCheckBox *NumNewInfectionsSelect;

};



#endif /* SETUPPOPSTATSDIALOG_H_ */
