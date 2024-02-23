#ifndef ENTITYTYPES_HPP
#define ENTITYTYPES_HPP

namespace transm {

    /** every Entity's CD4 count falls in a CD4 strata - used in CEPAC */
	enum class CD4Strata
	{
		CD4_ZERO,
		CD4_ONE,
		CD4_TWO,
		CD4_THREE,
		CD4_FOUR,
		CD4_FIVE,
        ENDType,
		Last = ENDType,
        First = CD4_ZERO
	};

	/** every Entity's hvl level falls in an HVL stratum (values in copies/mL) */
	enum class HVLStrata
	{

		UNINFECTED,    /***< HIV negative */
		HVL_ZERO,      /***< 0-20 */
		HVL_ONE,       /***< 21-500 */
		HVL_TWO,       /***< 501-3000	*/
		HVL_THREE,     /***< 3001-10000 */
		HVL_FOUR,      /***< 10001-30000 */
		HVL_FIVE,      /***< 30001-100000 */
		HVL_SIX,       /***< 100000+ */
		HVL_PRIMARY,   /***< Initial stage of disease progression */
		HVL_LATESTAGE, /***< Final stage of disease progression */
        ENDType,
		Last = ENDType,
        First = UNINFECTED
	};

    /** Tracking every Entity's HIV status in CDM */
	enum class HIVStatus
	{
		NEGATIVE, /* HIV negative */
		OBSERVED_ACUTE,
		UNOBSERVED_ACUTE,
		OBSERVED_CHRONIC,
		UNOBSERVED_CHRONIC,
		OBSERVED_LATESTAGE, /* Late stage takes precedence over chronic (acute cases are never latestage) */
		UNOBSERVED_LATESTAGE,
		ENDHIVStatus,
		ANY_POSITIVE,
		ANY_OBSERVED_POSITIVE,
		ANY_NOT_OBSERVED_POSITIVE,
        ENDType,
		Last = ENDHIVStatus,
		First = NEGATIVE
	};

	enum class DeathStatus
	{
		ALIVE, /* not dead */
		DTH_OI,
		DTH_CHRAIDS,
		DTH_NONAIDS,
		DTH_TOX_ART,
		DTH_TOX_PROPH,
		DTH_OTHER,
        ENDType,
		Last = ENDType,
        First = ALIVE
	};

	enum class RiskLevel  /* used for assortativeness */
	{
		LOW,
		HIGH,
        ENDType,
		Last = ENDType,
		First = LOW
	};

    enum class PrepStatus
    {
        OFF_PREP,
        PREP_ADHERENT,
        PREP_SUBSTANTIALLY_ADHERENT,
        PREP_PARTIALLY_ADHERENT,
        PREP_INADHERENT, /* for later */
        WAS_ON_PREP,
        ENDType,
		Last = ENDType,
        First = OFF_PREP
    };
}


#endif /* ENTITYTYPES_HPP */