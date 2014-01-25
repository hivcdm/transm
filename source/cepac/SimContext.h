#pragma once

#include "include.h"

/**
	SimContext class contains all the natural history information needed for running
	simulations.  Contains functions to read all the necessary information from an input file.
	Data is read into subclasses that correspond to the tabs on the input sheet.  Read only
	access to this is information is provided through public accessor functions that return const
	pointers to these classes.
*/
class SimContext
{
public:
	/* Constructors and Destructor */
	SimContext(string runName);
	~SimContext(void);
	int counter;
	/* Misc cohort and simulation constants */
	/** The maximum number of years that could be lived by a patient */
	static const int AGE_YRS = 101;
	/** The youngest patient age possible (for life tables) */
	static const int AGE_STARTING = 0;
	/** The oldest patient age possible (for life tables) */
	static const int AGE_MAXIMUM = 100;
	/** The number of age categories */
	static const int AGE_CATEGORIES = 20;
	/** The size of SimContext::GENDER_TYPE */
	static const int GENDER_NUM = 2;
	/** The specific genders available */
	enum GENDER_TYPE {GENDER_MALE, GENDER_FEMALE};
	/** Strings correlated with SimContext::GENDER_TYPE */
	static const char *GENDER_STRS[];
	/** The size of SimContext::BOUNDS_TYPE */
	static const int NUM_BOUNDS = 2;
	/**The number of age categories for for art start policy in peds*/
	static const int NUM_ART_START_CD4PERC_PEDS=4;
	/** The bounds calculated */
	enum BOUNDS_TYPE {LOWER_BOUND, UPPER_BOUND};
	/**Conditionals */
	enum CONDITIONS_TYPE {AND,OR};
	/**Directions */
	enum DIRECTIONS_TYPE {LEFT,RIGHT};
	/** The number of risk factors used by the model */
	static const int RISK_FACT_NUM = 5;
	/** The type of longitudal summary recording available: none, detailed monthly, brief monthly, detailed yearly */
	enum LONGIT_SUMM_TYPE {LONGIT_SUMM_NONE, LONGIT_SUMM_MTH_DET, LONGIT_SUMM_MTH_BRF, LONGIT_SUMM_YR_DET};
	/** A constant used for null values */
	static const int NOT_APPL = -1;
	/** The number of patients to be traced in the trace file (default 50 -- may be modified by GUI input) */
	static int numPatientsToTrace;

	/* CD4 and HVL strata constants */
	/** The size of SimContext::CD4_STRATA */
	static const int CD4_NUM_STRATA = 6;
	/** The CD4 strata */
	enum CD4_STRATA {CD4_VLO, CD4__LO, CD4_MLO, CD4_MHI, CD4__HI, CD4_VHI};
	/** Strings corresponding to SimContext::CD4_STRATA */
	static const char *CD4_STRATA_STRS[];
	/** The size of SimContext::HVL_STRATA */
	static const int HVL_NUM_STRATA = 7;
	/** The viral load strata */
	enum HVL_STRATA {HVL_VLO, HVL__LO, HVL_MLO, HVL_MED, HVL_MHI, HVL__HI, HVL_VHI};
	/** Strings corresponding to SimContext::HVL_STRATA */
	static const char *HVL_STRATA_STRS[];
	/** The midpoints of the HVL strata corresponding to SimContext::HVL_STRATA */
	static const double HVL_STRATA_MIDPTS[];
	/** A constant fixing suppressed viral load to HVL__LO */
	static const int HVL_SUPPRESSION = HVL__LO;

	/* OI and cause of death constants */
	/** The number of OIs */
	static const int OI_NUM = 15;
	/** All availabled OIs */
	enum OI_TYPE {OI_1, OI_2, OI_3, OI_4, OI_5, OI_6, OI_7, OI_8,
		OI_9, OI_10, OI_11, OI_12, OI_TB, OI_14, OI_15, OI_NONE};
	/** Strings corresponding to SimContext::OI_TYPE from RunSpecs input tab, RunSpecs B20-B34 */
	static char OI_STRS[OI_NUM][32];	  // from RunSpecs input tab, RunSpecs B20-B34
	/** The size of SimContext::DTH_CAUSES */
	static const int DTH_NUM_CAUSES = 25;
	/** Number of basic death causes (non-CHRMs, non-TB) */
	static const int DTH_NUM_CAUSES_BASIC = 17;
	/** Enum of the death causes */
	enum DTH_CAUSES {DTH_OI_1, DTH_OI_2, DTH_OI_3, DTH_OI_4, DTH_OI_5, DTH_OI_6,
		DTH_OI_7, DTH_OI_8, DTH_OI_9, DTH_OI_10, DTH_OI_11, DTH_OI_12, DTH_OI_TB,
		DTH_OI_14, DTH_OI_15, DTH_CHRAIDS, DTH_NONAIDS,
		DTH_TOX_ART, DTH_TOX_PROPH, DTH_TOX_TB_PROPH, DTH_TOX_TB_TREATM,
		DTH_CHRM_1, DTH_CHRM_2, DTH_CHRM_3, DTH_CHRM_4};
	/** Strings corresponding to SimContext::DTH_CAUSES filled in with OIs and CHRMs */
	static char DTH_CAUSES_STRS[DTH_NUM_CAUSES][32];	// filled in with OIs and CHRMs
	/** Size of SimContext::HIST_TYPE */
	static const int HIST_NUM = 2;
	/** The number of types of OI history (yes or no) */
	enum HIST_TYPE {HIST_N, HIST_Y};
	/** The size of SimContext::HIST_EXT_NUM */
	static const int HIST_EXT_NUM = 3;
	/** Enum of the extent of OI histories: no history of OIs in history, patient has a history of mild OIs and no severe OIs, patient has a history of at least one severe OI */
	enum HIST_EXT {
		HIST_EXT_N,			// no history of OI's in history
		HIST_EXT_MILD,		// patient has a history of mild OIs, and no severe OIs
		HIST_EXT_SEVR		// patient has a history of at least one severe OI
	};
	/** Strings corresponding to SimContext::HIST_EXT */
	static const char *HIST_OI_CATS_STRS[];

	/** This class keeps track of an individual mortality risk */
	class MortalityRisk {
	public:
		/** The cause of death for this risk */
		DTH_CAUSES causeOfDeath;
		/** The probability of death due to this risk */
		double probDeath;
		/** The cost accrued if this death occurs */
		double costDeath;
	};

	/* CHRMs constants */
	static const int CHRM_NUM = 4;
	enum CHRM_TYPE {CHRM_1, CHRM_2, CHRM_3, CHRM_4};
	static char CHRM_STRS[CHRM_NUM][32];	  // from CHRMs input tab, CHRMs B4-B7
	static const int CHRM_AGE_CAT_NUM = 7;
	static const char *CHRM_AGE_CAT_STRS[];
	static const int CHRM_TIME_PER_NUM = 3;

	/*Peds Cost constants */
	static const int PEDS_COST_AGE_CAT_NUM = 4;
	enum PEDS_COST_AGE{PEDS_COST_AGE_1,PEDS_COST_AGE_2,PEDS_COST_AGE_3,PEDS_COST_AGE_4,PEDS_COST_AGE_ADULT};

	/*Peds ART cost categories*/
	static const int PEDS_ART_COST_AGE_CAT_NUM=6;
	enum PEDS_ART_COST_AGE{PEDS_ART_COST_AGE_5MTH, PEDS_ART_COST_AGE_11MTH, PEDS_ART_COST_AGE_2YR, PEDS_ART_COST_AGE_4YR, PEDS_ART_COST_AGE_7YR, PEDS_ART_COST_AGE_12YR, PEDS_ART_COST_AGE_ADULT};

	/* Treatment and therapy constants */
	/** The size of SimContext::CLINIC_VISITS */
	static const int CLINIC_VISITS_NUM = 3;
	/** The different types of clinic visits */
	enum CLINIC_VISITS {
		/** attend clinic only if initial visit or if on ART or OI proph */
		CLINIC_INITIAL,
		/** attend clinic if acute OI, or if initial visit, or if on ART or OI proph */
		CLINIC_INIT_ACUTE,
		/** always attend clinic visits if scheduled, or if acute OI, or if on ART or OI proph */
		CLINIC_SCHED
	};
	/** Strings corresponding to SimContext::CLINIC_VISITS */
	static const char* CLINIC_VISITS_STRS[];
	/** The size of SimContext::THERAPY_IMPL */
	static const int THERAPY_IMPL_NUM = 3;
	/** An enum of therapy implementation types: patient type never implementing ART or OI prophylaxis,
	 *  patient type implementing OI proph but not ART, and patient type implementing OI proph and ART
	 */
	enum THERAPY_IMPL {
		THERAPY_IMPL_NONE,		// patient type never implementing ART or OI prophylaxis
		THERAPY_IMPL_PROPH,		// patient type implementing OI proph but not ART
		THERAPY_IMPL_PROPH_ART	// patient type implementing OI proph and ART
	};

	/* LTFU constants */
	/** The size of SimContext::LTFU_COEFF */
	static const int LTFU_NUM_COEFF = 7;
	/** An enum of the LTFU regression coefficients */
	enum LTFU_COEFF {LTFU_BACKGROUND, LTFU_AGE, LTFU_GENDER, LTFU_HISTORY,
		LTFU_T1, LTFU_T1_T2, LTFU_T2};
	/** The size of SimContext::RTC_COEFF */
	static const int RTC_NUM_COEFF = 4;
	/** An enum of the return-to-care regression coefficients */
	enum RTC_COEFF {RTC_BACKGROUND, RTC_CD4, RTC_ACUTESEVEREOI, RTC_ACUTEMILDOI};
	/** The lost-to-follow up states: never lost, currently lost, returned from lost */
	enum LTFU_STATE {LTFU_STATE_NONE, LTFU_STATE_LOST, LTFU_STATE_RETURNED};

