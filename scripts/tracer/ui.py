from PyQt5 import QtCore, QtGui, QtWidgets

class Ui_TracerWindow(object):
    def setupUi(self, main_window):
        main_window.setObjectName("main_window")
        main_window.resize(835, 699)
        main_window.setWindowTitle("Tracer 0.2.0")
        
        central_widget = QtWidgets.QWidget(main_window)
        central_widget.setObjectName("central_widget")
        
        horizontalLayout_2 = QtWidgets.QHBoxLayout(central_widget)
        horizontalLayout_2.setObjectName("horizontalLayout_2")
        
        mainHorizontalLayout = QtWidgets.QHBoxLayout()
        mainHorizontalLayout.setObjectName("mainHorizontalLayout")
        
        leftHorizontalLayout = QtWidgets.QHBoxLayout()
        leftHorizontalLayout.setSpacing(6)
        leftHorizontalLayout.setSizeConstraint(QtWidgets.QLayout.SetMinimumSize)
        leftHorizontalLayout.setObjectName("leftHorizontalLayout")

        self.filter_selectors = self.build_filter_selectors(central_widget)
        leftHorizontalLayout.addLayout(self.filter_selectors)
        
        self.person_selector = self.build_person_selector(central_widget)
        leftHorizontalLayout.addLayout(self.person_selector)
        
        leftHorizontalLayout.setStretch(1, 2)
        mainHorizontalLayout.addLayout(leftHorizontalLayout)
        self.plot = self.build_plot(central_widget)
        mainHorizontalLayout.addWidget(self.plot)
        mainHorizontalLayout.setStretch(1, 1)
        horizontalLayout_2.addLayout(mainHorizontalLayout)
        
        main_window.setCentralWidget(central_widget)

        self.status_bar = self.build_status_bar(main_window)
        main_window.setStatusBar(self.status_bar)

        self.menu_bar = self.build_menu_bar(main_window)
        main_window.setMenuBar(self.menu_bar)

        QtCore.QMetaObject.connectSlotsByName(main_window)

    def get_gender_filter(self):
        enabled = self.gender_filter_group.isChecked()
        selected = self.male_radio_button.isChecked() or self.female_radio_button.isChecked()
        gender = 'MALE' if self.male_radio_button.isChecked() else 'FEMALE'
        
        return enabled and selected, gender

    def get_csw_filter(self):
        enabled = self.csw_filter_group.isChecked()
        selected = self.csw_yes_radio_button.isChecked() or self.csw_no_radio_button.isChecked()
        csw = self.csw_yes_radio_button.isChecked()

        return enabled and selected, csw

    def get_hiv_filter(self):
        enabled = self.hiv_filter_group.isChecked()
        selected = self.positive_radio_button.isChecked() or self.negative_radio_button.isChecked()
        hiv = self.positive_radio_button.isChecked()

        return enabled and selected, hiv

    def get_risk_filter(self):
        enabled = self.risk_filter_group.isChecked()
        selected = self.high_risk_radio_button.isChecked() or self.low_risk_radio_button.isChecked()
        risk = 'HIGH' if self.high_risk_radio_button.isChecked() else 'LOW'

        return enabled and selected, risk

    def build_filter_selectors(self, parent):
        filterWrapperVerticalLayout = QtWidgets.QVBoxLayout()
        filterWrapperVerticalLayout.setSpacing(0)
        filterWrapperVerticalLayout.setContentsMargins(-1, -1, -1, 0)
        filterWrapperVerticalLayout.setObjectName("filterWrapperVerticalLayout")
        filterFrame = QtWidgets.QFrame(parent)
        filterFrame.setMinimumSize(QtCore.QSize(120, 400))
        filterFrame.setFrameShape(QtWidgets.QFrame.StyledPanel)
        filterFrame.setFrameShadow(QtWidgets.QFrame.Raised)
        filterFrame.setObjectName("filterFrame")
        widget = QtWidgets.QWidget(filterFrame)
        widget.setGeometry(QtCore.QRect(0, 0, 122, 359))
        widget.setObjectName("widget")
        filterVerticalLayout = QtWidgets.QVBoxLayout(widget)
        filterVerticalLayout.setContentsMargins(0, 0, 0, 0)
        filterVerticalLayout.setObjectName("filterVerticalLayout")
        filterLabel = QtWidgets.QLabel(widget)
        filterLabel.setEnabled(True)
        filterLabel.setMaximumSize(QtCore.QSize(16777215, 13))
        filterLabel.setAlignment(QtCore.Qt.AlignCenter)
        filterLabel.setObjectName("filterLabel")
        filterVerticalLayout.addWidget(filterLabel)
        self.gender_filter_group = QtWidgets.QGroupBox(widget)
        self.gender_filter_group.setMinimumSize(QtCore.QSize(120, 80))
        self.gender_filter_group.setMaximumSize(QtCore.QSize(120, 80))
        self.gender_filter_group.setAlignment(QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.gender_filter_group.setCheckable(True)
        self.gender_filter_group.setChecked(False)
        self.gender_filter_group.setObjectName("gender_filter_group")
        verticalLayout_2 = QtWidgets.QVBoxLayout(self.gender_filter_group)
        verticalLayout_2.setObjectName("verticalLayout_2")
        self.male_radio_button = QtWidgets.QRadioButton(self.gender_filter_group)
        self.male_radio_button.setObjectName("male_radio_button")
        verticalLayout_2.addWidget(self.male_radio_button)
        self.female_radio_button = QtWidgets.QRadioButton(self.gender_filter_group)
        self.female_radio_button.setObjectName("female_radio_button")
        verticalLayout_2.addWidget(self.female_radio_button)
        filterVerticalLayout.addWidget(self.gender_filter_group)
        self.csw_filter_group = QtWidgets.QGroupBox(widget)
        self.csw_filter_group.setMinimumSize(QtCore.QSize(120, 80))
        self.csw_filter_group.setMaximumSize(QtCore.QSize(120, 80))
        self.csw_filter_group.setAlignment(QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.csw_filter_group.setCheckable(True)
        self.csw_filter_group.setChecked(False)
        self.csw_filter_group.setObjectName("csw_filter_group")
        self.csw_yes_radio_button = QtWidgets.QRadioButton(self.csw_filter_group)
        self.csw_yes_radio_button.setGeometry(QtCore.QRect(10, 30, 103, 21))
        self.csw_yes_radio_button.setObjectName("csw_yes_radio_button")
        self.csw_no_radio_button = QtWidgets.QRadioButton(self.csw_filter_group)
        self.csw_no_radio_button.setGeometry(QtCore.QRect(10, 50, 103, 21))
        self.csw_no_radio_button.setObjectName("csw_no_radio_button")
        filterVerticalLayout.addWidget(self.csw_filter_group)
        self.hiv_filter_group = QtWidgets.QGroupBox(widget)
        self.hiv_filter_group.setMinimumSize(QtCore.QSize(120, 80))
        self.hiv_filter_group.setMaximumSize(QtCore.QSize(120, 80))
        self.hiv_filter_group.setAlignment(QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.hiv_filter_group.setCheckable(True)
        self.hiv_filter_group.setChecked(False)
        self.hiv_filter_group.setObjectName("hiv_filter_group")
        verticalLayout_4 = QtWidgets.QVBoxLayout(self.hiv_filter_group)
        verticalLayout_4.setObjectName("verticalLayout_4")
        self.positive_radio_button = QtWidgets.QRadioButton(self.hiv_filter_group)
        self.positive_radio_button.setObjectName("positive_radio_button")
        verticalLayout_4.addWidget(self.positive_radio_button)
        self.negative_radio_button = QtWidgets.QRadioButton(self.hiv_filter_group)
        self.negative_radio_button.setObjectName("negative_radio_button")
        verticalLayout_4.addWidget(self.negative_radio_button)
        filterVerticalLayout.addWidget(self.hiv_filter_group)
        self.risk_filter_group = QtWidgets.QGroupBox(widget)
        self.risk_filter_group.setMinimumSize(QtCore.QSize(120, 80))
        self.risk_filter_group.setMaximumSize(QtCore.QSize(120, 80))
        self.risk_filter_group.setAlignment(QtCore.Qt.AlignLeading|QtCore.Qt.AlignLeft|QtCore.Qt.AlignTop)
        self.risk_filter_group.setCheckable(True)
        self.risk_filter_group.setChecked(False)
        self.risk_filter_group.setObjectName("risk_filter_group")
        verticalLayout_5 = QtWidgets.QVBoxLayout(self.risk_filter_group)
        verticalLayout_5.setObjectName("verticalLayout_5")
        self.high_risk_radio_button = QtWidgets.QRadioButton(self.risk_filter_group)
        self.high_risk_radio_button.setObjectName("high_risk_radio_button")
        verticalLayout_5.addWidget(self.high_risk_radio_button)
        self.low_risk_radio_button = QtWidgets.QRadioButton(self.risk_filter_group)
        self.low_risk_radio_button.setObjectName("low_risk_radio_button")
        verticalLayout_5.addWidget(self.low_risk_radio_button)
        filterVerticalLayout.addWidget(self.risk_filter_group)
        filterWrapperVerticalLayout.addWidget(filterFrame)

        filterLabel.setText("Filter")
        self.gender_filter_group.setTitle("Gender")
        self.male_radio_button.setText("Male")
        self.female_radio_button.setText("Female")
        self.csw_filter_group.setTitle("CSW Status")
        self.csw_yes_radio_button.setText("Yes")
        self.csw_no_radio_button.setText("No")
        self.hiv_filter_group.setTitle("HIV Status")
        self.positive_radio_button.setText("Positive")
        self.negative_radio_button.setText("Negative")
        self.risk_filter_group.setTitle("Risk")
        self.high_risk_radio_button.setText("High")
        self.low_risk_radio_button.setText("Low")

        return filterWrapperVerticalLayout

    def build_person_selector(self, parent):
        personSelectorVerticalLayout = QtWidgets.QVBoxLayout()
        personSelectorVerticalLayout.setObjectName("personSelectorVerticalLayout")
        personSelectorId = QtWidgets.QLabel(parent)
        personSelectorId.setAlignment(QtCore.Qt.AlignCenter)
        personSelectorId.setObjectName("personSelectorId")
        personSelectorVerticalLayout.addWidget(personSelectorId)
        self.personSelectorList = QtWidgets.QListWidget(parent)
        self.personSelectorList.setMinimumSize(QtCore.QSize(120, 0))
        self.personSelectorList.setMaximumSize(QtCore.QSize(120, 16777215))
        self.personSelectorList.setObjectName("personSelectorList")
        personSelectorVerticalLayout.addWidget(self.personSelectorList)

        personSelectorId.setText("ID")

        return personSelectorVerticalLayout

    def build_plot(self, parent):
        plot = MatplotlibWidget(parent)
        plot.setMinimumSize(QtCore.QSize(640, 480))
        plot.setObjectName("plot")

        return plot

    def build_menu_bar(self, parent):
        menu_bar = QtWidgets.QMenuBar(parent)
        menu_bar.setGeometry(QtCore.QRect(0, 0, 835, 21))
        menu_bar.setObjectName("menu_bar")
        
        menu_file = QtWidgets.QMenu(menu_bar)
        menu_file.setObjectName("menu_file")
        
        self.action_open_trace = QtWidgets.QAction(parent)
        self.action_open_trace.setObjectName("action_open_trace")
        self.action_save_plot = QtWidgets.QAction(parent)
        self.action_save_plot.setObjectName("action_save_plot")
        self.action_close = QtWidgets.QAction(parent)
        self.action_close.setObjectName("action_close")
        
        menu_file.addAction(self.action_open_trace)
        menu_file.addAction(self.action_save_plot)
        menu_file.addSeparator()
        menu_file.addAction(self.action_close)
        menu_bar.addAction(menu_file.menuAction())

        menu_file.setTitle("File")
        self.action_open_trace.setText("Open Trace")
        self.action_save_plot.setText("Save Plot")
        self.action_close.setText("Close")

        return menu_bar

    def build_status_bar(self, parent):
        status_bar = QtWidgets.QStatusBar(parent)
        status_bar.setEnabled(True)
        status_bar.setObjectName("status_bar")

        return status_bar

from matplotlibwidget import MatplotlibWidget
