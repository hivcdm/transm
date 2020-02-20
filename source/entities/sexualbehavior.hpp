#pragma once

#include <vector>

#include "entitytypes.hpp"
#include "demographicprofile.hpp"
#include "sexualpartnership.hpp"

namespace transm {

/// <summary>
/// This can parse XML that contains info about sexual behavior for a particular
/// type of partnership.
/// </summary>
/// <remarks>
/// So far, only males have detailed sexual behavior. There should be 1 XML -
/// subtree for every SexualPartnership::Type for each entity type.
/// </remarks>
class SexualBehavior
{

public :
    SexualBehavior() : SexualBehavior(SexualPartnership::Type::Last) { }

	SexualBehavior(SexualPartnership::Type type) : partnershipType(type) { }

	void SetHighRiskMultiplier(double multiplier);

	/// <summary>
	/// Returns number of Buckets that are available for this kind of partnership.
	/// This is used to allow class Male to set aside some temporary space
	/// </summary>
	unsigned int getNumAvailableBuckets() const;

	const LogNormalDist getAcquisitionRatePerMonth(RiskLevel risk) const;

	double getCoitalEventsPerMonth(RiskLevel risk) const;

	const BetaDist getChanceCondomUsePerEvent(RiskLevel risk) const;

	const NormalDist getAverageYearsYounger() const;

	const ShiftedLogNormalDist getPartnershipDurationMth(RiskLevel risk) const;

	SexualPartnership::Type getPartnershipType() const;

	void setChanceCondomUsePerEvent(RiskLevel risk, BetaDist dist) { chanceCondomUsePerEvent[static_cast<std::size_t>(risk)] = dist; }

    void setCoitalEventsPerMonth(RiskLevel risk, double meanEvents) { coitalEventsPerMonth[static_cast<std::size_t>(risk)] = meanEvents; }

    void setPartnershipDuration(RiskLevel risk, ShiftedLogNormalDist dist) { partnershipDurationMth[static_cast<std::size_t>(risk)] = dist; }

	void setAverageYearsYounger(NormalDist dist) { averageYearsYounger = dist; }

    void setAcquisitionRatePerMonth(RiskLevel risk, LogNormalDist dist) { acquisitionRatePerMonth[static_cast<std::size_t>(risk)] = dist; }

    void setChanceChooseWithSteady(double chance) { chanceChooseWithSteady = chance; }
    double getChanceChooseWithSteady() { return chanceChooseWithSteady; }

private:
	friend class SimulationBuilder;

    double chanceChooseWithSteady;

	//the partnership type that these parameters represent
	SexualPartnership::Type partnershipType;

	//LogNormal distribution from which the people draw a rate to acquire this type of partner
	LogNormalDist acquisitionRatePerMonth[(std::size_t)RiskLevel::Last];

	//average number of partners men acquire at a time
	//double averagePartnersAtATime[(std::size_t)RiskLevel::Last];

	//The distribution the males will draw from to determine how many years younger their partner should be (resulting difference may be negative for older women)
	NormalDist averageYearsYounger;

	//avg events per month across all Couples; will be used as a mean in Poisson distribution
    double coitalEventsPerMonth[(std::size_t)RiskLevel::Last];

	//chance per event that this person will use a condom
    BetaDist chanceCondomUsePerEvent[(std::size_t)RiskLevel::Last];

	//avg duration if partnerships across all Couples
    ShiftedLogNormalDist partnershipDurationMth[(std::size_t)RiskLevel::Last];
};

} // namespace transm