	/* Heterogeneity constants */
	/** The size of SimContext::RESP_TYPE */
	static const int RESP_NUM_TYPES = 3;
	/** An enum of heterogeneity response types */
	enum RESP_TYPE {RESP_TYPE_FULL, RESP_TYPE_PARTIAL, RESP_TYPE_NON};
	/** Strings corresponding to SimContext::RESP_TYPE */
	static const char *RESP_TYPE_STRS[];
	/**size of SimContext::HET_OUTCOME */
	static const int HET_NUM_OUTCOMES=9;
	/** An enum of heterogeneity outcome types*/
	enum HET_OUTCOME{HET_OUTCOME_SUPP,HET_OUTCOME_LATEFAIL,HET_OUTCOME_ARTEFFECT_OI,HET_OUTCOME_ARTEFFECT_CHRMS,HET_OUTCOME_ARTEFFECT_MORT,HET_OUTCOME_RESIST,HET_OUTCOME_TOX,HET_OUTCOME_COST,HET_OUTCOME_RESTART};
	/** Strings corresponding to SimContexxt::HET_OUTCOME*/
	static const char *HET_OUTCOME_STRS[];
	/** The number of heterogeneity age categories */
	static const int RESP_AGE_CAT_NUM = 7;
	/* ART and Prophylaxis constants */
	/** The number of ART lines */
	static const int ART_NUM_LINES = 10;
	/** The size of SimContext::ART_STATES */
	static const int ART_NUM_STATES = 2;
	/** An enum representing the ART states (on or off ART) */
	enum ART_STATES {ART_OFF_STATE, ART_ON_STATE};
	/** The size of SimContext::ART_EFF_TYPE */
	static const int ART_EFF_NUM_TYPES = 3;
	/** An enum of the ART efficacy types (suppressed, partially suppressed, and failed */
	enum ART_EFF_TYPE {ART_EFF_SUCCESS, ART_EFF_PARTIAL, ART_EFF_FAILURE};
	/** Strings corresponding to SimContext::ART_EFF_TYPE */
	static const char *ART_EFF_STRS[];
	/** The size of SimContext::CD4_RESPONSE_TYPE */
	static const int CD4_RESPONSE_NUM_TYPES = 4;
	/** An enum of the possible CD4 response types */
	enum CD4_RESPONSE_TYPE {CD4_RESPONSE_1, CD4_RESPONSE_2, CD4_RESPONSE_3, CD4_RESPONSE_4};
	/** Strings corresponding to SimContext::CD4_RESPONSE_TYPE */
	static const char *CD4_RESPONSE_STRS[];
	/** The number of ART subregimens (per regimen) */
	static const int ART_NUM_SUBREGIMENS = 4;
	/** The size of SimContext::ART_TOX_SEVERITY */
	static const int ART_NUM_TOX_SEVERITY = 3;
	static const int ART_NUM_TOX_PER_SEVERITY = 6;
	enum ART_TOX_SEVERITY {ART_TOX_MINOR, ART_TOX_CHRONIC, ART_TOX_MAJOR};
	/** Strings corresponding to SimContext::ART_TOX_SEVERITY */
	static const char *ART_TOX_SEVERITY_STRS[];
	/** An enum of the duration of ART toxicity */
	enum ART_TOX_DUR {ART_TOX_DUR_MONTH, ART_TOX_DUR_SUBREG, ART_TOX_DUR_REG, ART_TOX_DUR_DEATH};
	/** The size of SimContext::ART_FAIL_TYPE */
	static const int ART_NUM_FAIL_TYPES = 3;
	/** An enum of the ART failure types */
	enum ART_FAIL_TYPE {ART_FAIL_VIROLOGIC, ART_FAIL_IMMUNOLOGIC, ART_FAIL_CLINICAL, ART_FAIL_NOT_FAILED};
	/** Strings corresponding to SimContext::ART_FAIL_TYPE */
	static const char *ART_FAIL_TYPE_STRS[];
	/** The size of SimContext::ART_FAIL_BY_OI */
	static const int ART_FAIL_BY_OI_NUM = 4;
	/** An enum of the ART failure by OI types */
	enum ART_FAIL_BY_OI {ART_FAIL_BY_OI_NONE, ART_FAIL_BY_OI_PRIMARY,
		ART_FAIL_BY_OI_SECONDARY, ART_FAIL_BY_OI_ANY};
	/** The size of SimContext::ART_STOP_TYPE less two */
	static const int ART_NUM_STOP_TYPES = 7;
	/** An enum of the ART stopping types */
	enum ART_STOP_TYPE {ART_STOP_MAX_MTHS, ART_STOP_MAJ_TOX, ART_STOP_FAIL, ART_STOP_CD4, ART_STOP_SEV_OI,
		ART_STOP_FAIL_MTHS, ART_STOP_LTFU, ART_STOP_NOT_STOPPED, ART_STOP_STI};
	/** Strings corresponding to SimContext::ART_STOP_TYPE */
	static const char *ART_STOP_TYPE_STRS[];
	/** The size of the partial suppression distribution */
	static const int ART_NUM_HVL_PARTIAL = 3;
	/** The number of months to record ART stats */
	static const int ART_NUM_MTHS_RECORD = 3;
	/** The number of STI cycles */
	static const int STI_NUM_CYCLES = 5;
	/** The number of STI periods */
	static const int STI_NUM_PERIODS = 2;
	/** The number of STIs to track */
	static const int STI_NUM_TRACKED = 30;
	/** An enum of STI states */
	enum STI_STATE {STI_STATE_NONE, STI_STATE_INTERRUPT, STI_STATE_RESTART, STI_STATE_ENDPOINT};
	/** One more than the number of prophylaxis types for iterating */
	static const int PROPH_NUM = 3;
	/** The size of SimContext::PROPH_TYPE */
	static const int PROPH_NUM_TYPES = 2;
	/** An enum of the types of prohpylaxis (primary and secondary) */
	enum PROPH_TYPE {PROPH_PRIMARY, PROPH_SECONDARY};
	/** Strings corresponding to SimContext::PROPH_TYPE */
	static const char *PROPH_TYPE_STRS[];
	/** An enum of prophylaxis toxicity types */
	enum PROPH_TOX_TYPE {PROPH_TOX_NONE, PROPH_TOX_MINOR, PROPH_TOX_MAJOR};

	/** A class representing a single ART Toxicity Effect */
	class ARTToxicityEffect {
	public:
		/** The month of the toxicity start */
		int monthOfToxStart;
		/** The SimContext::ART_TOX_SEVERITY corresponding to this toxicity */
		ART_TOX_SEVERITY toxSeverityType;
		/** The toxicity number */
		int toxNum;
		/** The corresponding ART regimen number */
		int ARTRegimenNum;
		/** The corresponding ART subregimen number */
		int ARTSubRegimenNum;
	};

	/** An enum of CD4 envelope types */
	enum ENVL_CD4_TYPE {ENVL_CD4_OVERALL, ENVL_CD4_INDIV, ENVL_CD4_PERC_OVERALL, ENVL_CD4_PERC_INDIV};
	/** A class representing a single CD4 envelope */
	class CD4Envelope {
	public:
		/** True if this is an active envelope */
		bool isActive;
		/** The regimen number corresponding to this envelope */
		int regimenNum;
		/** The month this envelope started */
		int monthOfStart;
		/** The slope of this envelope */
		double slope;
		/** The current value of this envelope */
		double value;
	};

	/* Cost and QOL constants */
	/** The size of SimContext::COST_TYPES */
	static const int COST_NUM_TYPES = 4;
	/** An enum of the different cost types */
	enum COST_TYPES {
		/** direct medical costs */
		COST_DIR_MED,		// direct medical costs
		/** direct non-medical costs */
		COST_DIR_NONMED,	// direct non-medical costs
		/** time costs */
		COST_TIME,			// time costs
		/** indirect costs */
		COST_INDIR,			// indirect costs
		/**  sum of costs, only used in special cases */
		COST_SUM			// sum of costs, only used in special cases
	};

	/* TB specific constants */
	/** Size of SimContext::TB_STRAIN */
	static const int TB_NUM_STRAINS = 3;
	/** An enum of the different TB strains: Drug Sensitive (DS), Multi-Drug Resistant (MDR), eXtensive Drug Resistant (XDR) */
	enum TB_STRAIN {TB_STRAIN_DS, TB_STRAIN_MDR, TB_STRAIN_XDR};
	/** Strings corresponding to SimContext::TB_STRAIN */
	static const char *TB_STRAIN_STRS[];
	/** Number of TB states ("no history" of TB is not considered a TB state in this counter) */
	static const int TB_NUM_STATES = 6;
	/** Number of initial TB states (no one can start on treatment) */
	static const int TB_NUM_INIT_STATES = 5;
	/** An enum of the possible TB states */
	enum TB_STATE {TB_STATE_LATENT, TB_STATE_ACTIVE, TB_STATE_TREATM_TRUE_SUCC, TB_STATE_TREATM_FALSE_SUCC,
		TB_STATE_HIST_ACTV,	TB_STATE_TREATM_FAILING, TB_STATE_NO_HIST};
	/** Strings corresponding to SimContext::TB_STATE */
	static const char *TB_STATE_STRS[];
	/** The number of distinct history of active TB substates */
	static const int TB_NUM_HIST_ACTV_STATES = 3;
	/** An enum of the possible history of active TB states */
	enum TB_HIST_ACTV_STATE {TB_HIST_ACTV_AFTER_TRUE, TB_HIST_ACTV_AFTER_FALSE, TB_HIST_ACTV_AFTER_SELF, TB_HIST_ACTV_NO_HIST_ACTV};
	/** Strings corresponding to SimContext::TB_HIST_ACTV_STATE */
	static const char *TB_HIST_ACTV_STATE_STRS[];
	/** An enum of the types of TB infection */
	enum TB_INFECT {TB_INFECT_PREVALENT, TB_INFECT_INITIAL, TB_INFECT_REINFECT, TB_INFECT_REACTIVATE, TB_INFECT_RELAPSE};
	/** Number of TB treatment lines */
	static const int TB_TREATM_LINES_NUM = 3;
	/** Number of TB treatment time periods */
	static const int TB_MTH_PERIODS_NUM = 3;
	/** Size of SimContext::TB_TREATM_STAGE */
	static const int TB_TREATM_STAGE_NUM = 4;
	/** An enum of the four TB treatment stages */
	enum TB_TREATM_STAGE {TB_TREATM_STAGE_1_NEW, TB_TREATM_STAGE_1_RPT, TB_TREATM_STAGE_2,
		TB_TREATM_STAGE_3};
	/** Strings corresponding to SimContext::TB_TREATM_STAGE */
	static const char *TB_TREATM_STAGE_STRS[];
	/** Size of SimContext::TB_CURE_TYPE */
	static const int TB_CURE_TYPE_NUM = 2;
	/** An enum of the two types of cure -- true and false */
	enum TB_CURE_TYPE {TB_CURE_TRUE, TB_CURE_FALSE};
	/** An enum of TB reinfection options */
	enum TB_REINFECT_OPTION {TB_REINFECT_NEW_OVER_OLD = 1, TB_REINFECT_SENS_OVER_RESIST, TB_REINFECT_RESIST_OVER_SENS};

