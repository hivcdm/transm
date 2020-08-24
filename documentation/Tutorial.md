# CDM Coding Tutorial

In this tutorial covers basic coding practices of the CDM through an example of adding demographic distirbutions to an exisiting input. A major intervention in the fight against HIV/AIDS is anti-retroviral therapy (ART). The CDM includes an intervention for modelling the rollout of ART in the population of study. In most locations, ART resources were or are still limited and increasing access to treatment will be key to reducing incidence and prevalence of the disease.

New additions to the population demographics in the model allow us to vary ART access across age, gender, sexual orientation, race and ethnicity within the study population. This tutorial covers the inputs and code areas required to enable researchers to specify ART access across these demographics. 

## Adding Demographic Distributions to Target Rollout Proportions

The CDM takes input parameters and variables from an XML file. For this tutorial we will focus on the [Interventions](https://wiki.harvard.edu/confluence/display/k95973/Input+Sheet+Walkthrough+v4.5) section of the XML. The current XML for ART access is under the ART Intervention:

```
<simulation version="4.5">
  ...
  <interventions>
    <artRolloutIntervention enabled="true">
      ...
      <targetRolloutProportions>
        <target year="2001">0.01</target>
        <target year="2002">0.02</target>
        <target year="2003">0.06</target>
        ...
     </targetRolloutProportions>
   </artRolloutIntervention>
  </interventions>
</simulation>
```

Each target in the `targetRolloutProportion` section of the XML needs to be updated with a demographic distribution:

```
<simulation version="4.5">
  ...
  <interventions>
    <artRolloutIntervention enabled="true">
      ...
      <targetRolloutProportions>
        <target year="2001" value="0.01">
          <demographicDistributions>
            <gender MALE="0.486" FEMALE="0.514"/>
            <orientation MSW="0.92" MSM="0.07" MSMW="0.01"/>
            <raceAndEthnicityProfiles>
              <raceAndEthnicity type="WHITE:NON_HISPANIC">0.13</raceAndEthnicity>
              <raceAndEthnicity type="WHITE:HISPANIC">0.70</raceAndEthnicity>
              <raceAndEthnicity type="BLACK:NON_HISPANIC">0.15</raceAndEthnicity>
              <raceAndEthnicity type="OTHER:NON_HISPANIC">0.02</raceAndEthnicity>
            </raceAndEthnicityProfiles>
          </demographicDistributions>
        </target>
        ...
     </targetRolloutProportions>
   </artRolloutIntervention>
  </interventions>
</simulation>
```


The CDM takes inputs from an XML file which get read in to the model by the code in the 'parameters' folder. The main class to read in these parameters is the Simulation Parameters class. In a snippet of the header file for this class, we see 
