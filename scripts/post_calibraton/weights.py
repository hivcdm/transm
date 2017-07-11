from statFile import StatFile
from checkFuncs import *
import likelihood as Likelihood

import os
from os import path
import argparse
import numpy as np
import pylab
import math

def fitPrevNoShift(monthlyPrev, UNAIDSYearlyPrev, UNAIDSHigh, UNAIDSLow):
    fitValues = []
    for month in range(len(monthlyPrev)):
        try:
            modelYearlyPrev = np.array([monthlyPrev[month+12*i] for i in range(len(UNAIDSYearlyPrev))])
        except IndexError:
            break
        sumDiffSquaresModified = np.sum(((modelYearlyPrev - UNAIDSYearlyPrev)/(UNAIDSHigh-UNAIDSLow))**2)
        fitValues.append(sumDiffSquaresModified)
    
    return np.argmin(fitValues), np.min(fitValues)

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

def perform_checks(root, rootName, checkSettings):
    infFile = StatFile(path.join(root,rootName+"-Infections.xls"),2)
    popFile = StatFile(path.join(root,rootName+"-Population.xls"),1)
    partFile = StatFile(path.join(root,rootName+"-Partnership.xls"),1)
    
    prevChecks = [checkPrev(infFile, popFile, yearOf2002,i, checkSettings.prevBounds[i])for i in range(5)]
    numPartCSW = checkNumPartCSW(popFile, partFile, monthOfEnd2002, checkSettings.numPartCSWBounds)
    numPart = checkNumPart(popFile, partFile, monthOfEnd2002, checkSettings.numPartBounds)
    inc = checkInc(infFile, yearOf2002, checkSettings.incBounds)
    casPrevRatio = checkCasPrevRatio(popFile, partFile, monthOfEnd2002, checkSettings.casPrevLB)
    
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
    if passedChecks:
        storedRuns[rootName] = (monthOf1990, saPrev, likelihood)
 
    worstRun = (None, 1)
    if len(storedRuns.keys()) > numToStore:
        for run in storedRuns.keys():
            if storedRuns[run][2] < worstRun[1]:
                worstRun = (run, storedRuns[run][2])
            del storedRuns[worstRun]

    return [saPrev, prevChecks, numPartCSW, numPart, inc, casPrevRatio]

#does checks and fitting for single file
def calculate_likelihood(root, rootName):
    infFilePath = path.join(root,rootName+"-Infections.xls")

    # log likelihood for Botswana calibration
    logLikelihood = Likelihood.calculate_log_likelihood(infFilePath)

    return logLikelihood

def calculate_weights(runs, index):
    #calc likelihood weights all runs
    # Add the maximum log likelihood: LLi = lli + ABS(max(ll))
    # Then unlog: Li = e^LLi
    maxLogLikelihood = runs[0]['logLikelihood']
    for run in runs:
        try:
            maxLogLikelihood = max(maxLogLikelihood, run['logLikelihood'])
        except:
            pass
    for run in runs:
        try:
            shifted_logLikelihood = run['logLikelihood'] - maxLogLikelihood
            likelihood = math.exp(shifted_logLikelihood)
            run['likelihood'] = likelihood
        except ValueError: 
            print("Math error")

    # Finally calculate the sum likelihood and divide the likelihood by the sum to get the weight
    # Wi = Li/sum(L)
    sumLikelihood = 0
    for run in runs:
        try:
            sumLikelihood += run['likelihood']
        except:
            pass
    for run in runs:
        try:
            run[index] = run['likelihood'] / sumLikelihood
        except:
            run[index] = None

def postCalib(rootFolderPath, checkSettings, numToStore, outText = None):
    checksToDo=checkSettings.getChecks()
    baseCheckHeaders = ("HIV Prev CSW", "HIV Prev HR Male", "HIV Prev HR Female",
                    "HIV Prev LR Male", "HIV Prev LR Female",
                        "Num Part CSW per Month", "Num Part per Month", "2 year Incidence", "Casual Prev Ratio(f/m)")
    checkHeaders = []
    for i,header in enumerate(baseCheckHeaders):
        if checksToDo[i]:
            checkHeaders.append("Passes " + header)
            checkHeaders.append(header)

    postCalibFile = open("post_calib_new_weights.out","w")
    postCalibFile.write("run name\tMonth Of 1990\tLikelihood\tWeight All Runs\tWeight Passed Runs\t")
    postCalibFile.write("\t".join([header for i,header in enumerate(checkHeaders)]))
    postCalibFile.write("\n")

    storedRuns = {}
    allRuns = []
    for root, dirs, files in os.walk(rootFolderPath):
        for inputFile in [f for f in files if f.endswith("-Infections.xls")]:
            try:
                runData = {}
                rootName = path.splitext(inputFile)[0].split("-")[0]
                runData['name'] = rootName

                if outText:
                    outText.SetStatusText(rootName)
                #monthOf1990, saPrev, logLikelihood, prevChecks, numPartCSW, numPart, inc, casPrevRatio = \
                #analyseSingle(root, rootName, checkSettings)
                if not args.no_checks:
                    saPrev, prevChecks, numPartCSW, numPart, inc, casPrevRatio = \
                    perform_checks(root, rootName, checkSettings)

                runData['1990'] = monthOf1990
                runData['logLikelihood'] = calculate_likelihood(root, rootName)

                allRuns.append(runData)
            except:
                pass

    with open('prevData_new_weights.out','w') as fStore:
        fStore.write("Run Name\tMonth of 1990\tLikelihood\tSA Prev Data\n")
        for run in storedRuns.keys():
            fStore.write("{}\t{}\t{}".format(run, storedRuns[run][0], storedRuns[run][2]))
            for prev in storedRuns[run][1]:
                fStore.write("\t{}".format(prev))
            fStore.write("\n")
 
    #calc likelihood weights all runs
    calculate_weights(allRuns, 'weight_all')
    
    #calc likelihood weights passed runs
    if not args.no_checks:
        passedRuns = [run for run in allRuns if run['passedChecks']]
        calculate_weights(passedRuns, 'weight_passed')

    for run in allRuns:
        postCalibFile.write("{}\t{}\t{}\t{}\t{}".format(run['name'],run.get('1990',None), run['logLikelihood'], run['likelihood'],run.get('weight_all',None), run.get('weight_passed',None)))
        if not args.no_checks:
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

# Main Entry
if __name__ == '__main__':
    #Parse arguments from the command line
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', action='store', help='Directory containing results data')
    parser.add_argument('-o', '--output', action='store', help='Directory to output summary')
    parser.add_argument('-x', '--no-checks', action='store_true', help='Do not perform checks; only weights')
    parser.add_argument('--debug', action='store_true', default=False)
    args = parser.parse_args()

    monthOf1990=600

    if args.debug:
        print(args)
 
    data_dir = args.directory
    postCalib(data_dir,CheckSettings(),10)

    #plotRuns(args.output)