	/* Testing specific constants */
	/** Size of SimContext::HIV_ID */
	static const int HIV_ID_NUM = 3;
	/** An enum of HIV testing status: HIV negative, unidentified HIV positive, identified HIV positive */
	enum HIV_ID {HIV_ID_NEG, HIV_ID_UNID, HIV_ID_IDEN};
	/** Strings corresponding to SimContext::HIV_ID */
	static const char *HIV_ID_STRS[];
	/** Size of SimContext::HIV_INF */
	static const int HIV_INF_NUM = 4;
	/** An enum of the types of HIV infection: negative (no infection), asymptomatic chronic, symptomatic chronic, and acute */
	enum HIV_INF {HIV_INF_NEG, HIV_INF_ASYMP_CHR_POS, HIV_INF_SYMP_CHR_POS, HIV_INF_ACUTE_SYN};
	/** Strings corresponding to SimContext::HIV_INF */
	static const char *HIV_INF_STRS[];
	/** Size of SimContext::HIV_POS */
	static const int HIV_POS_NUM=3;
	/** An enum of the types of Positive HIV Infection: asymptomatic chronic, symptomatic chronic, and acute*/
	enum HIV_POS {HIV_POS_ASYMP_CHR_POS, HIV_POS_SYMP_CHR_POS, HIV_POS_ACUTE_SYN};
	/** Size of SimContext::HIV_EXT_INF */
	static const int HIV_EXT_INF_NUM = 5;
	/** An enum of HIV status, similar to SimContext::HIV_INF but including risk type for HIV negative patients (high (HI) or low (LO)) */
	enum HIV_EXT_INF {HIV_EXT_INF_NEG_HI, HIV_EXT_INF_ASYMP_CHR_POS, HIV_EXT_INF_SYMP_CHR_POS, HIV_EXT_INF_ACUTE_SYN, HIV_EXT_INF_NEG_LO};
	/** Strings corresponding to SimContext::HIV_EXT_INF */
	static const char *HIV_EXT_INF_STRS[];
	/** Size of SimContext::HIV_BEHAV */
	static const int HIV_BEHAV_NUM = 2;
	/** Enums of risk levels for HIV negative patients (HI or LO) */
	enum HIV_BEHAV {HIV_BEHAV_HI, HIV_BEHAV_LO};
	/** Size of SimContext::HIV_DET */
	static const int HIV_DET_NUM = 8;
	/** An enum of the different HIV detection methods */
	enum HIV_DET {HIV_DET_INITIAL, HIV_DET_SCREENING, HIV_DET_SCREENING_PREV_DET, HIV_DET_BACKGROUND, HIV_DET_BACKGROUND_PREV_DET, HIV_DET_OI, HIV_DET_OI_PREV_DET, HIV_DET_UNDETECTED};
	/** Strings corresponding to SimContext::HIV_DET */
	static const char *HIV_DET_STRS[];
	/** Size of SimContext::HIV_CARE */
	static const int HIV_CARE_NUM =6;
	static const char *HIV_CARE_STRS[];

	/** An enum of the different states of HIV care */
	enum HIV_CARE {HIV_CARE_NEG, HIV_CARE_UNDETECTED, HIV_CARE_UNLINKED, HIV_CARE_IN_CARE, HIV_CARE_LTFU, HIV_CARE_RTC};

	/** Number of age categories for HIV testing */
	static const int AGE_CAT_TEST = 7;
	/** Number of HIV test frequencies */
	static const int HIV_TEST_FREQ_NUM = 5;
	/** Number of test acceptance types */
	static const int TEST_ACCEPT_NUM = 3;
	/** Size of SimContext::TEST_RESULT */
	static const int TEST_RESULT_NUM = 4;
	/** An enum of the different HIV test results */
	enum TEST_RESULT {TEST_TRUE_POS, TEST_FALSE_POS, TEST_TRUE_NEG, TEST_FALSE_NEG};
	/** Strings corresponding to SimContext::TEST_RESULT */
	static const char *TEST_RESULT_STRS[];

	/* Pediatrics specific constants */
	/** Size of SimContext::PEDS_HIV_STATE */
	static const int PEDS_HIV_NUM = 4;
	/** Number of elements of SimContext::PEDS_HIV_STATE that don't correspond to HIV negative **/
	static const int PEDS_HIV_POS_NUM = 3;
	/** An enum of the different Pediatric HIV states: intra-uterine infected, intra-partum infected, post-partum infected, and uninfected */
	enum PEDS_HIV_STATE {PEDS_HIV_POS_IU, PEDS_HIV_POS_IP, PEDS_HIV_POS_PP, PEDS_HIV_NEG};
	/** Strings corresponding to SimContext::PEDS_HIV_STATE */
	static const char *PEDS_HIV_STATE_STRS[];
	/** Size of SimContext::PEDS_MOM_HIV_STATE */
	static const int PEDS_MOM_HIV_NUM = 8;
	/** Number of elements of SimContext::PEDS_MOM_HIV_STATE that don't correspond to HIV negative **/
	static const int PEDS_MOM_HIV_POS_NUM = 7;
	/** An enum of the different states a pediatric patient's mother might have */
	enum PEDS_MOM_HIV_STATE {PEDS_MOM_AIDS_ONART, PEDS_MOM_AIDS_INCARE, PEDS_MOM_AIDS_NOCARE,
		PEDS_MOM_NOAIDS_ONART, PEDS_MOM_NOAIDS_INCARE, PEDS_MOM_NOAIDS_NOCARE,
		PEDS_MOM_ACUTE, PEDS_MOM_HIV_NEG};
	/** Size of PEDS_EXPOSED_CONDITIONS */
	static const int PEDS_EXPOSED_CONDITIONS_NUM = 5;
	/** Enum of different conditions for counting exposure for peds*/
	enum PEDS_EXPOSED_CONDITIONS {PEDS_EXPOSED_MOTHER_CHRONIC, PEDS_EXPOSED_MOTHER_ACUTE, PEDS_EXPOSED_BREASTFEEDING, PEDS_EXPOSED_AFTER_WEANING, PEDS_EXPOSED_MOM_NEG};
	/** Strings corresponding to SimContext::PEDS_MOM_HIV_STATE */
	static const char *PEDS_MOM_HIV_STRS[];
	/** The number of month a pediatric patient's mother stays in the acute state */
	static const int PEDS_MOM_MTHS_ACUTE = 3;
	/** Size of SimContext::PEDS_BF_TYPE */
	static const int PEDS_BF_NUM = 3;
	/** An enum of the breast feeding types: exclusive breast feeding, mixed, and replacement feeding */
	enum PEDS_BF_TYPE {PEDS_BF_EXCL, PEDS_BF_MIXED, PEDS_BF_REPL};
	/** Number of months a patient should spend on exclusive breast feeding */
	static const int PEDS_BF_MTHS_EXCL = 6;
	/** Number of age categories that just correspond to infancy (from SimContext::PEDS_AGE_CAT) */
	static const int PEDS_AGE_INFANT_NUM = 7;
	/** Number of age categories that just correspond to infancy and early childhood (from SimContext::PEDS_AGE_CAT) */
	static const int PEDS_AGE_EARLY_NUM = 10;
	/** Number of age categories that just correspond to infancy and all of childhood (from SimContext::PEDS_AGE_CAT) */
	static const int PEDS_AGE_CHILD_NUM = 11;
	/** An enum of the pediatric age categories */
	enum PEDS_AGE_CAT {PEDS_AGE_2MTH, PEDS_AGE_5MTH, PEDS_AGE_8MTH, PEDS_AGE_11MTH, PEDS_AGE_14MTH,
		PEDS_AGE_17MTH, PEDS_AGE_23MTH, PEDS_AGE_2YR, PEDS_AGE_3YR, PEDS_AGE_4YR, PEDS_AGE_LATE, PEDS_AGE_ADULT};
	/** Strings corresponding to SimContext::PEDS_AGE_CAT */
	static const char *PEDS_AGE_CAT_STRS[];
	/** Age cut off in years for "early childhood" */
	static const int PEDS_YEAR_EARLY = 5;
	/** Final age cut off in years for "late childhood */
	static const int PEDS_YEAR_LATE = 13;
	/** Size of SimContext::PEDS_CD4_PERC */
	static const int PEDS_CD4_PERC_NUM = 8;
	/** An enum of the pediatric CD4 percentage categories */
	enum PEDS_CD4_PERC {PEDS_CD4_PERC_L5, PEDS_CD4_PERC_L10, PEDS_CD4_PERC_L15, PEDS_CD4_PERC_L20,
		PEDS_CD4_PERC_L25, PEDS_CD4_PERC_L30, PEDS_CD4_PERC_L35, PEDS_CD4_PERC_HIGHER};
	/** Strings corresponding to SimContext:: PEDS_CD4_PERC */
	static const char *PEDS_CD4_PERC_STRS[];


	/** RunSpecsInputs class contains inputs from the RunSpecs tab,
		one per simulation context */
	class RunSpecsInputs {
	public:
		/* Shortcuts and Miscellaneous inputs */
		/** RunSpecs D3 */
		string runSetName;
		string runName;
		/** RunSpecs D6 */
		int numCohorts;
		/** RunSpecs D7 */
		double discountFactor;
		/** RunSpecs F14 */
		double maxPatientCD4;
		/** RunSpecs F15 */
		bool enableARTCD4Envelope;
		/** RunSpecs F30-F32 */
		int monthRecordARTEfficacy[ART_NUM_MTHS_RECORD];
		/** RunSpecs E35 */
		bool randomSeedByTime;
		/** RunSpecs E37 */
		string userProgramLocale;
		/** RunSpecs E40-G40 */
		string inputVersion;
		string modelVersion;
		/** RunSpecs C45-C59 */
		double OIsFractionOfBenefit[OI_NUM];
		/** RunSpecs D45-D61 **/
		double deathFractionOfBenefit[DTH_NUM_CAUSES_BASIC];
		/** RunSpecs C66-E80 **/
		bool severeOIs[OI_NUM];
		/** RunSpecs C86-E90 **/
		double CD4StrataUpperBounds[CD4_NUM_STRATA-1];

		/* Logging inputs */
		/** RunSpecs F18 **/
		LONGIT_SUMM_TYPE longitLoggingLevel;
		/** RunSpecs AH5-AI19 **/
		int firstOIsLongitLogging[OI_NUM];
		int firstOIsChronicLongitLogging;
		/** RunSpecs N8 **/
		bool enableOIHistoryLogging;
		/** RunSpecs N9 **/
		int numARTFailuresForOIHistoryLogging;
		/** RunSpecs N12-O12 **/
		double CD4BoundsForOIHistoryLogging[NUM_BOUNDS];
		/** RunSpecs N14-O14 **/
		int HVLBoundsForOIHistoryLogging[NUM_BOUNDS];
		/** RunSpecs N17-N31 **/
		bool OIsToExcludeOIHistoryLogging[OI_NUM];
	}; /* end RunSpecsInputs */

	/** CohortInputs class contains inputs from the Cohort tab,
		one per simulation context */
	class CohortInputs {
	public:
		/* Initial characteristics and history inputs */
		/** Cohort C5 **/
		double initialCD4Mean;
		/** Cohort C6 **/
		double initialCD4StdDev;
		/** Cohort C10-H16 **/
		double initialHVLDistribution[CD4_NUM_STRATA][HVL_NUM_STRATA];
		/**Cohort C20 **/
		double initialAgeMean;
		/**Cohort C21 **/
		double initialAgeStdDev;
		/**Cohort C23-C24 **/
		double maleGenderDistribution;
		/**Cohort C26 **/
		double OIProphNonComplianceRisk;
		/**Cohort C27 **/
		double OIProphNonComplianceDegree;
		/**Cohort G31-G33 **/
		double clinicVisitTypeDistribution[CLINIC_VISITS_NUM];
		/**Cohort G35-G37 **/
		double therapyImplementationDistribution[THERAPY_IMPL_NUM];
		/**Cohort C42-C45 **/
		double CD4ResponseTypeOnARTDistribution[CD4_RESPONSE_NUM_TYPES];
		/**Cohort C52-H170 **/
		double probOIHistoryAtEntry[CD4_NUM_STRATA][HVL_NUM_STRATA][OI_NUM];
		/**Cohort M6-M7 **/
		double probRiskFactorPrev[RISK_FACT_NUM];
		/**Cohort Q6-Q7 **/
		double probRiskFactorIncid[RISK_FACT_NUM];
	}; /* end CohortInputs */

