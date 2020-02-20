#include "sexualbehavior.hpp"

namespace transm {

void SexualBehavior::SetHighRiskMultiplier(double multiplier)
{
    acquisitionRatePerMonth[(std::size_t)RiskLevel::HIGH] = acquisitionRatePerMonth[(std::size_t)RiskLevel::LOW];
    acquisitionRatePerMonth[(std::size_t)RiskLevel::HIGH].mu += log(multiplier);
}

SexualPartnership::Type SexualBehavior::getPartnershipType() const
{
	return partnershipType;
}

const LogNormalDist SexualBehavior::getAcquisitionRatePerMonth(RiskLevel risk) const
{
    return acquisitionRatePerMonth[(std::size_t)risk];
}

const NormalDist SexualBehavior::getAverageYearsYounger() const
{
	return averageYearsYounger;
}

double SexualBehavior::getCoitalEventsPerMonth(RiskLevel risk) const
{
    return coitalEventsPerMonth[(std::size_t)risk];
}

const BetaDist SexualBehavior::getChanceCondomUsePerEvent(RiskLevel risk) const
{
    return chanceCondomUsePerEvent[(std::size_t)risk];
}

const ShiftedLogNormalDist SexualBehavior::getPartnershipDurationMth(RiskLevel risk) const
{
    return partnershipDurationMth[(std::size_t)risk];
}

} // namespace transm
