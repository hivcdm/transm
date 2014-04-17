from statFile import StatFile
from itertools import product
import numpy as np

class CheckSettings:
    def __init__(self):
        self.checkCSWPrev = True
        self.checkHRMalePrev = True
        self.checkHRFemalePrev = True
        self.checkLRMalePrev = True
        self.checkLRFemalePrev = True
        self.checkNumPartCSW = True
        self.checkNumPart = True
        self.checkInc = True
        self.checkCasPrevRatio = True

        self.prevBounds = [[0,1] for i in range(5)]
        self.numPartCSWBounds = [0,1]
        self.numPartBounds = [0,1]
        self.incBounds = [0,1]
        self.casPrevLB = 0

    def getChecks(self):
        return (self.checkCSWPrev, self.checkHRMalePrev, self.checkHRFemalePrev,
                self.checkLRMalePrev, self.checkLRFemalePrev,
                self.checkNumPartCSW, self.checkNumPart, self.checkInc, self.checkCasPrevRatio)
    def setChecks(self, checks):
        self.checkCSWPrev, self.checkHRMalePrev, self.checkHRFemalePrev, self.checkLRMalePrev, self.checkLRFemalePrev, self.checkNumPartCSW, self.checkNumPart, self.checkInc, self.checkCasPrevRatio = checks
    def setBounds(self, lbs, ubs):
        self.prevBounds = [[lbs[i], ubs[i]] for i in range(5)]
        self.numPartCSWBounds = [lbs[5], ubs[5]]
        self.numPartBounds = [lbs[6], ubs[6]]
        self.incBounds = [lbs[7], ubs[7]]
        self.casPrevLB = lbs[8]
#returns SA HIV prev as tuple (prev among CSW, prev among HR Male, prev HR Female, prev LR Male, prev LR Female) among given months
def getPrev(infFile, popFile, months):
    if months is None:
        return [None]*5

    riskNumInfected = [0]*6
    numRisk = [0]*6
    riskPrevalenceHeaders = ("CSW High Risk", "CSW Low Risk", "Non-CSW High Risk Male", "Non-CSW High Risk Female", "Non-CSW Low Risk Male", "Non-CSW Low Risk Female")
    riskGroupHeaders = ("CSW HR", "CSW LR", "Non-CSW High Risk Male", "Non-CSW High Risk Female", "Non-CSW Low Risk Male", "Non-CSW Low Risk Female")

    for month in months:
      riskNumInfected = [riskNumInfected[i]+int(infFile.get_data(header, month)) for i, header in enumerate(riskPrevalenceHeaders)]
      numRisk = [numRisk[i]+int(popFile.get_data(header, month)) for i,header in enumerate(riskGroupHeaders)]

    prevData = []

    #csw
    try:
        prevData.append((riskNumInfected[0]+riskNumInfected[1])/float(numRisk[0]+numRisk[1]))
    except ZeroDivisionError:
        prevData.append(None)

    #hr male
    try:
        prevData.append((riskNumInfected[2])/float(numRisk[2]))
    except ZeroDivisionError:
        prevData.append(None)

    #hr female
    try:
        prevData.append((riskNumInfected[3]+riskNumInfected[0])/float(numRisk[0]+numRisk[3]))
    except ZeroDivisionError:
        prevData.append(None)

    #lr male
    try:
        prevData.append((riskNumInfected[4])/float(numRisk[4]))
    except ZeroDivisionError:
        prevData.append(None)

    #lr female
    try:
        prevData.append((riskNumInfected[5]+riskNumInfected[1])/float(numRisk[1]+numRisk[5]))
    except ZeroDivisionError:
        prevData.append(None)

    return prevData

#returns a tuple of (passes check, value)
def checkPrev(infFile, popFile, months, group,bounds):
    prev = getPrev(infFile, popFile, months)[group]
    return (prev >= bounds[0] and prev <= bounds[1], prev)

#gets num partners per/ group for month returns (num part per csw/mth, num part per person/mth)
def getNumPartners(popFile, partFile, month):
    numCSWPartners = int(partFile.get_data("CSW HR", month))+int(partFile.get_data("CSW Mix", month))+int(partFile.get_data("CSW LR", month))
    numCSW = int(popFile.get_data("CSW HR", month))+int(popFile.get_data("CSW LR", month))

    riskTypes = ("HR", "Mix", "LR")
    partTypes = ("Steady", "Regular", "Casual", "CSW")
    totalNumPartners = 2*sum([int(partFile.get_data(header, month)) for header in [" ".join(pair) for pair in product(partTypes, riskTypes)]])
    numSA = int(popFile.get_data("SA Pop Size",month))

    partData = []
    #part csw
    try:
        partData.append(numCSWPartners/float(numCSW))
    except ZeroDivisionError:
        partData.append(None)

    #part SA
    try:
        partData.append(totalNumPartners/float(numSA))
    except ZeroDivisionError:
        partData.append(None)

    return partData

#returns a tuple of (passes check, value)
def checkNumPartCSW(popFile, partFile, month, bounds):
    numPart = getNumPartners(popFile, partFile, month)[0]
    return (numPart >= bounds[0] and numPart <= bounds[1], numPart)

#returns a tuple of (passes check, value)
def checkNumPart(popFile, partFile, month, bounds):
    numPart = getNumPartners(popFile, partFile, month)[1]
    return (numPart >= bounds[0] and numPart <= bounds[1], numPart)

#returns a tuple of (passes check, value)
def checkInc(infFile, months, bounds):
    inc = getIncidence(infFile, months)
    return (inc >= bounds[0] and inc <= bounds[1], inc)

#returns a tuple of (passes check, value)
def checkCasPrevRatio(popFile, partFile, month, bound):
    prevRatio = getCasualPrevRatio(popFile, partFile, month)
    return (prevRatio >= bound , prevRatio)

#gets incidince per person month over specified months
def getIncidence(infFile, months):
    totalIncidentCases = 0
    totalPersonMonths = 0 #SA pop person months
    for month in months:
        totalIncidentCases += int(infFile.get_data("New Infections", month))
        totalPersonMonths += int(infFile.get_data("SA Pop Size", month))

    try:
        return totalIncidentCases/float(totalPersonMonths)
    except ZeroDivisionError:
        return None

#gets ratio of casual partnership prevelance ratio for females to males over given month
def getCasualPrevRatio(popFile, partFile, month):
    genderTypes = ("Male", "Female")
    riskTypes = ("HR", "LR")
    relationshipTypes = ("Single", "Non-Single")
    cswStatusTypes = ("CSW", "Non-CSW")

    numInCasual = [0, 0] # num of each gender type in a casual
    headers = [" ".join(group) for group in product(genderTypes, relationshipTypes, cswStatusTypes, riskTypes)]
    riskGroupHeaders = ("CSW HR", "CSW LR", "Non-CSW High Risk Male", "Non-CSW High Risk Female", "Non-CSW Low Risk Male", "Non-CSW Low Risk Female")
    numRisk = [0]*6
    numRisk = [numRisk[i]+int(popFile.get_data(header, month)) for i,header in enumerate(riskGroupHeaders)]
    numSA = (numRisk[2]+numRisk[4], numRisk[0]+numRisk[1]+numRisk[3]+numRisk[5])
    for header in headers:
        if "Non-Single" in header:
            n = 3
        else:
            n = 2
        numCas = int(partFile.get_data(header, month, n))
        if "Male" in header:
            numInCasual[0] += numCas
        else:
            numInCasual[1] += numCas

    try:
       return (numInCasual[1]/float(numSA[1]))/(numInCasual[0]/float(numSA[0]))
    except ZeroDivisionError:
        return None
    