	/** TreatmentInputs class contains inputs from the Treatment tab,
		one per simulation context */
	class TreatmentInputs {
	public:
		/* ART start policy class */
		class ARTStartPolicy {
		public:
			double CD4BoundsOnly[NUM_BOUNDS];
			int HVLBoundsOnly[NUM_BOUNDS];
			double CD4BoundsWithHVL[NUM_BOUNDS];
			int HVLBoundsWithCD4[NUM_BOUNDS];
			bool OIHistory[OI_NUM];
			int numOIs;
			double CD4BoundsWithOIs[NUM_BOUNDS];
			bool OIHistoryWithCD4[OI_NUM];
			int minMonthNum;
			int monthsSincePrevRegimen;
		};
		/* ART failure policy class */
		class ARTFailPolicy {
		public:
			int HVLNumIncrease;
			int HVLBounds[NUM_BOUNDS];
			bool HVLFailAtSetpoint;
			int HVLMonthsFromInit;
			double CD4PercentageDrop;
			bool CD4BelowPreARTNadir;
			double CD4BoundsOR[NUM_BOUNDS];
			double CD4BoundsAND[NUM_BOUNDS];
			int CD4MonthsFromInit;
			ART_FAIL_BY_OI OIsEvent[OI_NUM];
			int OIsMinNum;
			int OIsMonthsFromInit;
			int diagnoseNumTestsFail;
			bool diagnoseUseHVLTestsConfirm;
			bool diagnoseUseCD4TestsConfirm;
			int diagnoseNumTestsConfirm;
		};
		/* ART stopping policy class */
		class ARTStopPolicy {
		public:
			int maxMonthsOnART;
			bool withMajorToxicty;
			bool afterFailImmediate;
			double afterFailCD4LowerBound;
			bool afterFailWithSevereOI;
			int afterFailMonthsFromObserved;
			int afterFailMinMonthNum;
			int afterFailMonthsFromInit;
		};
		/* Prophylaxis policy classes */
		class ProphStartPolicy {
		public:
			bool useOrEvaluation;
			double currCD4Bounds[NUM_BOUNDS];
			double minCD4Bounds[NUM_BOUNDS];
			int OIHistory[OI_NUM];
			int minMonthNum;
		};
		class ProphStopPolicy {
		public:
			bool useOrEvaluation;
			double currCD4Bounds[NUM_BOUNDS];
			double minCD4Bounds[NUM_BOUNDS];
			int OIHistory[OI_NUM];
			int minMonthNum;
			int monthsOnProph;
		};

		/* Clinical visit and testing inputs */
		//Treatment AO18 **/
		int clinicVisitInterval;
		//Treatment AM22-BA23  **/
		double probDetectOIAtEntry[OI_NUM];
		double probDetectOISinceLastVisit[OI_NUM];
		//Treatment AM26-BA26  **/
		double probSwitchSecondaryProph[OI_NUM];
		//Treatment AO4-AV4  **/
		double testingIntervalCD4Threshold;
		int testingIntervalARTMonthsThreshold;
		int testingIntervalLastARTMonthsThreshold;
		//Treatment AN7-AX8  **/
		int CD4TestingIntervalPreARTHighCD4;
		int CD4TestingIntervalPreARTLowCD4;
		int CD4TestingIntervalOnART[2];
		int CD4TestingIntervalOnLastART[2];
		int CD4TestingIntervalPostART;
		int HVLTestingIntervalPreARTHighCD4;
		int HVLTestingIntervalPreARTLowCD4;
		int HVLTestingIntervalOnART[2];
		int HVLTestingIntervalOnLastART[2];
		int HVLTestingIntervalPostART;
		//Treatment AN12-AN15  **/
		double probHVLTestErrorHigher;
		double probHVLTestErrorLower;
		double CD4TestStdDevPercentage;
		//Treatment AQ29-AQ34  **/
		bool ARTFailureOnlyAtRegularVisit;
		int numARTInitialHVLTests;
		int numARTInitialCD4Tests;
		bool emergencyVisitIsNotRegularVisit;

		//Treatment AQ36-AQ37
		int CD4TestingLag;
		int HVLTestingLag;

		/* ART policy and failure inputs */
		//Treatment D5-M48 **/
		ARTStartPolicy startART[ART_NUM_LINES];
		//Treatment K52-K61 **/
		bool enableSTIForART[ART_NUM_LINES];
		//Treatment D67-M101 **/
		ARTFailPolicy failART[ART_NUM_LINES];
		//Treatment D105-M109 **/
		ARTStopPolicy stopART[ART_NUM_LINES];
		//Treatment D114-M129 **/
		double ARTResistancePriorRegimen[ART_NUM_LINES][ART_NUM_LINES];
		double ARTResistanceHVL[HVL_NUM_STRATA];

		/* Proph policy inputs */
		//Treatment S5-AG114
		ProphStartPolicy startProph[PROPH_NUM_TYPES][OI_NUM];
		ProphStopPolicy stopProph[PROPH_NUM_TYPES][OI_NUM];
	}; /* end TreatmentInputs */

	/** LTFUInputs class contains inputs from the LTFU tab, one per simulation context */
	class LTFUInputs {
	public:
		//LTFU E4 **/
		bool useLTFU;
		//LTFU B8-C8 **/
		double propRespondLTFUPreARTLogitMean;
		double propRespondLTFUPreARTLogitStdDev;
		//LTFU B13-E13 **/
		double responseThresholdLTFU[RESP_NUM_TYPES-1];
		double responseValueLTFU[RESP_NUM_TYPES-1];
		//LTFU B17-E17 **/
		double propGeneralMedicineCost[HIV_CARE_NUM];
		//LTFU F33-F37 **/
		double probRemainOnOIProph;
		double probRemainOnOITreatment;
		//LTFU Q4-Q12 **/
		int minMonthsRemainLost;
		double regressionCoefficientsRTC[RTC_NUM_COEFF];
		double CD4ThresholdRTC;
		bool severeOIsRTC[SimContext::OI_NUM];
		//LTFU U26-U27 **/
		int maxMonthsAfterObservedFailureToRestartRegimen;
		double probRestartRegimenWithoutObsvervedFailure;
		//LTFU V31-V32 **/
		double probSuppressionWhenReturnToFailed[ART_NUM_LINES];
		double probSuppressionWhenReturnToSuppressed[ART_NUM_LINES];
	};

	/* HeterogeneityInputs class contains inputs from the Heterogeneity tab,
	 * 	one per simulation context */
	class HeterogeneityInputs {
	public:
		//Hetero D10-E10
		double propRespondBaselineLogitMean;
		double propRespondBaselineLogitStdDev;
		//Hetero D13-J13
		double propRespondAge[RESP_AGE_CAT_NUM];
		//Hetero H19-I19
		double propRespondAgeEarly;
		double propRespondAgeLate;
		//Hetero D16-I16
		double propRespondCD4[CD4_NUM_STRATA];
		//Hetero D19-D26
		double propRespondFemale;
		double propRespondHistoryOIs;
		double propRespondPriorARTToxicity;
		double propRespondRiskFactor[RISK_FACT_NUM];

		//Hetero AC3
		double interventionEligibility;
		double interventionInitCost;
		double interventionMthCost;
		int interventionCostDuration;
		int stageBoundsInterventionEfficacy[2];
		double interventionEfficacyMean[3];
		double interventionEfficacyStdDev[3];
	};

	/* STIInputs class contains inputs from the STI tab,
		one per simulation context */
	class STIInputs {
	public:
		/* STI initiation policy class, same inputs as ART start policy plus months since init ART */
		class InitiationPolicy : public TreatmentInputs::ARTStartPolicy {
		public:
			int monthsSinceARTStart;
		};
		/* STI endpoint policy class, same inputs as ART start policy plus months since init STI */
		class EndpointPolicy : public TreatmentInputs::ARTStartPolicy {
		public:
			int monthsSinceSTIStart;
		};

		/* STI initiation, restart, and subsequent stopping policy inputs */
		//STI D5-M50
		InitiationPolicy firstInterruption[ART_NUM_LINES];
		//STI D56-M59
		double ARTRestartCD4Bounds[ART_NUM_LINES][NUM_BOUNDS];
		int ARTRestartHVLBounds[ART_NUM_LINES][NUM_BOUNDS];
		//STI D62-F62
		int ARTRestartFirstTestMonth;
		int ARTRestartSecondTestMonth;
		int ARTRestartTestInterval;
		//STI D67-M70
		double ARTRestopCD4Bounds[ART_NUM_LINES][NUM_BOUNDS];
		int ARTRestopHVLBounds[ART_NUM_LINES][NUM_BOUNDS];
		//STI D73-F73
		int ARTRestopFirstTestMonth;
		int ARTRestopSecondTestMonth;
		int ARTRestopTestInterval;

		/* STI endpoint policy inputs */
		//STI S5-AB45
		EndpointPolicy endpoint[ART_NUM_LINES];
	}; /* end STIInputs */

	/* ProphInputs class contains inputs from the UserProphs tab,
		one for each proph type x OI x proph line in the simulation context */
	class ProphInputs {
	public:
		//UserProphs E5-S24
		double primaryOIEfficacy[OI_NUM];
		//UserProphs E28-S47
		double secondaryOIEfficacy[OI_NUM];
		//UserProphs E51-I70
		double monthlyProbResistance;
		double percentResistance;
		int timeOfResistance;
		double costFactorResistance;
		double mortalityFactorResistance;
		//UserProphs E74-G93
		double probMinorToxicity;
		double probMajorToxicity;
		int monthsToToxicity;
		double probDeathMajorToxicity;
		//UserProphs E97-I116
		double costMonthly;
		double costMinorToxicity;
		double QOLMinorToxicity;
		double costMajorToxicity;
		double QOLMajorToxicity;
		//UserProphs E120-E139
		int monthsToSwitch;
		bool switchOnMinorToxicity;
		bool switchOnMajorToxicity;
	}; /* end ProphInputs */

