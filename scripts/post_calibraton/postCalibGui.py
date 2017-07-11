import os
from os import path
import wx
import postCalib
import re

class MainWindow(wx.Frame):
    def __init__(self, parent, title):
        wx.Frame.__init__(self, parent, title=title,size=(500,500))
        self.sb=self.CreateStatusBar() # A StatusBar in the bottom of the window
        panel=wx.Panel(self,-1,size=(-1,-1))
        
        self.chooseFolderTxt = wx.TextCtrl(panel, size=(300,-1))
        self.sigmaTxt=wx.TextCtrl(panel,size=(40,-1))
        self.sigmaTxt.SetValue(".7071")
        self.numToStoreTxt=wx.TextCtrl(panel,size=(30,-1))
        self.weightBeforeChecksCB = wx.CheckBox(panel, -1,"calculate weight before doing checks (for storage)")
        cbLabels = ("CSW Prev", "HR Male Prev", "HR Female Prev",
                    "LR Male Prev", "LR Female Prev", "Num Part CSW per Mth",
                    "Num Part per Mth", "2 year Incidence","Casual Prev Ratio")
        self.calibCheckCBs = [wx.CheckBox(panel,-1,label) for label in cbLabels]
        self.calibCheckLBs = [wx.TextCtrl(panel,size=(30,-1)) for label in cbLabels]
        self.calibCheckUBs = [wx.TextCtrl(panel,size=(30,-1)) for label in cbLabels[:-1]]
        self.calibCheckUBs.append((30,-1))
        chooseFolderBtn=wx.Button(panel,1,label="folder")
        startCalibBtn=wx.Button(panel,2,label="start")
        plotBtn=wx.Button(panel,3,label="plot")
        self.Bind(wx.EVT_BUTTON,self.OnChooseFolder,id=1)
        self.Bind(wx.EVT_BUTTON,self.OnStartCalib,id=2)
        self.Bind(wx.EVT_BUTTON, self.OnPlot, id=3)
        
        vbox=wx.BoxSizer(wx.VERTICAL)

        hbox=wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(self.chooseFolderTxt,0,wx.RIGHT|wx.LEFT,10)
        hbox.Add(chooseFolderBtn,0)
        vbox.Add(hbox,0,wx.TOP|wx.BOTTOM,5)

        hbox=wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(self.sigmaTxt,0,wx.RIGHT|wx.LEFT,10)
        hbox.Add(wx.StaticText(panel,-1,"sigma"),1)
        vbox.Add(hbox,0,wx.TOP|wx.BOTTOM,5)

        hbox=wx.BoxSizer(wx.HORIZONTAL)
        hbox.Add(self.numToStoreTxt,0,wx.RIGHT|wx.LEFT,10)
        hbox.Add(wx.StaticText(panel,-1,"num to store"))
        vbox.Add(hbox,0,wx.TOP|wx.BOTTOM,5)

        vbox.Add(self.weightBeforeChecksCB,0,wx.TOP|wx.BOTTOM, 5)
        gs = wx.GridSizer(10,3,1,1)

        gs.Add(wx.StaticText(panel,-1,"Check"),0)
        gs.Add(wx.StaticText(panel,-1,"Lower Bound"),0)
        gs.Add(wx.StaticText(panel,-1,"Upper Bound"),0)
        for i,cb in enumerate(cbLabels):
            gs.Add(self.calibCheckCBs[i], 0)
            gs.Add(self.calibCheckLBs[i], 0)
            gs.Add(self.calibCheckUBs[i], 0)

        vbox.Add(gs, 0)

        vbox.Add(startCalibBtn,0,wx.TOP|wx.BOTTOM|wx.LEFT,5)
        vbox.Add(plotBtn,0,wx.TOP|wx.BOTTOM|wx.LEFT,5)
        panel.SetSizer(vbox)
        
        panel.SetSizer(vbox)

        # Setting up the menu
        filemenu= wx.Menu()

        # wx.ID_ABOUT and wx.ID_EXIT are standard ids provided by wxWidgets.
        menuAbout = filemenu.Append(wx.ID_ABOUT, "&About"," Information about this program")
        menuExit = filemenu.Append(wx.ID_EXIT,"E&xit"," Terminate the program")

        # Creating the menubar
        menuBar = wx.MenuBar()
        menuBar.Append(filemenu,"&File") # Adding the "filemenu" to the MenuBar
        self.SetMenuBar(menuBar)  # Adding the MenuBar to the Frame content.

        # Set events.
        self.Bind(wx.EVT_MENU, self.OnAbout, menuAbout)
        self.Bind(wx.EVT_MENU, self.OnExit, menuExit)

        self.Show(True)

    def OnStartCalib(self,evt):
        filePath=self.chooseFolderTxt.GetValue()
                 
        try:
            numToStore=int(self.numToStoreTxt.GetValue())
        except ValueError:
            dlg = wx.MessageDialog( self, "Please input a valid number for num to store", "Input Error", wx.OK)
            dlg.ShowModal() # Show it
            dlg.Destroy() # finally destroy it when finished.
            return
                 
        try:
            sigma=float(self.sigmaTxt.GetValue())
        except ValueError:
            dlg = wx.MessageDialog( self, "Please input a valid number for sigma", "Input Error", wx.OK)
            dlg.ShowModal() # Show it
            dlg.Destroy() # finally destroy it when finished.
            return

        
        lbs = []
        ubs = []
        for lbTxt in self.calibCheckLBs:
            try:
                lb = float(lbTxt.GetValue())
            except ValueError:
                lb = 0.0
            lbs.append(lb)
            
        for ubTxt in self.calibCheckUBs[:-1]:
            try:
                ub = float(ubTxt.GetValue())
            except ValueError:
                ub = 1.0
            ubs.append(ub)
        checkSettings = postCalib.CheckSettings()
        checkSettings.setChecks([cb.GetValue() for cb in self.calibCheckCBs])

        checkSettings.setBounds(lbs, ubs)
        checkSettings.sigma = sigma
        os.chdir(filePath)

        postCalib.postCalib(filePath,checkSettings, numToStore, self.weightBeforeChecksCB.GetValue(),self.sb)
        
        dlg = wx.MessageDialog( self, "Calibration Weighting Finished", "Finished Weighting", wx.OK)
        dlg.ShowModal() # Show it
        dlg.Destroy() # finally destroy it when finished.
    def OnPlot(self,evt):
        filePath=self.chooseFolderTxt.GetValue()
        filePath=path.join(filePath,'prevData.out')
        postCalib.plotRuns(filePath)
    def OnChooseFolder(self,evt):
        dlg=wx.DirDialog(self,"Choose a top level folder",os.getcwd())
        if dlg.ShowModal() == wx.ID_OK:
            self.chooseFolderTxt.SetValue(dlg.GetPath())
        dlg.Destroy()
    def OnAbout(self,e):
        dlg = wx.MessageDialog( self, "A program for analyzing post calibration runs", "About Bayesian Melding App", wx.OK)
        dlg.ShowModal() # Show it
        dlg.Destroy() # finally destroy it when finished.

    def OnExit(self,e):
        self.Close(True)  # Close the frame.

app = wx.App(False)
frame = MainWindow(None, "Post Calibration")
app.MainLoop()
