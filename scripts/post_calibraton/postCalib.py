from checkFuncs import *
import os
from os import path
import numpy as np
import pylab
import math

def fitPrevWithShift(monthlyPrev, UNAIDSYearlyPrev, UNAIDSHigh, UNAIDSLow):
    fitValues = []
    for month in range(len(monthlyPrev)):
        try:
            modelYearlyPrev = np.array([monthlyPrev[month+12*i] for i in range(len(UNAIDSYearlyPrev))])
        except IndexError:
            break
        sumDiffSquaresModified = np.sum(((modelYearlyPrev - UNAIDSYearlyPrev)/(UNAIDSHigh-UNAIDSLow))**2)
        fitValues.append(sumDiffSquaresModified)
    
    return np.argmin(fitValues), np.min(fitValues)
#does checks and fitting for single file
def analyseSingle(root, rootName, checkSettings):
    infFile = StatFile(path.join(root,"results",rootName+"-Infections.xls"),2)
    popFile = StatFile(path.join(root,"results",rootName+"-Population.xls"),1)
    partFile = StatFile(path.join(root,"results",rootName+"-Partnership.xls"),2)
    saPrev=np.array(infFile.get_all("SA Prevalence"), dtype=float)
    
    UNAIDSPrev = 0.01*np.array([0.5,0.8,1.3,2.1,3.3,4.9,6.9,9.2,11.4,13.3,14.8,15.9,16.6])
#    UNAIDSHighPrev = 0.01*np.array([0.7,1.0,1.6,2.4,3.6,5.3,7.5,9.9,12.3,14.2,15.7,16.8,17.4])
#    UNAIDSLowPrev = 0.01*np.array([0.4,0.6,1.1,1.8,2.9,4.4,6.4,8.5,10.6,12.4,14.0,15.1,15.8])
    UNAIDSHighPrev = 0.01*np.array([0.8, 1.1, 1.7, 2.5, 3.7, 5.3, 7.5, 9.8, 12.2, 14.0, 15.4, 16.4, 17.0])
    UNAIDSLowPrev = 0.01*np.array([0.4, 0.5, 1.0, 1.7, 2.8, 4.4, 6.4, 8.6, 10.7, 12.6, 14.3, 15.4, 16.2])
    monthOf1990, goodnessOfFit= fitPrevWithShift(saPrev, UNAIDSPrev, UNAIDSHighPrev, UNAIDSLowPrev)

    likelihood = math.exp(-1*goodnessOfFit/(2.0*checkSettings.sigma**2))
    monthOf2002 = monthOf1990+144
    monthOfEnd2002 = monthOf2002+11
    yearOf2002 = range(monthOf2002,monthOf2002+12)

    prevChecks = [checkPrev(infFile, popFile, yearOf2002,i, checkSettings.prevBounds[i])for i in range(5)]
    numPartCSW = checkNumPartCSW(popFile, partFile, monthOfEnd2002, checkSettings.numPartCSWBounds)
    numPart = checkNumPart(popFile, partFile, monthOfEnd2002, checkSettings.numPartBounds)
    inc = checkInc(infFile, yearOf2002, checkSettings.incBounds)
    casPrevRatio = checkCasPrevRatio(popFile, partFile, monthOfEnd2002, checkSettings.casPrevLB)
    return [monthOf1990, saPrev, likelihood, prevChecks, numPartCSW, numPart, inc, casPrevRatio]


    #pylab.plot(range(len(saPrev)), saPrev)
    #pylab.plot([12*i for i in range(len(UNAIDSPrev))], UNAIDSPrev)
    #pylab.plot([i - monthOf1990 for i in range(len(saPrev))],saPrev)
    #pylab.show()
    #checkCSWPrev(infFile, popFile, yearOf2002 ,0,1)