	/* ARTInputs class contains inputs from the UserARTs tab,
		one for each ART line in the simulation context */
	class ARTInputs {
	public:
		/* ART toxicity class */
		class ARTToxicity {
		public:
			string toxicityName;
			double probToxicity;
			double timeToToxicityMean;
			double timeToToxicityStdDev;
			double QOLMultiplier;
			ART_TOX_DUR QOLDuration;
			double costAmount;
			ART_TOX_DUR costDuration;
			int switchSubRegimenOnToxicity;
			int timeToChronicDeathImpact;
			double chronicDeathIncrease;
			ART_TOX_DUR chronicDeathDuration;
			double probAcuteDeathMajorToxicity;
			double costAcuteDeathMajorToxicity;
		};
		//UserARTs C4-D4
		double costInitial;
		double costMonthly;
		//UserARTs C6
		int efficacyTimeHorizon;
		//UserARTs C8-I11
		//double probInitialEfficacy[ART_EFF_NUM_TYPES][HVL_NUM_STRATA];
		//UserARTs C15-E15
		double partialSuppressionDistribution[ART_NUM_HVL_PARTIAL];
		//UserARTs C18-D19
		/**
		double probLateFailFromSuppress;
		double probLatePartialSuppressFromSuppress;
		double probLateFailFromPartialSuppress;
		**/
		int forceFailAtMonth;
		//UserARTs C21-H45
		int stageBoundsCD4ChangeOnART[ART_EFF_NUM_TYPES-1][2];
		int stageBoundCD4ChangeOnARTFail;
		double CD4ChangeOnARTMean[ART_EFF_NUM_TYPES-1][CD4_RESPONSE_NUM_TYPES][3];
		double CD4ChangeOnARTStdDev[ART_EFF_NUM_TYPES-1][CD4_RESPONSE_NUM_TYPES][3];
		double CD4MultiplierOnFailedART[CD4_RESPONSE_NUM_TYPES][2];
		double secondaryCD4ChangeOnARTStdDev;
		//UserARTs C49-D51
		double monthlyCD4MultiplierOffARTPreSetpoint[ART_EFF_NUM_TYPES];
		double monthlyCD4MultiplierOffARTPostSetpoint[ART_EFF_NUM_TYPES];
		//UserARTs C54-D56
		double monthlyProbHVLChange[ART_EFF_NUM_TYPES];
		int monthlyNumStrataHVLChange[ART_EFF_NUM_TYPES];
		//UserARTs C45-H202
		ARTToxicity toxicity[ART_NUM_SUBREGIMENS][ART_NUM_TOX_SEVERITY][ART_NUM_TOX_PER_SEVERITY];
		int monthsToSwitchSubRegimen[ART_NUM_SUBREGIMENS];

		//ART C233
		double propRespondARTRegimenLogitMean;
		//ART D233
		double propRespondARTRegimenLogitStdDev;

		//Art C244-D252
		double responseTypeThresholds[HET_NUM_OUTCOMES][RESP_NUM_TYPES-1];
		//Art E244-F252
		double responseTypeValues[HET_NUM_OUTCOMES][RESP_NUM_TYPES-1];
		//ART G244-252
		double responseTypeExponents[HET_NUM_OUTCOMES];

		//UserARTs F231
		double probFillARTPrescriptionsNonResponder;
		//UserARTs D235-D238
		double probRestartARTRegimenAfterFailure[RESP_NUM_TYPES];
	}; /* end ARTInputs */

	/* NatHistInputs class contains inputs from the NatHist tab,
		one per simulation context */
	class NatHistInputs {
	public:
		//NatHist C6-E11
		double chronicAIDSDeathProbOffART[HIST_EXT_NUM][CD4_NUM_STRATA];
		//NatHist C16-K21
		double chronicAIDSDeathProbOnARTMult[HIST_EXT_NUM][CD4_NUM_STRATA];
		//NatHist D26-R38
		double monthlyOIProbOffART[CD4_NUM_STRATA][OI_NUM][HIST_NUM];
		//NatHist C43-R63
		double monthlyOIProbOnARTMult[CD4_NUM_STRATA][OI_NUM];
		//NatHist D68-R80
		double probDeathFromOITreated[CD4_NUM_STRATA][OI_NUM][HIST_NUM];
		//NatHist D82-R94
		double probDeathFromOIUntreated[CD4_NUM_STRATA][OI_NUM][HIST_NUM];
		//NatHist D99-J111
		double monthlyCD4DeclineMean[CD4_NUM_STRATA][HVL_NUM_STRATA];
		double monthlyCD4DeclineStdDev[CD4_NUM_STRATA][HVL_NUM_STRATA];
		double monthlyCD4DeclineBtwSubject;
		//NatHist C115-D215
		double monthlyNonAIDSDeathProb[GENDER_NUM][AGE_YRS];
	}; /* end NatHistInputs */

	/* CHRMsInputs class contains inputs from the CHRMs tab,
		one per simulation context */
	class CHRMsInputs {
	public:
		//CHRMs H3
		bool showCHRMsOutput;
		//CHRMs D13-Q46
		double probPrevalentCHRMsHIVneg[CHRM_NUM][GENDER_NUM][CHRM_AGE_CAT_NUM];
		double probPrevalentCHRMs[CHRM_NUM][CD4_NUM_STRATA][GENDER_NUM][CHRM_AGE_CAT_NUM];
		//CHRMs D52-H55
		double probPrevalentCHRMsRiskFactorLogit[CHRM_NUM][RISK_FACT_NUM];
		//CHRMs D60-E63
		double prevalentCHRMsMonthsSinceStartMean[CHRM_NUM];
		double prevalentCHRMsMonthsSinceStartStdDev[CHRM_NUM];
		//CHRMs W6-AJ39
		double probIncidentCHRMsHIVneg[CHRM_NUM][GENDER_NUM][CHRM_AGE_CAT_NUM];
		double probIncidentCHRMs[CHRM_NUM][CD4_NUM_STRATA][GENDER_NUM][CHRM_AGE_CAT_NUM];
		//CHRMs W45-AB62
		double probIncidentCHRMsOnARTMult[CHRM_NUM][CD4_NUM_STRATA];
		double probIncidentCHRMsRiskFactorLogit[CHRM_NUM][RISK_FACT_NUM];
		double probIncidentCHRMsPriorHistoryLogit[CHRM_NUM][CHRM_NUM];
		//CHRMs W68-AJ182
		int probDeathCHRMsStageBounds[CHRM_NUM][CHRM_TIME_PER_NUM - 1];
		double probDeathCHRMsHIVneg[CHRM_NUM][CHRM_TIME_PER_NUM][GENDER_NUM][CHRM_AGE_CAT_NUM];
		double probDeathCHRMs[CHRM_NUM][CHRM_TIME_PER_NUM][CD4_NUM_STRATA][GENDER_NUM][CHRM_AGE_CAT_NUM];
		//CHRMs AP4-BC38
		int costCHRMsStageBounds[CHRM_NUM][CHRM_TIME_PER_NUM - 1];
		double costCHRMs[CHRM_NUM][CHRM_TIME_PER_NUM][GENDER_NUM][CHRM_AGE_CAT_NUM];
		double costDeathCHRMs[CHRM_NUM];
		//CHRMs AP44-BC78
		int QOLMultCHRMsStageBounds[CHRM_NUM][CHRM_TIME_PER_NUM - 1];
		double QOLMultCHRMs[CHRM_NUM][CHRM_TIME_PER_NUM][GENDER_NUM][CHRM_AGE_CAT_NUM];
		double QOLMultDeathCHRMs[CHRM_NUM];
	};

	/* CostInputs class contains inputs from the Cost tab,
		one per simulation context */
	class CostInputs {
	public:
		//Cost C7-M39
		double acuteOICostTreated[ART_NUM_STATES][OI_NUM][COST_NUM_TYPES];
		double acuteOICostUntreated[ART_NUM_STATES][OI_NUM][COST_NUM_TYPES];
		//Cost C45-M45
		double CD4TestCost[COST_NUM_TYPES];
		double HVLTestCost[COST_NUM_TYPES];
		//Cost C51-M87
		double deathCostTreated[ART_NUM_STATES][DTH_NUM_CAUSES_BASIC][COST_NUM_TYPES];
		double deathCostUntreated[ART_NUM_STATES][DTH_NUM_CAUSES_BASIC][COST_NUM_TYPES];
		//Cost S6-AF65
		double generalMedicineCost[GENDER_NUM][CHRM_AGE_CAT_NUM][COST_NUM_TYPES];
		double routineCareCostHIVPositive[ART_NUM_STATES][CD4_NUM_STRATA][GENDER_NUM][CHRM_AGE_CAT_NUM][COST_NUM_TYPES];
		//Cost C93-M109
		double clinicVisitCostRoutine[GENDER_NUM][CD4_NUM_STRATA][COST_NUM_TYPES];
	}; /* end CostInputs */

	/* TBInputs class contains inputs from the TB tab,
		one per simulation context */
	class TBInputs {
	public:
		/* TB Proph class */
		class TBProph {
		public:
			double efficacyNoHistory;
			double efficacyReactivation[TB_NUM_STRAINS];
			double efficacyReinfection[TB_NUM_STRAINS];
			double costMonthly;
			double costMinorToxicity;
			double QOLMultiplierMinorToxicity;
			double costMajorToxicity;
			double QOLMultiplierMajorToxicity;
			double probMinorToxicity;
			double probMinorToxicityARTMultiplier;
			double probMajorToxicity;
			double probMajorToxicityARTMultiplier;
			int monthsToToxicity;
			double probDeathMajorToxicity;
			double probIncreasedResistanceFailure;
		};
		/* Cohort TB initialization inputs */
		//TB D6-H15
		double distributionTBStateAtEntry[CD4_NUM_STRATA][TB_NUM_INIT_STATES];
		double distributionTBStrainAtEntry[TB_NUM_STRAINS][TB_NUM_INIT_STATES];
		//TB F19-G22
		double percentLatentTBIsEarly;
		double monthsSinceEarlyLatentMean;
		double monthsSinceEarlyLatentStdDev;
		double monthsSinceLateLatentMean;
		double monthsSinceLateLatentStdDev;
		//TB E29-G30
		double monthsInfectedNotTreatedMean[TB_NUM_STRAINS];
		double monthsInfectedNotTreatedStdDev[TB_NUM_STRAINS];

		/* Natural history inputs, no prior TB history */
		//TB M5-M10
		double probInfectionNoHistoryOffART[CD4_NUM_STRATA];
		//TB M12-O19
		int multiplierInfectionStageBoundsNoHistoryOnART[TB_MTH_PERIODS_NUM];
		double multiplierInfectionNoHistoryOnART[CD4_NUM_STRATA][TB_MTH_PERIODS_NUM];
		//TB L22-M22
		double probActiveInfectionNoHistory;
		//TB L25-N25
		double distributionInfectionStrainNoHistory[TB_NUM_STRAINS];

		/* Natural history inputs, latent TB */
		//TB M29-R37
		int probReactivationStageBoundsLatentOffART[TB_MTH_PERIODS_NUM-1];
		double probReactivationLatentOffART[CD4_NUM_STRATA][TB_NUM_STRAINS][TB_MTH_PERIODS_NUM-1];
		//TB M40-O47
		int multiplierReactivationStageBoundsLatentOnART[TB_MTH_PERIODS_NUM];
		double multiplierReactivationLatentOnART[CD4_NUM_STRATA][TB_MTH_PERIODS_NUM];
		//TB M49-M54
		double probReinfectionLatentOffART[CD4_NUM_STRATA];
		//TB M56-063
		int multiplierReinfectionStageBoundsLatentOnART[TB_MTH_PERIODS_NUM];
		double multiplierReinfectionLatentOnART[CD4_NUM_STRATA][TB_MTH_PERIODS_NUM];
		//TB L66-M66
		double probActiveReinfectionLatent;
		//TB L69-N69
		double distributionReinfectionStrainLatent[TB_NUM_STRAINS];
		//TB L71
		int reinfectionSupercedeOption;

