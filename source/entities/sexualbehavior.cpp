#include "sexualbehavior.hpp"
#include "person.hpp"

void SexualBehavior::SetHighRiskMultiplier(double multiplier)
{
	acquisitionRatePerMonth[Person::HIGH] = acquisitionRatePerMonth[Person::LOW];
	acquisitionRatePerMonth[Person::HIGH].mu += log(multiplier);
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

const LogNormalDist SexualBehavior::getAcquisitionRatePerMonth(Person::RiskLevel risk) const
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

double SexualBehavior::getCoitalEventsPerMonth(Person::RiskLevel risk) const
{
	return coitalEventsPerMonth[risk];
}

const BetaDist SexualBehavior::getChanceCondomUsePerEvent(Person::RiskLevel risk) const
{
	return chanceCondomUsePerEvent[risk];
}

const ShiftedLogNormalDist SexualBehavior::getPartnershipDurationMth(Person::RiskLevel risk) const
{
	return partnershipDurationMth[risk];
}
