# TRANSM Coding Tutorial

In this tutorial covers basic coding practices of TRANSM through an example of adding demographic distirbutions to an exisiting input. A major intervention in the fight against HIV/AIDS is anti-retroviral therapy (ART). TRANSM includes an intervention for modelling the rollout of ART in the population of study. In most locations, ART resources were or are still limited and increasing access to treatment will be key to reducing incidence and prevalence of the disease.

New additions to the population demographics in the model allow us to vary ART access across age, gender, sexual orientation, race and ethnicity within the study population. This tutorial covers the inputs and code areas required to enable researchers to specify ART access across these demographics. 

## Adding Demographic Distributions to Target Rollout Proportions

TRANSM takes input parameters and variables from an XML file. For this tutorial we will focus on the [Interventions](https://wiki.harvard.edu/confluence/display/k95973/Input+Sheet+Walkthrough+v4.5) section of the XML. The current XML for ART access is under the ART Intervention:

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

Each `target` in the `targetRolloutProportion` section of the XML needs to be updated with a demographic distribution so that the ART rollout. In the example XML below, in the model timestep associated with the year 2001, one percent of individuals who have tested positive for HIV will receive ART. By gender, the split will be 48.6% male and 51.4% female. By orientation, 92% of MSWs will receive ART, where as only 7% and 1% of MSM and MSMW will receive ART, respectively.  The racial and ethnic breakdown of ART allocation will be 70% White/Hispanic, 15% Black/Non-Hispanic, 13% White/Non-Hispanic, and 2% Other:

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

## Locating the Parameters in the code base

The TRANSM code reads the XML file as input in a class called `simulationparameters` which is defined in the the [parameters](https://github.com/hsphcdm/transm/tree/master-v4/source/parameters) folder. The header file for this class declares a virtual function `GetCepacParameters()` which is implemented in class definition `simulationparametersxml.cpp`. As shown below, the function reads the `artRolloutIntervention` section of the XML including the  `targetRolloutProportions` node into the `target_yearly_rollout_proportions` which is an attribute of the [`CepacParameters`](https://github.com/hsphcdm/transm/blob/master-v4/source/parameters/parameterdefinitions.hpp) strucuture.

```
struct CepacParameters
{
...
   std::vector<std::pair<int, double>> target_yearly_rollout_proportions;
}
```

```
CepacParameters SimulationParametersXml::GetCepacParameters() const
{
...
        for(auto target : interventions_node.select_nodes("artRolloutIntervention/targetRolloutProportions/target"))
        {
            auto year = Attr<int>(target.node(), "year");
            auto value = Attr<double>(target.node(), "value");
            parameters.target_yearly_rollout_proportions.push_back({year, Text<double>(target.node())});
        }
...
}
```

## Modifying Parameters in the code base

To add demographic distributions to the ART rollout parameter, the structure and parameteer input code must be updated to read and store an Entity Distribution which is a pair of a double and a [DemographicProfile](https://github.com/hsphcdm/transm/blob/master-v4/source/entities/demographicprofile.hpp). The `target_yearly_rollout_proportions` attribute is modified to receive the EntityDistribution and the function to read the parameter is updated to use the existing funciton `GetEntityDistribution()`. 

```
struct CepacParameters
{
...
    std::vector<std::tuple<int, double, EntityDistributions>> target_yearly_rollout_proportions;
}
```

```
CepacParameters SimulationParametersXml::GetCepacParameters() const
{
...
        for(auto target : interventions_node.select_nodes("artRolloutIntervention/targetRolloutProportions/target"))
        {
            auto year = Attr<int>(target.node(), "year");
            auto value = Attr<double>(target.node(), "value");
            EntityDistributions distributions = GetEntityDistributions(target.node());

            parameters.target_yearly_rollout_proportions.push_back(
              {year, value, distributions}        }
...
}
```

## Setting ART Rollout by Demographic Proportions

Finally the code that uses the `target_yearly_rollout_proportions` is updated to assign access to ART by proportions. This takes place when the code Simulation class is initialized. The code currently loops through the cepac_parameters instance for the simulation and assigns the yearly rollout proportions to the entire population. The next step will be to update this code to distribute the proportions by demographics.

```
void Simulation::Initialize(SimulationParameters &parameters)
{
...
    for(const auto &prop : cepac_params.target_yearly_rollout_proportions)
    {
        parameters_.targetYearlyRolloutProportions[std::get<0>(prop)] = std::get<1>(prop);    
    }
...
}
```