		/* Natural history inputs, history of active TB after true cure*/
		/** TB M77-M82 */
		double probRelapseHistoryActiveAfterTrueCureOffART[CD4_NUM_STRATA];
		/** TB M84-Q84 */
		int multiplierRelapseStageBoundsHistoryActiveAfterTrueCureOnART[TB_MTH_PERIODS_NUM];
		/** TB M86-O91*/
		double multiplierRelapseHistoryActiveAfterTrueCureOnART[CD4_NUM_STRATA][TB_MTH_PERIODS_NUM];

		/* Natural history inputs, history of active TB after false cure*/
		/** TB M98-M103 */
		double probRelapseHistoryActiveAfterFalseCureOffART[CD4_NUM_STRATA];
		/** TB M105-Q105 */
		int multiplierRelapseStageBoundsHistoryActiveAfterFalseCureOnART[TB_MTH_PERIODS_NUM];
		/** TB M107-O112*/
		double multiplierRelapseHistoryActiveAfterFalseCureOnART[CD4_NUM_STRATA][TB_MTH_PERIODS_NUM];

		/* Natural history inputs, history of active TB after self cure*/
		/** TB M119-M124 */
		double probRelapseHistoryActiveAfterSelfCureOffART[CD4_NUM_STRATA];
		/** TB M126-Q126 */
		int multiplierRelapseStageBoundsHistoryActiveAfterSelfCureOnART[TB_MTH_PERIODS_NUM];
		/** TB M128-O133*/
		double multiplierRelapseHistoryActiveAfterSelfCureOnART[CD4_NUM_STRATA][TB_MTH_PERIODS_NUM];

		/* Active TB inputs */
		//TB X5-AA30
		int probSpontaneousResolutionStageBounds[TB_NUM_STRAINS][TB_MTH_PERIODS_NUM];
		double probSpontaneousResolution[TB_NUM_STRAINS][CD4_NUM_STRATA][TB_MTH_PERIODS_NUM+1];
		//TB X35-X40
		double probAcuteMortality[CD4_NUM_STRATA];
		//TB X42-AA67
		int probExtendedMortalityStageBoundsOffART[TB_NUM_STRAINS][TB_MTH_PERIODS_NUM];
		double probExtendedMortalityOffART[TB_NUM_STRAINS][CD4_NUM_STRATA][TB_MTH_PERIODS_NUM+1];
		//TB X69-X74
		double multiplierExtendedMortalityOnART[CD4_NUM_STRATA];

		/* TB treatment inputs */
		//TB AG4-AJ4
		double probReceiveTreatmentOnART;
		double probReceiveTreatmentOffART;
		//TB AG8-AJ17
		double monthsLagToStartTreatmentMean[TB_TREATM_STAGE_NUM];
		double monthsLagToStartTreatmentStdDev[TB_TREATM_STAGE_NUM];
		int monthsTreatmentDuration[TB_TREATM_STAGE_NUM];
		double probTreatmentDropout[TB_TREATM_STAGE_NUM];
		//TB AG21-AI23
		double probInitialTreatmentLine[TB_NUM_STRAINS][TB_TREATM_LINES_NUM];
		//TB AH27-AK30
		double probRepeatTreatmentAfterFailure[TB_TREATM_STAGE_NUM];
		double probNextTreatmentAfterFailure[TB_TREATM_STAGE_NUM];
		double probSkipTreatmentAfterFailure[TB_TREATM_STAGE_NUM];
		double probDiscontinueTreatmentAfterFailure[TB_TREATM_STAGE_NUM];
		//TB AG34-AJ36
		double probCuredAfterTreatment[TB_NUM_STRAINS][TB_TREATM_STAGE_NUM][TB_CURE_TYPE_NUM];
		//TB AG40-AJ40
		double probIncreasedResistanceNotCured[TB_TREATM_STAGE_NUM];
		//TB AQ6-AU6
		double acuteUntreatedCosts[COST_NUM_TYPES];
		//TB AQ12-AU22
		double acuteActiveTreatedCosts[TB_TREATM_STAGE_NUM][COST_NUM_TYPES];
		double acuteActiveTreatedARTMultiplier[TB_TREATM_STAGE_NUM];
		double monthlyTreatedCosts[TB_TREATM_STAGE_NUM][COST_NUM_TYPES];
		double monthlyTreatedARTMultiplier[TB_TREATM_STAGE_NUM];
		//TB AQ30-AU38
		double costTreatmentMinorToxicity[TB_TREATM_STAGE_NUM];
		double QOLMultiplierTreatmentMinorToxicity[TB_TREATM_STAGE_NUM];
		double costTreatmentMajorToxicity[TB_TREATM_STAGE_NUM];
		double QOLMultiplierTreatmentMajorToxicity[TB_TREATM_STAGE_NUM];
		double probTreatmentMinorToxicity[TB_TREATM_STAGE_NUM];
		double probTreatmentMinorToxicityARTMultiplier[TB_TREATM_STAGE_NUM];
		double probTreatmentMajorToxicity[TB_TREATM_STAGE_NUM];
		double probTreatmentMajorToxicityARTMultiplier[TB_TREATM_STAGE_NUM];
		int monthsToTreatmentToxicity[TB_TREATM_STAGE_NUM];
		double probDeathTreatmentMajorToxicity[TB_TREATM_STAGE_NUM];

		/* TB prophylaxis inputs */
		//TB BJ12-BL18
		bool startProphUseOrEvaluation;
		double startProphCurrentCD4Bounds[NUM_BOUNDS];
		double startProphMinCD4Bounds[NUM_BOUNDS];
		int startProphKnownActiveHistory;
		int startProphAtARTInitiation;
		//TB BK23-BL31
		double probReceiveProphOffART;
		double probReceiveProphOnART;
		double monthsLagToStartProphMean;
		double monthsLagToStartProphStdDev;
		double probDropoffProph;
		//TB BJ35-BL39
		bool stopProphUseOrEvaluation;
		double stopProphCurrentCD4Bounds[NUM_BOUNDS];
		int stopProphAtARTInitiation;
		int stopProphNumMonths;
		bool continueProphAfterStop;
		//TB AY6-BF73
		TBProph *tbProphInputs[PROPH_NUM];
	}; /* end TBInputs */

	/* QOLInputs class contains inputs from the QOL tab,
		one per simulation context */
	class QOLInputs {
	public:
		//QOL C6-R11
		double routineCareQOL[CD4_NUM_STRATA][OI_NUM+1];
		//QOL C16-D32
		double acuteOIQOL[OI_NUM];
		double deathBasicQOL[DTH_NUM_CAUSES_BASIC];
		//QOL C38-D138
		double nonAIDSBackgroundQOL[GENDER_NUM][AGE_YRS];
		//QOL H16
		bool enableQOLDecreaseMultipleOI;
	}; /* end QOLInputs */

	/* HIVTestInputs class contains inputs from the HIVTest tab,
		one per simulation context */
	class HIVTestInputs {
	public:
		//HIVTest E3
		bool enableHIVTesting;
		//HIVTest E4
		bool HIVTestAvailable;
		//HIVTest E5
		bool CD4TestAvailable;
		//HIVTest E6
		bool useAlternateStoppingRule;
		//HIVTest E7
		int totalCohortsWithHIVPositiveLimit;
		//HIVTest E8
		int totalCohortsWithoutHIVPositiveLimit;
		//HIVTest M5-N8
		double initialHIVDistribution[HIV_EXT_INF_NUM];
		//HIVTest N12-O15
		double initialAcuteCD4DistributionMean;
		double initialAcuteCD4DistributionStdDev;
		//HIVTest N18-T21
		double initialAcuteHVLDistribution[CD4_NUM_STRATA][HVL_NUM_STRATA];
		//HIVTest M39-M41
		double probHIVDetectionInitial[HIV_INF_NUM];
		//HIVTest K55-K69
		double probHIVDetectionWithOI[OI_NUM];
		//HIVTest N28-O34
		double probHIVInfection[AGE_CAT_TEST][HIV_BEHAV_NUM];
		//HIVTest Y5-Z9
		int HIVTestingInterval[HIV_TEST_FREQ_NUM];
		double HIVTestingProbability[HIV_TEST_FREQ_NUM];
		//HIVTest AB15-AB24
		double deathCostHIVNegative;
		double monthCostHIVUndetected[CD4_NUM_STRATA];
		double chronicDeathCostHIVUndetected;
		double nonAIDSDeathCostHIVUndetected;
		//HIVTest AC15-AC24
		double deathQOLHIVNegative;
		double monthQOLHIVUndetected[CD4_NUM_STRATA];
		double chronicDeathQOLHIVUndetected;
		double nonAIDSDeathQOLHIVUndetected;
		//HIVTest AB29
		int monthsFromAcuteToChronic;
		//HIVTest AB31-AH32
		double CD4ChangeAtChronicHIVMean[HVL_NUM_STRATA];
		double CD4ChangeAtChronicHIVStdDev[HVL_NUM_STRATA];
		//HIVTest AB35-AH41
		double HVLDistributionAtChronicHIV[HVL_NUM_STRATA][HVL_NUM_STRATA];
		//HIVTest AO5-AS10
		double HIVTestAcceptDistribution[HIV_EXT_INF_NUM][TEST_ACCEPT_NUM];
		double HIVTestAcceptRate[HIV_EXT_INF_NUM][TEST_ACCEPT_NUM];
		//HIVTest AO14-AS18
		double HIVTestInitialCost[HIV_EXT_INF_NUM];
		double HIVTestNonReturnCost[HIV_EXT_INF_NUM];
		double HIVTestDetectionCost[HIV_INF_NUM];
		double HIVBackgroundDetectionRate[HIV_INF_NUM];
		double HIVBackgroundTestingCost[HIV_INF_NUM];
		//HIVTest AO31-AR78
		double HIVTestReturnRate[HIV_EXT_INF_NUM];
		double HIVTestPositiveRate[HIV_INF_NUM];
		double HIVTestPositiveCost[HIV_INF_NUM];
		double HIVTestNegativeCost[HIV_INF_NUM];
		double HIVTestPositiveQOLMultiplier[HIV_INF_NUM];
		double HIVTestNegativeQOLMultiplier[HIV_INF_NUM];
		double HIVTestCost[HIV_EXT_INF_NUM];
		//HIVTEST BA5-BC11
		double CD4TestAcceptRate[HIV_POS_NUM];
		double CD4TestReturnRate[HIV_POS_NUM];
		double CD4TestInitialCost[HIV_POS_NUM];
		double CD4TestCost[HIV_POS_NUM];
		double CD4TestReturnCost[HIV_POS_NUM];
		double CD4TestNonReturnCost[HIV_POS_NUM];
		//HIVTEST BA11
		double CD4TestStdDevPercentage;
		//HIVTEST AZ14-BE14
		double CD4TestLinkageRate[CD4_NUM_STRATA];
	}; /* end HIVTestInputs */

	/* PedsInputs class contains inputs from the Peds tab,
	 * 	one per simulation context */
	class PedsInputs {
	public:
		//Peds E3-E4
		bool enablePediatricsModel;
		bool enableSimplifiedBehavior;

		//Peds J3-J4
		double initialAgeMean;
		double initialAgeStdDev;

