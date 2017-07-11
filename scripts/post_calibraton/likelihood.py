import numpy as np
import math
import pandas as pd
import scipy.misc as sc

# BAIS Prevalence by sex and age group (20 to 49yo, five-year intervals) for each year surveyed
trialsMales2004=(866,693,559,449,362,276)
successMales2004=(79,159,202,149,122,87)
trialsFemales2004=(928,723,495,385,346,299)
successFemales2004=(243,297,216,146,97,83)

trialsMales2008=(795,743,580,453,364,282)
successMales2008=(59,119,166,169,159,78)
trialsFemales2008=(847,780,535,357,297,275)
successFemales2008=(136,265,262,153,114,86)

trialsMales2013=(545,519,477,357,273,215)
successMales2013=(27,69,128,125,119,92)
trialsFemales2013=(582,545,461,299,199,173)
successFemales2013=(85,148,186,151,79,72)

trials=[]
trials.extend(list(trialsMales2004))
trials.extend(list(trialsFemales2004))
trials.extend(list(trialsMales2008))
trials.extend(list(trialsFemales2008))
trials.extend(list(trialsMales2013))
trials.extend(list(trialsFemales2013))

successes=[]
successes.extend(list(successMales2004))
successes.extend(list(successFemales2004))
successes.extend(list(successMales2008))
successes.extend(list(successFemales2008))
successes.extend(list(successMales2013))
successes.extend(list(successFemales2013))

def calculate_log_likelihood(file):
    # load the data from the results file
    infData=pd.read_csv(file, sep='\t')
    headers=1
    prevData = infData.ix[:,'Prevalence By Age and Gender':'Prevalent Cases By Risk']
    prevData.drop(prevData.columns[len(prevData.columns)-1], axis=1, inplace=True)

    # should be able to parse the age ranges from the file
    #ranges=[]
    #ranges=set(prevData.iloc[headers]).sort()

    prevalences=[]

    firstMaleIndex=3 # Males '240-299' months
    lastMaleIndex=8  # Males '540-599' months
    firstFemaleIndex=12 # Females '240-299' months
    lastFemaleIndex=17  # Females '540-599' months
    for year in [2004,2008,2013]:
        month=600+(year-1990)*12+(headers+1)
        idx=firstMaleIndex
        while idx <= lastMaleIndex:
            prevalences.extend([prevData.iloc[month].get_value(idx)])
            idx+=1
        idx=firstFemaleIndex
        while idx <= lastFemaleIndex:
            prevalences.extend([prevData.iloc[month].get_value(idx)])
            idx+=1

    data=[trials,successes,prevalences]

    loglikelihood=0
    idx=0
    while idx < len(data):
        D=data[0][idx]
        N=data[1][idx]
        P=float(data[2][idx])

        try:
            # log likelihood for Botswana calibration
            loglikelihood+=N*math.log10(P)+(D-N)*math.log10(1-P)
        except ValueError:
            print("Prevalence value less than or equal to 0.0")
        idx+=1
    
    return loglikelihood

if __name__ == '__main__':
    calculate_log_likelihood(sys.argv[1])

