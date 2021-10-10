#pragma once

namespace transm {

    /// <summary>
	/// every Entity's CD4 count falls in a CD4 strata - used in CEPAC
	/// </summary>
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

	/// <summary>
	/// every Entity's hvl level falls in an HVL stratum (values in copies/mL)
	/// </summary>
	enum class HVLStrata
	{
		/// <summary>
		/// HIV-
		/// </summary>
		UNINFECTED,
		/// <summary>
		/// 0-20
		/// </summary>
		HVL_ZERO,
		/// <summary>
		/// 21-500
		/// </summary>
		HVL_ONE,
		/// <summary>
		/// 501-3000
		/// </summary>
		HVL_TWO,
		/// <summary>
		/// 3001-10000
		/// </summary>
		HVL_THREE,
		/// <summary>
		/// 10001-30000
		/// </summary>
		HVL_FOUR,
		/// <summary>
		/// 30001-100000
		/// </summary>
		HVL_FIVE,
		/// <summary>
		/// 100000+
		/// </summary>
		HVL_SIX,
		/// <summary>
		/// Initial stage of disease progression
		/// </summary>
		HVL_PRIMARY,
		/// <summary>
		/// Final stage of disease progression
		/// </summary>
		HVL_LATESTAGE,
        ENDType,
		Last = ENDType,
        First = UNINFECTED
	};

	enum class HIVStatus
	{
		NEGATIVE, //hiv negative
		OBSERVED_ACUTE,
		UNOBSERVED_ACUTE,
		OBSERVED_CHRONIC,
		UNOBSERVED_CHRONIC,
		OBSERVED_LATESTAGE,//Late stage takes precedence over chronic (acute cases are never latestage)
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
		ALIVE, //not dead
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

	enum class RiskLevel   //used for assortativeness
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
        PREP_INADHERENT,
        WAS_ON_PREP,
        ENDType,
		Last = ENDType,
        First = OFF_PREP
    };
}
