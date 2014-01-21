###
#
#Determines the name of the executables. Change for each new version
VERSION = 333a

EXE = transm$(VERSION)

#header files
HEADERS = source/util/ticpp/*.h source/util/rand/*.h source/util/*.h source/statistics/*.h source/graphviz/*.h source/entities/entitypool/bucket/*.h source/entities/entitypool/*.h source/entities/classifiers/*.h source/entities/behaviors/*.h source/entities/*.h source/data/*.h source/cepacbridge/*.h source/*.h source/cepac/*.h source/util/sqlite/*.h

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
	source/cepac/Tracer.o \
	source/statistics/ArtRolloutTracker.o \
	source/statistics/TabularOutput.o \
	source/statistics/BucketCounter.o \
	source/util/sqlite/sqlite3.o

GUI_OBJS = $(OBJS) source/gui/widgets/statusWidget.o \
	source/gui/widgets/verticalStatusWidget.o \
	source/gui/DisplayBox.o \
	source/gui/main.o \
	source/gui/dialogs/SetupBatchStatsDialog.o 

CONSOLE_OBJS = $(OBJS) source/main.o

#compiler and related flags
CC = gcc
CFLAGS = -I. -O3 -D__LINUX__
CXX = g++
CXXFLAGS = -I. -O3 -std=c++11 -D__LINUX__ -I/PHShome/nna6/build/boost_1_55_0/
LDFLAGS = -lm -ldl -lpthread

#rules for compiling C and CPP files
.SUFFIXES: .c .cpp

%.o : %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o : %.rc
	windres $^ -o $@


##################
# These are the only args that should be called from the command line
##################

all : console

console : CXXFLAGS += -DCONSOLE $(BOOST_CXXFLAGS)
console : LDFLAGS += $(BOOST_LDFLAGS)
console : $(CONSOLE_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $(EXE)

gui : CXXFLAGS += `wx-config --cxxflags` -D__WXDEBUG__
gui : LDFLAGS += `wx-config --libs`
gui : $(GUI_OBJS)
	$(CXX) $(CXXFLAGS) $^ $(LDFLAGS) -o $(EXE)

clean : 
	rm -f source/*.o source/cepacbridge/*.o source/data/*.o source/entities/*.o source/entities/behaviors/*.o source/entities/classifiers/*.o source/entities/entitypool/*.o source/entities/entitypool/bucket/*.o source/graphviz/*.o source/statistics/*.o source/util/*.o source/util/rand/*.o source/util/ticpp/*.o source/gui/*.o source/gui/widgets/*.o source/gui/dialogs/*.o source/cepac/*.o source/util/sqlite/*.o
	rm -f $(EXE)


