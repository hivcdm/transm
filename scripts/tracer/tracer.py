import sys
from PyQt5 import QtWidgets
from tracer_ui import Ui_TracerWindow
import ZoomPan
import os
import trace_parser
import numpy as np
import traceback

class Tracer(object):
    def __init__(self):
        self.app = QtWidgets.QApplication(sys.argv)
        self.window = QtWidgets.QMainWindow()
        self.ui = Ui_TracerWindow()
        
        self.ui.setupUi(self.window)
        
        scale = 1.1
        zp = ZoomPan.ZoomPan()
        figZoom = zp.zoom_factory(self.ui.plot.figure.gca(), base_scale = scale)
        figPan = zp.pan_factory(self.ui.plot.figure.gca())

        self.ui.action_open_trace.triggered.connect(self.open_file)
        self.ui.action_save_plot.triggered.connect(self.save_file)
        self.ui.action_close.triggered.connect(self.app.quit)
        self.ui.personSelectorList.itemClicked.connect(self.choosePersonToPlot)

        self.ui.gender_filter_group.clicked.connect(self.populateList)
        self.ui.male_radio_button.toggled.connect(self.populateList)
        self.ui.female_radio_button.toggled.connect(self.populateList)

        self.ui.csw_filter_group.clicked.connect(self.populateList)
        self.ui.csw_no_radio_button.toggled.connect(self.populateList)
        self.ui.csw_yes_radio_button.toggled.connect(self.populateList)

        self.ui.hiv_filter_group.clicked.connect(self.populateList)
        self.ui.positive_radio_button.toggled.connect(self.populateList)
        self.ui.negative_radio_button.toggled.connect(self.populateList)

        self.ui.risk_filter_group.clicked.connect(self.populateList)
        self.ui.high_risk_radio_button.toggled.connect(self.populateList)
        self.ui.low_risk_radio_button.toggled.connect(self.populateList)

    def run(self):
        self.window.show()
        sys.exit(self.app.exec_())

    def load_file(self, filename):

        try:
            self.simulation = trace_parser.TraceSimulation(filename)
        except Exception as e:
            errorDialog = QtWidgets.QErrorMessage(self.window)
            errorDialog.setModal(True)
            errorDialog.showMessage(str(e))
            return

        msg = 'Loaded {0}...'.format(filename)
        self.ui.status_bar.showMessage(msg)

        self.tracedPopulation = [x for x in self.simulation.population.person.keys()
                                 if self.simulation.population.person[x].traced]
        self.populateList()

    def open_file(self):
        #homedir = os.path.expanduser('~')
        homedir = r'C:\Users\taf656\Desktop\Development\tracer\Calibration Paper Runs\6th Batch Results'
##        filename = QtWidgets.QFileDialog.getOpenFileName(self.window,
##                                               'Open XML Control File',
##                                               homedir,
##                                               "XML files (*.xml)")
        filename = (homedir+os.sep+'Try03Cal_Batch14_9468.xml',0)

        if len(filename) == 0:
            return

        msg = 'Loading {0}...'.format(filename[0])
        self.ui.status_bar.showMessage(msg)
        self.load_file(filename[0])

    def save_file(self):
        """Should be able to save to PDF."""
        homedir = os.path.expanduser('~')
        filename = QFileDialog.getSaveFileName(self.window,
                                               'Save File as PDF',
                                               homedir,
                                               "Images (*.pdf)")

        if len(filename) == 0:
            return

        plt.savefig(str(filename), format='pdf') 

    def choosePersonToPlot(self):
        """Plot the person chosen in the list widget.
        """
        row = self.ui.personSelectorList.currentRow()
        item = self.ui.personSelectorList.item(row)
        id = item.text()
        print('selected %s' % id)
        key = int(id)

        # The xlimits are determined by the current value of the sliders.
        xlim = [0, 1200]

        try:
            self.simulation.plot_person(key, self.ui.plot, xlim)
        except Exception as e:
            errorDialog = QtWidgets.QErrorMessage(self.window)
            errorDialog.setModal(True)
            errorDialog.showMessage(str(e))
            return

    def populateList(self):
        """Subselect the population based on factors determined by GUI.
        """
        print('populating list...')
        self.ui.personSelectorList.clear()

        tracedPopulation = self.tracedPopulation
        fullPopulation = self.simulation.population
        print("Initial traced population is %d" % len(tracedPopulation))

        gender_filter = self.ui.get_gender_filter()
        if (gender_filter[0]):
            print('selecting on gender...')
            tracedPopulation = [x for x in tracedPopulation
                                if fullPopulation.person[x].gender == gender_filter[1]]

        csw_filter = self.ui.get_csw_filter()
        if (csw_filter[0]):
            print('selecting on CSW status...')
            tracedPopulation = [x for x in tracedPopulation
                                if fullPopulation.person[x].ever_csw == csw_filter[1]]

        hiv_filter = self.ui.get_hiv_filter()
        if (hiv_filter[0]):
            print('selecting on HIV status...')
            tracedPopulation = [x for x in tracedPopulation
                                if fullPopulation.person[x].hiv_positive == hiv_filter[1]]
            print("%d of them are HIV positive." % len(tracedPopulation))
            # Now sort by infection time.
            time_of_infection = [fullPopulation.person[key].time_of_infection
                                 for key in tracedPopulation]
            sort_index = np.argsort(time_of_infection)
            tracedPopulation = [tracedPopulation[idx] for idx in sort_index]

        risk_filter = self.ui.get_risk_filter()
        if (risk_filter[0]):
            print('selecting on risk...')
            tracedPopulation = [x for x in tracedPopulation
                                if fullPopulation.person[x].risk == risk_filter[1]]

        print('number of people in subset:  %d' % len(tracedPopulation))
        for id in tracedPopulation:
            self.ui.personSelectorList.addItem(str(id))

def main():
    Tracer().run()
 
if __name__ == '__main__':
    main()
