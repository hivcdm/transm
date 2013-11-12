project (TRANS)
cmake_minimum_required (VERSION 2.6)

# Set the version number.
set (TRANS_VERSION_MAJOR 3)
set (TRANS_VERSION_MINOR 32)

set (other_SRCS
src/util/ticpp/ticpp.cpp        
src/util/ticpp/tinystr.cpp
src/util/ticpp/tinyxml.cpp 
src/util/ticpp/tinyxmlerror.cpp
src/util/ticpp/tinyxmlparser.cpp 
src/util/rand/RandomNums.cpp
src/util/Util.cpp	
src/statistics/InfectionsTracker.cpp
src/statistics/PopStats.cpp
src/statistics/CostsTracker.cpp
src/statistics/TransmissionSummaryStats.cpp
src/graphViz/graphVizParse.cpp
src/entities/entitypool/bucket/BucketAge.cpp
src/entities/entitypool/bucket/BucketSexualMixing.cpp
src/entities/entitypool/bucket/DmgProfileBucket.cpp
src/entities/entitypool/bucket/FullVector.cpp
src/entities/entitypool/EntityPool.cpp
src/entities/classifiers/DmgProfile.cpp
src/entities/classifiers/SexualPartnership.cpp
src/entities/behaviors/SexualBehaviorParams.cpp
src/entities/Female.cpp
src/entities/Male.cpp
src/entities/Person.cpp
src/data/Enum.cpp
src/cepacBridge/ParseCepacInput.cpp
src/cepacBridge/cepac_api.cpp
src/Constants.cpp
src/Population.cpp
src/PopulationParams.cpp
src/Sim.cpp
src/CEPAC/AcuteOIUpdater.cpp
src/CEPAC/BeginMonthUpdater.cpp
src/CEPAC/BehaviorUpdater.cpp
src/CEPAC/CD4HVLUpdater.cpp
src/CEPAC/CD4TestUpdater.cpp
src/CEPAC/CepacUtil.cpp
src/CEPAC/CHRMsUpdater.cpp
src/CEPAC/ClinicVisitUpdater.cpp
src/CEPAC/DrugEfficacyUpdater.cpp
src/CEPAC/DrugToxicityUpdater.cpp
src/CEPAC/EndMonthUpdater.cpp
src/CEPAC/HIVInfectionUpdater.cpp
src/CEPAC/HIVTestingUpdater.cpp	
src/CEPAC/HVLTestUpdater.cpp
src/CEPAC/MortalityUpdater.cpp
src/CEPAC/mtrand.cpp
src/CEPAC/Patient.cpp
src/CEPAC/RunStats.cpp
src/CEPAC/SimContext.cpp
src/CEPAC/StateUpdater.cpp
src/CEPAC/SummaryStats.cpp
src/CEPAC/TBDiseaseUpdater.cpp
src/CEPAC/Tracer.cpp 
)
set (console_SRCS src/main.cpp)
set (gui_SRCS
src/GUI/widgets/statusWidget.cpp
src/GUI/widgets/verticalStatusWidget.cpp
src/GUI/DisplayBox.cpp
src/GUI/dialogs/SetupBatchStatsDialog.cpp
src/main.cpp
)

# Build the GUI version or the console version?
option (USE_GUI
        "Build graphical user interface" OFF)
if (USE_GUI)
    add_executable(transm332 ${gui_SRCS} ${other_SRCS})
    find_package(wxWidgets REQUIRED adv core base)
    if(wxWidgets_FOUND)
        include(${wxWidgets_USE_FILE})
        include_directories(${wxWidgets_INCLUDE_DIRS})
        target_link_libraries(transm332 ${wxWidgets_LIBRARIES})
    endif(wxWidgets_FOUND)
else()
    add_definitions(-DCONSOLE)
    add_executable(transm332 ${console_SRCS} ${other_SRCS})
endif (USE_GUI)


# --------------------------------------------------------------------------
find_package(Boost)
if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
    target_link_libraries(transm332 ${Boost_LIBRARIES})
endif(Boost_FOUND)
