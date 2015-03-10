#include "sexualbehavior.hpp"
#include "entity.hpp"

namespace transm {

void SexualBehavior::SetHighRiskMultiplier(double multiplier)
{
    acquisitionRatePerMonth[(std::size_t)Entity::RiskLevel::HIGH] = acquisitionRatePerMonth[(std::size_t)Entity::RiskLevel::LOW];
    acquisitionRatePerMonth[(std::size_t)Entity::RiskLevel::HIGH].mu += log(multiplier);
}

void SexualBehavior::ApplyCoefficientVariation(double /*coefficient*/)
{
	throw std::runtime_error("not implemented");
}

unsigned int SexualBehavior::getNumAvailableBuckets()  const
{
	return (unsigned int)availableBuckets.size();
}


SexualPartnership::Type SexualBehavior::getPartnershipType() const
{
	return partnershipType;
}

const LogNormalDist SexualBehavior::getAcquisitionRatePerMonth(Entity::RiskLevel risk) const
{
    return acquisitionRatePerMonth[(std::size_t)risk];
}

const SexualBehavior::AvailableBucket SexualBehavior::getAvailableBucket(int _bucket) const
{
	return availableBuckets.at(_bucket);
}

const NormalDist SexualBehavior::getAverageYearsYounger() const
{
	return averageYearsYounger;
}

double SexualBehavior::getCoitalEventsPerMonth(Entity::RiskLevel risk) const
{
    return coitalEventsPerMonth[(std::size_t)risk];
}

const BetaDist SexualBehavior::getChanceCondomUsePerEvent(Entity::RiskLevel risk) const
{
    return chanceCondomUsePerEvent[(std::size_t)risk];
}

const ShiftedLogNormalDist SexualBehavior::getPartnershipDurationMth(Entity::RiskLevel risk) const
{
    return partnershipDurationMth[(std::size_t)risk];
}

} // namespace transm
