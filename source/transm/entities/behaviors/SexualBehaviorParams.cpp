#include "SexualBehaviorParams.h"
#include "../Person.h"

void SexualBehaviorParams::SetHighRiskMultiplier(double multiplier)
{
	acquisitionRatePerMonth[Person::HIGH] = acquisitionRatePerMonth[Person::LOW];
	acquisitionRatePerMonth[Person::HIGH].mu += log(multiplier);
}

void SexualBehaviorParams::ApplyCoefficientVariation(double /*coefficient*/)
{
	throw std::runtime_error("not implemented");
}

unsigned int SexualBehaviorParams::getNumAvailableBuckets()  const
{
	return (unsigned int)availableBuckets.size();
}


SexualPartnership::Type SexualBehaviorParams::getPartnershipType() const
{
	return partnershipType;
}

const LogNormalDist SexualBehaviorParams::getAcquisitionRatePerMonth(Person::RiskLevel risk) const
{
	return acquisitionRatePerMonth[risk];
}

const SexualBehaviorParams::AvailableBucket SexualBehaviorParams::getAvailableBucket(int _bucket) const
{
	return availableBuckets.at(_bucket);
}

const NormalDist SexualBehaviorParams::getAverageYearsYounger() const
{
	return averageYearsYounger;
}

double SexualBehaviorParams::getCoitalEventsPerMonth(Person::RiskLevel risk) const
{
	return coitalEventsPerMonth[risk];
}

const BetaDist SexualBehaviorParams::getChanceCondomUsePerEvent(Person::RiskLevel risk) const
{
	return chanceCondomUsePerEvent[risk];
}

const ShiftedLogNormalDist SexualBehaviorParams::getPartnershipDurationMth(Person::RiskLevel risk) const
{
	return partnershipDurationMth[risk];
}
