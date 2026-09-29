#ifndef FOCUSGROUP_HPP
#define FOCUSGROUP_HPP

#include <string>
#include <vector>

namespace transm {

/** The FOCUS module screens twelve cohorts, defined by race/ethnicity x sex x
 * the reason the cohort is targeted (undiagnosed HIV, or lost to follow up).
 *
 * The ordering below is the positional schema that the whole module already
 * relies on: it is the index order of Simulation::db_YearlyCounts_<year> /
 * db_YearlyProbs_<year>, and of the parallel name/finder arrays in
 * Population::UpdateForFOCUSAnalysis(). This enum does not introduce a new
 * convention, it only gives the existing one a name. Do NOT reorder these
 * without updating every one of those tables.
 *
 * Calendar: the module treats simulation month 600 as January 2007, which is
 * what the scenario's epidemic seed assumes but not what <monthOf1990> says.
 * See the CALENDAR CONVENTION note in Simulation's FOCUS block before changing
 * any FOCUS month number.
 *
 * Note that the cohorts are targeted, not partitioned: the Population::Find*
 * predicates test race with isHispanic()/isBlack()/isWhite() independently, so
 * a Black Hispanic person is reachable through both the Hispanic and the Black
 * cohort. FocusGroup therefore records which cohort a person was *drawn from*,
 * not a canonical classification of that person. */
enum class FocusGroup {
    HISPANIC_MALE_UNDIAGNOSED = 0,
    HISPANIC_FEMALE_UNDIAGNOSED,
    BLACK_MALE_UNDIAGNOSED,
    BLACK_FEMALE_UNDIAGNOSED,
    WHITE_MALE_UNDIAGNOSED,
    WHITE_FEMALE_UNDIAGNOSED,
    HISPANIC_MALE_LTFU,
    HISPANIC_FEMALE_LTFU,
    BLACK_MALE_LTFU,
    BLACK_FEMALE_LTFU,
    WHITE_MALE_LTFU,
    WHITE_FEMALE_LTFU,
    ENDType,
    Last = ENDType,
    First = HISPANIC_MALE_UNDIAGNOSED
};

/** Number of FOCUS cohorts; the length of every per-year FOCUS data table */
const int FocusGroupCount = (int)FocusGroup::ENDType;

/** Sentinel written into per-entity FOCUS fields for "this never happened".
 * Chosen as -1 so that the exported node attributes stay numeric and can be
 * filtered in Gephi with a single "< 0" range. */
const int FocusGroupNone = -1;

/** Why a cohort is targeted. The first half of the schema is undiagnosed HIV,
 * the second half is loss to follow up. */
enum class FocusReason {
    UNDIAGNOSED = 0,
    LTFU = 1
};

/** Machine-friendly cohort labels, indexed by FocusGroup.
 * RACE:SEX:REASON so that a Gephi partition sorts sensibly and so that the
 * label can be split on ':' downstream. */
const std::vector<std::string> FocusGroupStrs =
{
    "HISPANIC:MALE:UNDIAGNOSED",
    "HISPANIC:FEMALE:UNDIAGNOSED",
    "BLACK:MALE:UNDIAGNOSED",
    "BLACK:FEMALE:UNDIAGNOSED",
    "WHITE:MALE:UNDIAGNOSED",
    "WHITE:FEMALE:UNDIAGNOSED",
    "HISPANIC:MALE:LTFU",
    "HISPANIC:FEMALE:LTFU",
    "BLACK:MALE:LTFU",
    "BLACK:FEMALE:LTFU",
    "WHITE:MALE:LTFU",
    "WHITE:FEMALE:LTFU"
};

/** Human-readable cohort labels, indexed by FocusGroup. These are the strings
 * the monthly FOCUS log has always printed. */
const std::vector<std::string> FocusGroupNames =
{
    "Hispanic Males Undiagnosed",
    "Hispanic Females Undiagnosed",
    "Black Males Undiagnosed",
    "Black Females Undiagnosed",
    "White Males Undiagnosed",
    "White Females Undiagnosed",
    "Hispanic Males LTFU",
    "Hispanic Females LTFU",
    "Black Males LTFU",
    "Black Females LTFU",
    "White Males LTFU",
    "White Females LTFU"
};

/** @return true if group is a valid FocusGroup index */
inline bool isFocusGroupValid(int group) {
    return group >= 0 && group < FocusGroupCount;
}

/** @return the machine-friendly label, or "NONE" for an unset/invalid index */
inline std::string focusGroupStr(int group) {
    return isFocusGroupValid(group) ? FocusGroupStrs[(std::size_t)group] : "NONE";
}

/** @return the human-readable label, or "None" for an unset/invalid index */
inline std::string focusGroupName(int group) {
    return isFocusGroupValid(group) ? FocusGroupNames[(std::size_t)group] : "None";
}

/** @return FocusReason for a cohort as an int, or FocusGroupNone if unset.
 * The first six cohorts are undiagnosed, the last six are LTFU. */
inline int focusGroupReason(int group) {
    if (!isFocusGroupValid(group))
        return FocusGroupNone;
    return (group < FocusGroupCount / 2) ? (int)FocusReason::UNDIAGNOSED
                                         : (int)FocusReason::LTFU;
}

/** Composite FOCUS state of an entity, exported as a single node attribute so
 * that the whole reach-and-yield story can be coloured with one Gephi
 * partition instead of stacking several filters. Mirrors the existing
 * hiv_composite idiom in the network export. */
enum class FocusStatus {
    NOT_ELIGIBLE = -1,   /**< never met the FOCUS targeting criteria */
    ELIGIBLE = 0,        /**< eligible but never drawn into a screening sample */
    SCREENED = 1,        /**< sampled at least once, never won the selection roll */
    SELECTED_PAST = 2,   /**< selected by FOCUS more than twelve months ago */
    SELECTED_RECENT = 3  /**< selected by FOCUS within the last twelve months */
};

/** Window, in months, that separates SELECTED_RECENT from SELECTED_PAST */
const int FocusRecentWindowMonths = 12;

} // namespace transm

#endif /* FOCUSGROUP_HPP */
