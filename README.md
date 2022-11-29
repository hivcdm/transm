

    A dynamic agent-based model for HIV transmission
         _                                 
        | |                                
        | |_ _ __ __ _ _ __  ___ _ __ ___  
        | __| '__/ _` | '_ \/ __| '_ ` _ \ 
        | |_| | | (_| | | | \__ \ | | | | |
         \__|_|  \__,_|_| |_|___/_| |_| |_|
         
                      v4.6
                                    
 
The TRANSM is a C++ stochastic, agent-based model designed to simulate sexual transmission of HIV infection. The TRANSM can be used to answer the crucial questions facing HIV prevention leaders/policy-makers throughout the world. 

   *  For building instructions, see the file [INSTALL](https://github.com/hsphcdm/transm/INSTALL).
  
   *  Copyright and licensing information can be found in file [LICENSE](https://github.com/hsphcdm/transm/LICENSE).

As of 2022, the TRANSM has been adapted to address such questions in MIAMI and has undergone preparation for adaptation to other cities in the United States (Formerly addressed South Africa and Botswana in 2018). This expansive, detailed and innovative model reflects the best available data from prevention studies, HIV epidemiology and treatment. The agents in the model are male or female persons, who may or may not be sexually active and may or may not be HIV infected. The TRANSM accounts for several different sexual mixing behaviors among individuals and links to the previously published [CEPAC Disease Model](https://www.massgeneral.org/medicine/mpec/research/cpac-model), to account for the natural history of HIV in individual patients

In the TRANSM, the probability that any sexual act between an HIV-infected person and an HIV-uninfected one results in transmission depends on the HIV-RNA level and the gender of the infected person, the circumcision status of the male partner and whether or not a condom is used. Sexual acts occur in the context of various kinds of short- and long-term partnerships. In the model, males choose females for various types of partnerships based on each partner’s age, current relationship status, and behavioral risk group. Thus, the life of a person in the model progresses by becoming sexually active forming and dissolving partnerships in a way that depends on his/her current partnership status, age, risk group and other factors; possibly becoming HIV-infected and progressing through stages of infection via the Disease Model; and eventually dying of HIV infection or another cause.

The TRANSM provides multiple outputs for example, the number of new HIV infections, how many of each type of partnership led to the new infections, number of partnerships of each partnership type, the number of individuals in each age bucket, and the life expectancy, HIV-RNA distribution, and CD4 count distribution for the population.