		//Peds D11-K13
		double initialHIVStateDistribution[PEDS_HIV_NUM][PEDS_MOM_HIV_NUM];
		//Peds D18-F20
		double initialBFDistribution[PEDS_BF_NUM];
		int initialBFDuration;
		//Peds E24-F32
		double initialCD4PercentageIUMean;
		double initialCD4PercentageIUStdDev;
		double initialCD4PercentageIPMean;
		double initialCD4PercentageIPStdDev;
		double initialCD4PercentagePPMean[PEDS_AGE_INFANT_NUM];
		double initialCD4PercentagePPStdDev[PEDS_AGE_INFANT_NUM];
		//Peds E35-K43
		double initialHVLDistributionIU[HVL_NUM_STRATA];
		double initialHVLDistributionIP[HVL_NUM_STRATA];
		double initialHVLDistributionPP[PEDS_AGE_INFANT_NUM][HVL_NUM_STRATA];
		//Peds C47-J66
		CD4_STRATA adultCD4Strata[PEDS_AGE_EARLY_NUM][PEDS_CD4_PERC_NUM];
		//Peds P5-W36
		double monthlyCD4PercentDecline[PEDS_HIV_POS_NUM][PEDS_AGE_EARLY_NUM][PEDS_CD4_PERC_NUM];
		//Peds P40-Q47
		double absoluteCD4TransitionMean[PEDS_CD4_PERC_NUM];
		double absoluteCD4TransitionStdDev[PEDS_CD4_PERC_NUM];
		//Peds P52-V58
		double setpointHVLTransition[HVL_NUM_STRATA][HVL_NUM_STRATA];
		//Peds P62-W93
		double probChronicAIDSDeathEarly[HIST_EXT_NUM][PEDS_AGE_EARLY_NUM][PEDS_CD4_PERC_NUM];
		//Peds O97-Q102
		double probChronicAIDSDeathLate[HIST_EXT_NUM][CD4_NUM_STRATA];
		//Peds O106-P115
		double probNonAIDSDeathEarly[GENDER_NUM][PEDS_AGE_EARLY_NUM];
		//Peds Z115
		bool useExposedUninfectedDefs;
		//Peds T106-U115
		double probNonAIDSDeathExposedUninfectedEarly[GENDER_NUM][PEDS_AGE_EARLY_NUM];
		//Peds AA108-AA112
		bool exposedUninfectedDefsEarly[PEDS_EXPOSED_CONDITIONS_NUM];
		//Peds P122-AF299
		double probAcuteOIEarly[OI_NUM][PEDS_AGE_EARLY_NUM][PEDS_CD4_PERC_NUM][HIST_NUM];
		//Peds P303-AD315
		double probAcuteOILate[OI_NUM][CD4_NUM_STRATA][HIST_NUM];
		//Peds P319-AD383
		double probDeathAcuteOITreatedEarly[OI_NUM][PEDS_CD4_PERC_NUM][HIST_NUM];
		double probDeathAcuteOIUntreatedEarly[OI_NUM][PEDS_CD4_PERC_NUM][HIST_NUM];
		double probDeathAcuteOITreatedLate[OI_NUM][CD4_NUM_STRATA][HIST_NUM];
		double probDeathAcuteOIUntreatedLate[OI_NUM][CD4_NUM_STRATA][HIST_NUM];
		//Peds AL4
		double probMaternalHIVInfection;
		//Peds AL11-AS11
		double probMaternalDeath[PEDS_MOM_HIV_NUM];
		//Peds AM18-AS20
		double probHIVInfectionPP[PEDS_BF_NUM][PEDS_MOM_HIV_POS_NUM];
		//Peds AL26-AW35
		double probDeathHIVNegativeNonexposed[GENDER_NUM][PEDS_AGE_CHILD_NUM];
		double probDeathHIVNegativeExposed[GENDER_NUM][PEDS_AGE_CHILD_NUM];
		double probDeathHIVPositive[PEDS_HIV_POS_NUM][ART_NUM_STATES][GENDER_NUM][PEDS_AGE_CHILD_NUM];
		//Peds AM38-AM39
		double probDeathMaternalRateMultiplier;
		double probDeathReplacementFedMultiplier;
		int ReplacementFedMultiplierDuration;
		//Peds AM43-44
		double probStartART[PEDS_HIV_POS_NUM];
		//Peds AL47-AL52
		double monthlyCostPedsHIVNegativeNonexposed;
		double monthlyCostPedsHIVNegativeExposed;
		double monthlyCostPedsHIVPositive[PEDS_HIV_POS_NUM][ART_NUM_STATES];
		//Peds BC5-BC14
		double maxCD4Percentage[PEDS_AGE_EARLY_NUM];
		//Peds BK6-BL7
		int CD4TestingIntervalPreARTEarly;
		int CD4TestingIntervalPreARTLate;
		int HVLTestingIntervalPreARTEarly;
		int HVLTestingIntervalPreARTLate;

		//Peds BC52-BH72
		int stageBoundsChronicAIDSDeathProbOnARTMultEarly[2];
		double chronicAIDSDeathProbOnARTMultEarly[PEDS_CD4_PERC_NUM][3];
		int stageBoundsMonthlyOIProbOnARTMultEarly[2];
		double monthlyOIProbOnARTMultEarly[PEDS_CD4_PERC_NUM][3];
		//Peds BC77-BQ91
		double chronicAIDSDeathProbOnARTMultLate[HIST_EXT_NUM][CD4_NUM_STRATA];
		double monthlyOIProbOnARTMultLate[CD4_NUM_STRATA][OI_NUM];

		/*Peds ART start policy class */
		class ARTStartPolicy {
		public:
			int CD4PercStageMonths[NUM_ART_START_CD4PERC_PEDS-1];
			double CD4PercBounds[NUM_ART_START_CD4PERC_PEDS][ART_NUM_LINES][NUM_BOUNDS];
			int HVLBounds[ART_NUM_LINES][NUM_BOUNDS];
			bool OIHistory[ART_NUM_LINES][OI_NUM];
			int numOIs[ART_NUM_LINES];
			int minMonthNum[ART_NUM_LINES];
			int monthsSincePrevRegimen[ART_NUM_LINES];
		};

		/*Peds ART Observed failure policy class */
		class ARTFailPolicy {
		public:
			int HVLNumIncrease;
			int HVLBounds[NUM_BOUNDS];
			bool HVLFailAtSetpoint;
			int HVLMonthsFromInit;
			double CD4PercPercentageDrop;
			bool CD4PercBelowPreARTNadir;
			double CD4PercBoundsOR[NUM_BOUNDS];
			double CD4PercBoundsAND[NUM_BOUNDS];
			int CD4PercMonthsFromInit;
			ART_FAIL_BY_OI OIsEvent[OI_NUM];
			int OIsMinNum;
			int OIsMonthsFromInit;
			int diagnoseNumTestsFail;
			bool diagnoseUseHVLTestsConfirm;
			bool diagnoseUseCD4TestsConfirm;
			int diagnoseNumTestsConfirm;
		};

		/*Peds ART stopping policy class */
		class ARTStopPolicy {
		public:
			int maxMonthsOnART;
			bool withMajorToxicty;
			bool afterFailImmediate;
			double afterFailCD4PercLowerBound;
			bool afterFailWithSevereOI;
			int afterFailMonthsFromObserved;
			int afterFailMinMonthNum;
			int afterFailMonthsFromInit;
		};
		/*Peds CO5-DA34 */
		ARTStartPolicy startART;

		/*Peds CR39-DA74 */
		ARTFailPolicy failART[ART_NUM_LINES];

		/*Peds CR79-DA87 */
		ARTStopPolicy stopART[ART_NUM_LINES];

		/* Prophylaxis policy classes */
		class ProphStartPolicy {
			public:
				double ageBounds[NUM_BOUNDS][OI_NUM];
				double currCD4PercBounds[NUM_BOUNDS][OI_NUM];
				int OIHistory[OI_NUM][OI_NUM];
				CONDITIONS_TYPE firstCondition;
				CONDITIONS_TYPE secondCondition;
				DIRECTIONS_TYPE parDirection;
		};
		class ProphStopPolicy {
			public:
				double ageLowerBound[OI_NUM];
				double currCD4PercLowerBound[OI_NUM];
				int OIHistory[OI_NUM][OI_NUM];
				int monthsOnProph[OI_NUM];
				CONDITIONS_TYPE firstCondition;
				CONDITIONS_TYPE secondCondition;
				DIRECTIONS_TYPE parDirection;
		};

		/* Proph policy inputs */
		//Peds BW5-CK121
		ProphStartPolicy startProph[PROPH_NUM_TYPES];
		ProphStopPolicy stopProph[PROPH_NUM_TYPES];
	};

	/* PedsARTInputs class contains inputs from the PedsARTs tab,
		one for each ART line in the simulation context */
	class PedsARTInputs {
	public:
		//PedsARTs G5-H10
		double costInitial[PEDS_ART_COST_AGE_CAT_NUM];
		double costMonthly[PEDS_ART_COST_AGE_CAT_NUM];

		//PedsARTs C9-C10
		int efficacyTimeHorizonEarly;
		int efficacyTimeHorizonLate;
		//PedsARTs C14-I20
		double probInitialEfficacyEarly[ART_EFF_NUM_TYPES][HVL_NUM_STRATA];
		double probInitialEfficacyLate[ART_EFF_NUM_TYPES][HVL_NUM_STRATA];
		//PedsARTs C24-E25
		double partialSuppressionDistributionEarly[ART_NUM_HVL_PARTIAL];
		double partialSuppressionDistributionLate[ART_NUM_HVL_PARTIAL];
		//PedsARTs C29-H33
		/**
		double probLateFailFromSuppressEarly;
		double probLatePartialSuppressFromSuppressEarly;
		double probLateFailFromPartialSuppressEarly;
		**/
		int forceFailAtMonthEarly;
		/**
		double probLateFailFromSuppressLate;
		double probLatePartialSuppressFromSuppressLate;
		double probLateFailFromPartialSuppressLate;
		**/
		int forceFailAtMonthLate;
		//PedsARTs C37-H183
		int stageBoundsCD4PercentageChangeOnARTEarly[ART_EFF_NUM_TYPES-1][2];
		int stageBoundCD4PercentageChangeOnARTFailEarly;
		double CD4PercentageChangeOnARTMeanEarly[ART_EFF_NUM_TYPES-1][PEDS_AGE_EARLY_NUM][CD4_RESPONSE_NUM_TYPES][3];
		double CD4PercentageChangeOnARTStdDevEarly[ART_EFF_NUM_TYPES-1][PEDS_AGE_EARLY_NUM][CD4_RESPONSE_NUM_TYPES][3];
		double CD4PercentageMultiplierOnFailedARTEarly[CD4_RESPONSE_NUM_TYPES][2];
		double secondaryCD4PercentageChangeOnARTStdDevEarly;
		int stageBoundsCD4ChangeOnARTLate[ART_EFF_NUM_TYPES-1][2];
		int stageBoundCD4ChangeOnARTFailLate;
		double CD4ChangeOnARTMeanLate[ART_EFF_NUM_TYPES-1][CD4_RESPONSE_NUM_TYPES][3];
		double CD4ChangeOnARTStdDevLate[ART_EFF_NUM_TYPES-1][CD4_RESPONSE_NUM_TYPES][3];
		double CD4MultiplierOnFailedARTLate[CD4_RESPONSE_NUM_TYPES][2];
		double secondaryCD4ChangeOnARTStdDevLate;
		//PedsARTs C189-D198
		double monthlyCD4PercentageMultiplierOffARTPreSetpointEarly[ART_EFF_NUM_TYPES];
		double monthlyCD4PercentageMultiplierOffARTPostSetpointEarly[ART_EFF_NUM_TYPES];
		double monthlyCD4MultiplierOffARTPreSetpointLate[ART_EFF_NUM_TYPES];
		double monthlyCD4MultiplierOffARTPostSetpointLate[ART_EFF_NUM_TYPES];
		//UserARTs C203-D211
		double monthlyProbHVLChangeEarly[ART_EFF_NUM_TYPES];
		int monthlyNumStrataHVLChangeEarly[ART_EFF_NUM_TYPES];
		double monthlyProbHVLChangeLate[ART_EFF_NUM_TYPES];
		int monthlyNumStrataHVLChangeLate[ART_EFF_NUM_TYPES];

