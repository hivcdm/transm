import os
from glob import glob
from os import path
from lxml import etree

#parameterNames is a list of tuples of the format (parameterName, xpath syntax used to extract parameter)
malePrefix="//simulation/population/entities/entity[@type='Male']"
femalePrefix="//simulation/population/entities/entity[@type='Female']"
parameterNames={ "Assort" :        malePrefix+"/behavior/partnershipTypes/partnership[@type='Steady']/assortativeness"\
                ,"StdyMultLR" :    malePrefix+"/behavior/partnerAcqMultWithSteadyLowRisk"\
                ,"PropHRMale" :    malePrefix+"/behavior/proportionHighRiskNonCsw"\
                ,"HRMult" :        malePrefix+"/behavior/highRiskAcqRateMultiplier"\
                ,"CSWMult" :       malePrefix+"/behavior/highRiskCswAcqRateMultiplier"\
                ,"PropHRFemale" :  femalePrefix+"/behavior/proportionHighRiskNonCsw"\
                ,"ChanceCSW" :     femalePrefix+"/behavior/chanceBecomeSexWorker"\
                ,"RegActs" :       malePrefix+"/behavior/partnershipTypes/partnership[@type='Regular']/coitalEventsPerMonthHighRisk/distribution/mean"\
                ,"AqRateStdyHR" :  malePrefix+"/behavior/partnershipTypes/partnership[@type='Steady']/acquisitionRateHighRisk/distribution/mean"\
                ,"AqRateStdyLR" :  malePrefix+"/behavior/partnershipTypes/partnership[@type='Steady']/acquisitionRateLowRisk/distribution/mean"\
                ,"AqRateRegHR" :   malePrefix+"/behavior/partnershipTypes/partnership[@type='Regular']/acquisitionRateHighRisk/distribution/mean"\
                ,"AqRateRegLR" :   malePrefix+"/behavior/partnershipTypes/partnership[@type='Regular']/acquisitionRateLowRisk/distribution/mean"\
                ,"AqRateCasHR" :   malePrefix+"/behavior/partnershipTypes/partnership[@type='Casual']/acquisitionRateHighRisk/distribution/mean"\
                ,"AqRateCasLR" :   malePrefix+"/behavior/partnershipTypes/partnership[@type='Casual']/acquisitionRateLowRisk/distribution/mean"\
                ,"AqRateCSWHR" :   malePrefix+"/behavior/partnershipTypes/partnership[@type='Csw']/acquisitionRateHighRisk/distribution/mean"\
                ,"AqRateCSWLR" :   malePrefix+"/behavior/partnershipTypes/partnership[@type='Csw']/acquisitionRateLowRisk/distribution/mean"\
              }

"""extracts necessary parameters from files in the folder"""
def extractAll(folderName):
    distribFileDict={}
    fdistrib=open(path.join(folderName,"distrib_new.out"),'w')
    
    fdistrib.write("\t"+"\t".join(parameterNames.keys())+"\n")
    count=0

    fileNames = [] #list of filenames to compare
    for root,dirs,files in os.walk(folderName):
        for xmlFile in files:
            if ".xml" not in xmlFile:
                continue
            if xmlFile in fileNames:
                continue

            fileNames.append(xmlFile)
            filePath = path.join(root,xmlFile)
            fdistrib.write(path.basename(xmlFile).strip())
            for parameter in extractSingle(filePath):
                fdistrib.write("\t"+parameter[1])
            fdistrib.write("\n")
            print(path.basename(xmlFile))

    fdistrib.close()
             
"""extracts necessary parameters from single file"""
def extractSingle(filePath):
    tree=etree.parse(filePath)
    return getXmlParameters(tree,parameterNames)

def getXmlParameters(tree,parameterNames):
    parameters=[]
    for key,value in parameterNames.items():
        parameters.append((key,tree.xpath(value)[0].text))
    return parameters
    
if __name__=='__main__':
    extractAll(os.getcwd())
    
