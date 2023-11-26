

    A dynamic agent-based model for HIV transmission
         _                                 
        | |                                
        | |_ _ __ __ _ _ __  ___ _ __ ___  
        | __| '__/ _` | '_ \/ __| '_ ` _ \ 
        | |_| | | (_| | | | \__ \ | | | | |
         \__|_|  \__,_|_| |_|___/_| |_| |_|
         
                      v4.7
                                    
 
The TRANSM is a stochastic, agent-based C++ model designed to simulate the sexual transmission of HIV infection, providing crucial insights for global HIV prevention leaders and policymakers.

   * For building instructions, refer to the [INSTALL](https://github.com/hsphcdm/transm/blob/Issue42/INSTALL) file. 
   
   * Copyright and licensing information are available in the [LICENSE](https://github.com/hsphcdm/transm/blob/Issue42/LICENSE) file.

As of 2022, the TRANSM has been adapted to address key questions in MIAMI and is in preparation for adaptation to other U.S. cities (previously applied to South Africa and Botswana in 2018). This expansive and innovative model incorporates the best available data from prevention studies, HIV epidemiology, and treatment, featuring male and female agents representing individuals with varying sexual activity and HIV infection statuses.

The TRANSM considers diverse sexual mixing behaviors and links to the previously published [CEPAC Disease Model](https://www.massgeneral.org/medicine/mpec/research/cpac-model) to capture the natural history of HIV in individual patients.

In the TRANSM, the transmission probability in sexual acts depends on factors such as HIV-RNA level, gender, male partner's circumcision status, and condom use. Sexual acts occur within various short- and long-term partnerships. Males select female partners based on age, relationship status, and behavioral risk group. The life progression in the model includes becoming sexually active, forming and dissolving partnerships, potential HIV infection, disease progression via the Disease Model, and eventual mortality from HIV or other causes.

The TRANSM outputs multiple metrics, including new HIV infections, the partnership types leading to infections, the distribution of partnerships, age demographics, life expectancy, HIV-RNA distribution, and CD4 count distribution for the population.