###
#
#Determines the name of the executables. Change for each new version
VERSION = 332

EXE = transm$(VERSION)

#header files
HEADERS = source/util/ticpp/*.h source/util/rand/*.h source/util/*.h source/statistics/*.h source/graphviz/*.h source/entities/entitypool/bucket/*.h source/entities/entitypool/*.h \
	source/entities/classifiers/*.h source/entities/behaviors/*.h source/entities/*.h source/data/*.h source/cepacbridge/*.h source/*.h source/cepac/*.h

# src and .o directories. Add to here when a new file has been added
OBJS = source/util/ticpp/ticpp.o \
	source/util/ticpp/tinystr.o \
	source/util/ticpp/tinyxml.o \
	source/util/ticpp/tinyxmlerror.o \
	source/util/ticpp/tinyxmlparser.o \
	source/util/rand/RandomNums.o \
	source/util/Util.o	\
	source/statistics/InfectionsTracker.o \
	source/statistics/PopStats.o \
	source/statistics/CostsTracker.o	\
	source/statistics/TransmissionSummaryStats.o	\
	source/graphviz/graphVizParse.o \
	source/entities/entitypool/bucket/BucketAge.o \
	source/entities/entitypool/bucket/BucketSexualMixing.o \
	source/entities/entitypool/bucket/DmgProfileBucket.o \
	source/entities/entitypool/bucket/FullVector.o \
	source/entities/entitypool/EntityPool.o \
	source/entities/classifiers/DmgProfile.o \
	source/entities/classifiers/SexualPartnership.o \
	source/entities/behaviors/SexualBehaviorParams.o \
	source/entities/Female.o \
	source/entities/Male.o \
	source/entities/Person.o \
	source/data/Enum.o \
	source/cepacbridge/ParseCepacInput.o \
	source/cepacbridge/cepac_api.o \
	source/Constants.o \
	source/Population.o \
	source/PopulationParams.o \
	source/Sim.o \
	source/cepac/AcuteOIUpdater.o	\
	source/cepac/BeginMonthUpdater.o	\
	source/cepac/BehaviorUpdater.o	\
	source/cepac/CD4HVLUpdater.o	\
	source/cepac/CD4TestUpdater.o	\
	source/cepac/CepacUtil.o	\
	source/cepac/CHRMsUpdater.o	\
	source/cepac/ClinicVisitUpdater.o	\
	source/cepac/DrugEfficacyUpdater.o	\
	source/cepac/DrugToxicityUpdater.o	\
	source/cepac/EndMonthUpdater.o	\
	source/cepac/HIVInfectionUpdater.o	\
	source/cepac/HIVTestingUpdater.o	\
	source/cepac/HVLTestUpdater.o	\
	source/cepac/MortalityUpdater.o	\
	source/cepac/mtrand.o	\
	source/cepac/Patient.o	\
	source/cepac/RunStats.o	\
	source/cepac/SimContext.o	\
	source/cepac/StateUpdater.o	\
	source/cepac/SummaryStats.o	\
	source/cepac/TBDiseaseUpdater.o	\
	source/cepac/Tracer.o 

GUI_OBJS = $(OBJS) source/gui/widgets/statusWidget.o \
	source/gui/widgets/verticalStatusWidget.o \
	source/gui/DisplayBox.o \
	source/gui/main.o \
	source/gui/dialogs/SetupBatchStatsDialog.o 

CONSOLE_OBJS = $(OBJS) source/main.o

#compiler and related flags
CXX = g++
CXXFLAGS = -I.
LDFLAGS = -lm
#Note: This is a temp directory on the cluster until they update to a newer version of boost
BOOST_CXXFLAGS = -Ithirdparty/boost/1.55.0
BOOST_LDFLAGS = -Lthirdparty/boost/1.55.0/stage/lib -lboost_regex
BOOST_MAC_CXXFLAGS = -I/usr/local/include/boost_1_36_0
BOOST_MAC_LDFLAGS = -L/usr/local/lib -lboost_regex-xgcc40-mt
#For now we are going to ignore the WX_**FLAGS because we are only using the Makefile for console versions
#WX_CXXFLAGS = -I/usr/local/lib/wx/include/msw-ansi-release-static-2.8 -I/usr/local/include/wx-2.8 -D__WXMSW__
#WX_LDFLAGS = -L/usr/local/lib -Wl,--subsystem,windows -mwindows /usr/local/lib/libwx_msw_richtext-2.8.a /usr/local/lib/libwx_msw_aui-2.8.a /usr/local/lib/libwx_msw_xrc-2.8.a /usr/local/lib/libwx_msw_qa-2.8.a /usr/local/lib/libwx_msw_html-2.8.a /usr/local/lib/libwx_msw_adv-2.8.a /usr/local/lib/libwx_msw_core-2.8.a /usr/local/lib/libwx_base_xml-2.8.a /usr/local/lib/libwx_base_net-2.8.a /usr/local/lib/libwx_base-2.8.a /usr/local/lib/boost_regex-mgw-mt.a -lwxregex-2.8 -lwxexpat-2.8 -lwxtiff-2.8 -lwxjpeg-2.8 -lwxpng-2.8 -lwxzlib-2.8 -lrpcrt4 -loleaut32 -lole32 -luuid -lwinspool -lwinmm -lshell32 -lcomctl32 -lcomdlg32 -lctl3d32 -ladvapi32 -lwsock32 -lgdi32 
#WX_CXXFLAGS_DEBUG = -I/usr/local/lib/wx/include/msw-ansi-debug-static-2.8 -I/usr/local/include/wx-2.8 -D__WXDEBUG__ -D__WXMSW__
#WX_LDFLAGS_DEBUG = -L/usr/local/lib -Wl,--subsystem,windows -mwindows /usr/local/lib/libwx_mswd_richtext-2.8.a /usr/local/lib/libwx_mswd_aui-2.8.a /usr/local/lib/libwx_mswd_xrc-2.8.a /usr/local/lib/libwx_mswd_qa-2.8.a /usr/local/lib/libwx_mswd_html-2.8.a /usr/local/lib/libwx_mswd_adv-2.8.a /usr/local/lib/libwx_mswd_core-2.8.a /usr/local/lib/libwx_based_xml-2.8.a /usr/local/lib/libwx_based_net-2.8.a /usr/local/lib/libwx_based-2.8.a /usr/local/lib/boost_regex-mgw-mt-d.a -lwxregexd-2.8 -lwxexpatd-2.8 -lwxtiffd-2.8 -lwxjpegd-2.8 -lwxpngd-2.8 -lwxzlibd-2.8 -lrpcrt4 -loleaut32 -lole32 -luuid -lwinspool -lwinmm -lshell32 -lcomctl32 -lcomdlg32 -lctl3d32 -ladvapi32 -lwsock32 -lgdi32 

#rules for compiling C and CPP files
.SUFFIXES: .c .cpp

%.o : %.c $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o : %.rc
	windres $^ -o $@


##################
# These are the only args that should be called from the command line
##################

all : console

console : CXXFLAGS += -DCONSOLE -O3 $(BOOST_CXXFLAGS)
console : LDFLAGS += $(BOOST_LDFLAGS)
console : $(CONSOLE_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $(EXE)

console_mac : CXXFLAGS += -DCONSOLE -O3 $(BOOST_MAC_CXXFLAGS)
console_mac : LDFLAGS += $(BOOST_MAC_LDFLAGS)
console_mac : $(CONSOLE_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $(EXE)

console_debug : CXXFLAGS += -g -O0 $(BOOST_CXXFLAGS)
console_debug : $(CONSOLE_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $(EXE)

gui : CXXFLAGS += -O3 $(BOOST_CXXFLAGS) $(WX_CXXFLAGS)
gui : LDFLAGS += $(WX_LDFLAGS)
gui : $(GUI_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $(EXE)

gui_debug : CXXFLAGS += -g -O0 $(BOOST_CXXFLAGS) $(WX_CXXFLAGS_DEBUG)
gui_debug : LDFLAGS += $(WX_LDFLAGS_DEBUG)
gui_debug : $(GUI_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $(EXE)

clean : 
	rm -f source/*.o source/cepacbridge/*.o source/data/*.o source/entities/*.o source/entities/behaviors/*.o source/entities/classifiers/*.o source/entities/entitypool/*.o source/entities/entitypool/bucket/*.o source/graphviz/*.o source/statistics/*.o source/util/*.o source/util/rand/*.o source/util/ticpp/*.o source/gui/*.o source/gui/widgets/*.o source/gui/dialogs/*.o source/cepac/*.o
	rm -f $(EXE) $(EXE).exe