		//Peds ART C229
		double propRespondARTRegimenLogitMeanEarly;
		//Peds ART D229
		double propRespondARTRegimenLogitStdDevEarly;
		//Peds Art C232-D240
		double responseTypeThresholdsEarly[HET_NUM_OUTCOMES][RESP_NUM_TYPES-1];
		//Peds Art E232-F240
		double responseTypeValuesEarly[HET_NUM_OUTCOMES][RESP_NUM_TYPES-1];
		//Peds ART G232-G240
		double responseTypeExponentsEarly[HET_NUM_OUTCOMES];
		//PedsARTs F219
		double probFillARTPrescriptionsNonResponderEarly;
		//PedsARTs D223-D225
		double probRestartARTRegimenAfterFailureEarly[RESP_NUM_TYPES];

		//Peds ART C253
		double propRespondARTRegimenLogitMeanLate;
		//Peds ART D253
		double propRespondARTRegimenLogitStdDevLate;
		//Peds Art C256-D264
		double responseTypeThresholdsLate[HET_NUM_OUTCOMES][RESP_NUM_TYPES-1];
		//Peds Art E256-F264
		double responseTypeValuesLate[HET_NUM_OUTCOMES][RESP_NUM_TYPES-1];
		//Peds ART G256-G264
		double responseTypeExponentsLate[HET_NUM_OUTCOMES];
		//PedsARTs F243
		double probFillARTPrescriptionsNonResponderLate;
		//PedsARTs D247-D249
		double probRestartARTRegimenAfterFailureLate[RESP_NUM_TYPES];
	}; /* end PedsARTInputs */

	/* PedsCostInputs class contains inputs from the Cost tab,
		one per simulation context */
	class PedsCostInputs {
	public:
		//Cost C7-M39
		double acuteOICostTreated[PEDS_COST_AGE_CAT_NUM][ART_NUM_STATES][OI_NUM][COST_NUM_TYPES];
		double acuteOICostUntreated[PEDS_COST_AGE_CAT_NUM][ART_NUM_STATES][OI_NUM][COST_NUM_TYPES];
		//Cost C45-M45
		double CD4TestCost[PEDS_COST_AGE_CAT_NUM][COST_NUM_TYPES];
		double HVLTestCost[PEDS_COST_AGE_CAT_NUM][COST_NUM_TYPES];
		//Cost C51-M87
		double deathCostTreated[PEDS_COST_AGE_CAT_NUM][ART_NUM_STATES][DTH_NUM_CAUSES_BASIC][COST_NUM_TYPES];
		double deathCostUntreated[PEDS_COST_AGE_CAT_NUM][ART_NUM_STATES][DTH_NUM_CAUSES_BASIC][COST_NUM_TYPES];
		//Cost S6-AF65
		double routineCareCostHIVNegative[GENDER_NUM][PEDS_COST_AGE_CAT_NUM][COST_NUM_TYPES];
		double routineCareCostHIVPositive[ART_NUM_STATES][CD4_NUM_STRATA][GENDER_NUM][PEDS_COST_AGE_CAT_NUM][COST_NUM_TYPES];
	}; /* end PedsCostInputs */

	/* readInputs function reads in all the inputs from the given input file,
		throws exception if there is an error */
	void readInputs();

	/* public accessor functions that return const pointers to the input data classes */
	const RunSpecsInputs *getRunSpecsInputs();
	const CohortInputs *getCohortInputs();
	const TreatmentInputs *getTreatmentInputs();
	const LTFUInputs *getLTFUInputs();
	const HeterogeneityInputs *getHeterogeneityInputs();
	const STIInputs *getSTIInputs();
	const ProphInputs *getProphInputs(int prophType, int OINum, int prophNum);
	const ProphInputs *getPedsProphInputs(int prophType, int OINum, int prophNum);
	const ARTInputs *getARTInputs(int artLineNum);
	const NatHistInputs *getNatHistInputs();
	const CHRMsInputs *getCHRMsInputs();
	const CostInputs *getCostInputs();
	const TBInputs *getTBInputs();
	const QOLInputs *getQOLInputs();
	const HIVTestInputs *getHIVTestInputs();
	const PedsInputs *getPedsInputs();
	const PedsCostInputs *getPedsCostInputs();
	const PedsARTInputs *getPedsARTInputs(int artLineNum);

private:
	/* Input file name and file pointer */
	string inputFileName;
	FILE *inputFile;

	/* Classes for storing the input data */
	RunSpecsInputs runSpecsInputs;
	CohortInputs cohortInputs;
	TreatmentInputs treatmentInputs;
	LTFUInputs ltfuInputs;
	HeterogeneityInputs heterogeneityInputs;
	STIInputs stiInputs;
	ProphInputs *prophsInputs[PROPH_NUM_TYPES][OI_NUM][PROPH_NUM];
	ProphInputs *pedsProphsInputs[PROPH_NUM_TYPES][OI_NUM][PROPH_NUM];
	ARTInputs *artInputs[ART_NUM_LINES];
	NatHistInputs natHistInputs;
	CHRMsInputs chrmsInputs;
	CostInputs costInputs;
	TBInputs tbInputs;
	QOLInputs qolInputs;
	HIVTestInputs testingInputs;
	PedsInputs pedsInputs;
	PedsARTInputs *pedsARTInputs[ART_NUM_LINES];
	PedsCostInputs pedsCostInputs;

	/* Private functions for reading in the inputs, called by readInputs */
	void readRunSpecsInputs();
	void readCohortInputs();
	void readTreatmentInputsPart1();
	void readTreatmentInputsPart2();
	void readLTFUInputs();
	void readHeterogeneityInputs();
	void readSTIInputs();
	void readProphInputs();
	void readPedsProphInputs();
	void readARTInputs();
	void readNatHistInputs();
	void readCHRMsInputs();
	void readCostInputs();
	void readTBInputs();
	void readQOLInputs();
	void readHIVTestInputs();
	void readPedsInputs();
	void readPedsARTInputs();
	void readPedsCostInputs();
	bool readAndSkipPast(const char* searchStr, FILE* file);
	bool readAndSkipPast2(const char* searchStr1, const char *searchStr2, FILE *file);

};

/* getRunSpecsInputs returns a const pointer to the RunSpecsInputs data class */
inline const SimContext::RunSpecsInputs *SimContext::getRunSpecsInputs() {
	return &runSpecsInputs;
}

/* getCohortInputs returns a const pointer to the CohortInputs data class */
inline const SimContext::CohortInputs *SimContext::getCohortInputs() {
	return &cohortInputs;
}

/* getTreatmentInputs returns a const pointer to the TreatmentInputs data class */
inline const SimContext::TreatmentInputs *SimContext::getTreatmentInputs() {
	return &treatmentInputs;
}

/* getLTFUInputs returns a const pointer to the LTFUInputs data class */
inline const SimContext::LTFUInputs *SimContext::getLTFUInputs() {
	return &ltfuInputs;
}

/* getHeterogeneityInputs returns a const pointer to the HeterogeneityInputs data class */
inline const SimContext::HeterogeneityInputs *SimContext::getHeterogeneityInputs() {
	return &heterogeneityInputs;
}

/* getSTIInputs returns a const pointer to the STIInputs data class */
inline const SimContext::STIInputs *SimContext::getSTIInputs() {
	return &stiInputs;
}

/* getProphInputs returns a const pointer to the specified ProphInputs data class */
inline const SimContext::ProphInputs *SimContext::getProphInputs(int prophType, int OINum, int prophNum) {
	assert(prophType < PROPH_NUM_TYPES);
	assert(OINum < OI_NUM);
	assert(prophNum < PROPH_NUM);

	return prophsInputs[prophType][OINum][prophNum];
}

/* getPedsProphInputs returns a const pointer to the specified ProphInputs data class */
inline const SimContext::ProphInputs *SimContext::getPedsProphInputs(int prophType, int OINum, int prophNum) {
	assert(prophType < PROPH_NUM_TYPES);
	assert(OINum < OI_NUM);
	assert(prophNum < PROPH_NUM);

	return pedsProphsInputs[prophType][OINum][prophNum];
}

/* getARTInputs returns a const pointer to the specified ARTInputs data class */
inline const SimContext::ARTInputs *SimContext::getARTInputs(int artLineNum) {

	assert(artLineNum < ART_NUM_LINES);
	return artInputs[artLineNum];
}

/* getNatHistInputs returns a const pointer to the NatHistInputs data class */
inline const SimContext::NatHistInputs *SimContext::getNatHistInputs() {
	return &natHistInputs;
}

/* getCHRMsInputs returns a const pointer to the CHRMsInputs data class */
inline const SimContext::CHRMsInputs *SimContext::getCHRMsInputs() {
	return &chrmsInputs;
}

/* getCostInputs returns a const pointer to the CostInputs data class */
inline const SimContext::CostInputs *SimContext::getCostInputs() {
	return &costInputs;
}

/* getTBInputs returns a const pointer to the TBInputs data class */
inline const SimContext::TBInputs *SimContext::getTBInputs() {
	return &tbInputs;
}

/* getQOLInputs returns a const pointer to the QOLInputs data class */
inline const SimContext::QOLInputs *SimContext::getQOLInputs() {
	return &qolInputs;
}

/* getHIVTestInputs returns a const pointer to the HIVTestInputs data class */
inline const SimContext::HIVTestInputs *SimContext::getHIVTestInputs() {
	return &testingInputs;
}

/* getPedsInputs returns a const pointer to the PedsInputs data class */
inline const SimContext::PedsInputs *SimContext::getPedsInputs() {
	return &pedsInputs;
}

/* getPedsARTInputs returns a const pointer to the specified PedsARTInputs data class */
inline const SimContext::PedsARTInputs *SimContext::getPedsARTInputs(int artLineNum) {

	assert(artLineNum < ART_NUM_LINES);
	return pedsARTInputs[artLineNum];
}

/* getCostInputs returns a const pointer to the CostInputs data class */
inline const SimContext::PedsCostInputs *SimContext::getPedsCostInputs() {
	return &pedsCostInputs;
}

