#include "sexualbehavior.hpp"
#include "entity.hpp"

namespace transm {

void SexualBehavior::SetHighRiskMultiplier(double multiplier)
{
	acquisitionRatePerMonth[Entity::HIGH] = acquisitionRatePerMonth[Entity::LOW];
	acquisitionRatePerMonth[Entity::HIGH].mu += log(multiplier);
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
	return acquisitionRatePerMonth[risk];
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
	return coitalEventsPerMonth[risk];
}

const BetaDist SexualBehavior::getChanceCondomUsePerEvent(Entity::RiskLevel risk) const
{
	return chanceCondomUsePerEvent[risk];
}

const ShiftedLogNormalDist SexualBehavior::getPartnershipDurationMth(Entity::RiskLevel risk) const
{
	return partnershipDurationMth[risk];
}

} // namespace transm