def postCalib(rootFolderPath, checkSettings, numToStore, weightBeforeChecks = True, outText = None):
    checksToDo=checkSettings.getChecks()
    baseCheckHeaders = ("HIV Prev CSW", "HIV Prev HR Male", "HIV Prev HR Female",
                    "HIV Prev LR Male", "HIV Prev LR Female",
                        "Num Part CSW per Month", "Num Part per Month", "2 year Incidence", "Casual Prev Ratio(f/m)")
    checkHeaders = []
    for i,header in enumerate(baseCheckHeaders):
        if checksToDo[i]:
            checkHeaders.append("Passes " + header)
            checkHeaders.append(header)

    postCalibFile = open("post calib new weights.out","w")
    postCalibFile.write("run name\tMonth Of 1990\tLikelihood\tWeight All Runs\tWeight Passed Runs\t")
    postCalibFile.write("\t".join([header for i,header in enumerate(checkHeaders)]))
    postCalibFile.write("\n")

    storedRuns = {}
    allRuns = []
    for root, dirs, files in os.walk(rootFolderPath):
        for inputFile in [f for f in files if f.endswith(".xml")]:
            try:
                runData = {}
                rootName = path.splitext(inputFile)[0]
                runData['name'] = rootName

                if outText:
                    outText.SetStatusText(rootName)
                monthOf1990, saPrev, likelihood, prevChecks, numPartCSW, numPart, inc, casPrevRatio = analyseSingle(root, rootName, checkSettings)
                runData['1990'] = monthOf1990
                runData['likelihood'] = likelihood
                checks = []
                checks.extend(prevChecks)
                checks.extend((numPartCSW, numPart, inc, casPrevRatio))
                runData['checks'] = checks


                unfilteredChecks = prevChecks
                unfilteredChecks.append(numPartCSW)
                unfilteredChecks.append(numPart)
                unfilteredChecks.append(inc)
                unfilteredChecks.append(casPrevRatio)

                filteredChecks = [check for i,check in enumerate(unfilteredChecks) if checksToDo[i]]
                passedChecks = True
                for check in filteredChecks:
                    if not check[0]:
                        passedChecks = False
                        break
                runData['passedChecks'] = passedChecks
                allRuns.append(runData)
                if weightBeforeChecks or passedChecks:
                    storedRuns[rootName] = (monthOf1990, saPrev, likelihood)
                worstRun = (None, 1)
                if len(storedRuns.keys()) > numToStore:
                    for run in storedRuns.keys():
                        if storedRuns[run][2] < worstRun[1]:
                            worstRun = (run, storedRuns[run][2])
                    del storedRuns[worstRun]
            except:
                pass

    with open('prevData new weights.out','w') as fStore:
        fStore.write("Run Name\tMonth of 1990\tLikelihood\tSA Prev Data\n")
        for run in storedRuns.keys():
            fStore.write("{}\t{}\t{}".format(run, storedRuns[run][0], storedRuns[run][2]))
            for prev in storedRuns[run][1]:
                fStore.write("\t{}".format(prev))
            fStore.write("\n")
    #calc likelihood weights all runs
    sumLikelihood = 0
    for run in allRuns:
        try:
            sumLikelihood+= run['likelihood']
        except:
            pass
    
    for run in allRuns:
        try:
            run['weight_all'] = run['likelihood']/sumLikelihood
        except:
            run['weight_all'] = None

    #calc likelihood weights passed runs
    passedRuns = [run for run in allRuns if run['passedChecks']]
    sumLikelihood = 0
    for run in passedRuns:
        try:
            sumLikelihood+= run['likelihood']
        except:
            pass
    
    for run in passedRuns:
        try:
            run['weight_passed'] = run['likelihood']/sumLikelihood
        except:
            run['weight_passed'] = None

    for run in allRuns:
        postCalibFile.write("{}\t{}\t{}\t{}\t{}".format(run['name'],run.get('1990',None), run['likelihood'],run.get('weight_all',None), run.get('weight_passed',None)))
        unfilteredChecks = run['checks']
        filteredChecks = [check for i,check in enumerate(unfilteredChecks) if checksToDo[i]]
        for check in filteredChecks:
            postCalibFile.write("\t{}\t{}".format(check[0],check[1]))
        postCalibFile.write("\n")
    postCalibFile.close()
    return storedRuns
def plotRuns(storeFilePath):
    with open(storeFilePath) as fStore:
        allData=fStore.readlines()[1:]
    plotRuns = {}
    for row in allData:
        cells = row.split("\t")
        plotRuns[cells[0]]={}
        plotRuns[cells[0]]['prev']= [float(prev) for prev in cells[2:]]
        plotRuns[cells[0]]['1990']= int(cells[1])
    UNAIDSPrev = 0.01*np.array([0.7,1.2,1.8,2.9,4.3,6.1,8.4,10.7,12.9,14.8,16.1,17.1,17.7])
    pylab.plot([12*i for i in range(len(UNAIDSPrev))], UNAIDSPrev, 'x')
    for run in plotRuns.keys():
        pylab.plot([i - plotRuns[run]['1990'] for i in range(len(plotRuns[run]['prev']))],plotRuns[run]['prev'],label = run)
    
    pylab.legend(loc=2)
    pylab.xlabel('Months From 1990',fontsize='large')
    pylab.ylabel('SA Prevalence',fontsize='large')
    
    pylab.show()
if __name__=="__main__":
    postCalib(r"C:\Documents and Settings\Administrator\My Documents\Downloads\NadiasFitRuns\NadiasFitRuns",CheckSettings(),10)
    #plotRuns(r"C:\taige\scripts\post calibration\prevData.out")
