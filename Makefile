###
#
#Determines the name of the executables. Change for each new version
VERSION = 332

EXE = transm$(VERSION)

#header files
HEADERS = src/util/ticpp/*.h src/util/rand/*.h src/util/*.h src/statistics/*.h src/graphViz/*.h src/entities/entitypool/bucket/*.h src/entities/entitypool/*.h \
	src/entities/classifiers/*.h src/entities/behaviors/*.h src/entities/*.h src/data/*.h src/cepacBridge/*.h src/*.h src/CEPAC/*.h

# src and .o directories. Add to here when a new file has been added
OBJS = src/util/ticpp/ticpp.o \
	src/util/ticpp/tinystr.o \
	src/util/ticpp/tinyxml.o \
	src/util/ticpp/tinyxmlerror.o \
	src/util/ticpp/tinyxmlparser.o \
	src/util/rand/RandomNums.o \
	src/util/Util.o	\
	src/statistics/InfectionsTracker.o \
	src/statistics/PopStats.o \
	src/statistics/CostsTracker.o	\
	src/statistics/TransmissionSummaryStats.o	\
	src/graphViz/graphVizParse.o \
	src/entities/entitypool/bucket/BucketAge.o \
	src/entities/entitypool/bucket/BucketSexualMixing.o \
	src/entities/entitypool/bucket/DmgProfileBucket.o \
	src/entities/entitypool/bucket/FullVector.o \
	src/entities/entitypool/EntityPool.o \
	src/entities/classifiers/DmgProfile.o \
	src/entities/classifiers/SexualPartnership.o \
	src/entities/behaviors/SexualBehaviorParams.o \
	src/entities/Female.o \
	src/entities/Male.o \
	src/entities/Person.o \
	src/data/Enum.o \
	src/cepacBridge/ParseCepacInput.o \
	src/cepacBridge/cepac_api.o \
	src/Constants.o \
	src/Population.o \
	src/PopulationParams.o \
	src/Sim.o \
	src/CEPAC/AcuteOIUpdater.o	\
	src/CEPAC/BeginMonthUpdater.o	\
	src/CEPAC/BehaviorUpdater.o	\
	src/CEPAC/CD4HVLUpdater.o	\
	src/CEPAC/CD4TestUpdater.o	\
	src/CEPAC/CepacUtil.o	\
	src/CEPAC/CHRMsUpdater.o	\
	src/CEPAC/ClinicVisitUpdater.o	\
	src/CEPAC/DrugEfficacyUpdater.o	\
	src/CEPAC/DrugToxicityUpdater.o	\
	src/CEPAC/EndMonthUpdater.o	\
	src/CEPAC/HIVInfectionUpdater.o	\
	src/CEPAC/HIVTestingUpdater.o	\
	src/CEPAC/HVLTestUpdater.o	\
	src/CEPAC/MortalityUpdater.o	\
	src/CEPAC/mtrand.o	\
	src/CEPAC/Patient.o	\
	src/CEPAC/RunStats.o	\
	src/CEPAC/SimContext.o	\
	src/CEPAC/StateUpdater.o	\
	src/CEPAC/SummaryStats.o	\
	src/CEPAC/TBDiseaseUpdater.o	\
	src/CEPAC/Tracer.o 

GUI_OBJS = $(OBJS) src/GUI/widgets/statusWidget.o \
	src/GUI/widgets/verticalStatusWidget.o \
	src/GUI/DisplayBox.o \
	src/GUI/main.o \
	src/GUI/dialogs/SetupBatchStatsDialog.o 

CONSOLE_OBJS = $(OBJS) src/main.o

#compiler and related flags
CXX = g++
CXXFLAGS = -I.
LDFLAGS = -lm
#Note: This is a temp directory on the cluster until they update to a newer version of boost
BOOST_CXXFLAGS = -I/shr/home/th841/boost/boost_1_44_0
BOOST_LDFLAGS = -L/usr/lib -lboost_regex
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
	rm -f src/*.o src/cepacBridge/*.o src/data/*.o src/entities/*.o src/entities/behaviors/*.o src/entities/classifiers/*.o src/entities/entitypool/*.o src/entities/entitypool/bucket/*.o src/graphViz/*.o src/statistics/*.o src/util/*.o src/util/rand/*.o src/util/ticpp/*.o src/GUI/*.o src/GUI/widgets/*.o src/GUI/dialogs/*.o src/CEPAC/*.o
	rm -f $(EXE) $(EXE).exe



