#include "SexualBehaviorParams.h"
#include "../Person.h"

void SexualBehaviorParams::Deserialize(const pugi::xml_node &node)
{
	std::string type_string = node.child("type").text().as_string();
	if(type_string == "CSW") partnershipType = SexualPartnership::Type::Csw;
	if(type_string == "Casual") partnershipType = SexualPartnership::Type::Casual;
	if(type_string == "Regular") partnershipType = SexualPartnership::Type::Regular;
	if(type_string == "Steady") partnershipType = SexualPartnership::Type::Steady;

	acquisitionRatePerMonth[Person::LOW].Deserialize(node.child("acquisitionRateLowRisk"));
	acquisitionRatePerMonth[Person::HIGH].Deserialize(node.child("acquisitionRateHighRisk"));

	for(const auto &bucket_settings : node.select_nodes("selectionCriteria/availableBuckets/bucket"))
	{
		SexualBehaviorParams::AvailableBucket bucket;
		bucket.dmgProfileSelector.parse(bucket_settings.node().child("DmgProfile").text().as_string());
		bucket.weight = bucket_settings.node().child("weightedValue").text().as_double();
		availableBuckets.push_back(bucket);
	}

	averageYearsYounger.Deserialize(node.child("selectionCriteria").child("AverageYearsYounger"));

	//XXX:this should be a double, but old implementations mistakenly casted it to int
	//we will continue to do this to maintain reproduciblity for now
	coitalEventsPerMonth[Person::LOW] = node.select_single_node("coitalEventsPerMonthLowRisk/Distrib/mean").node().text().as_int();
	coitalEventsPerMonth[Person::HIGH] = node.select_single_node("coitalEventsPerMonthHighRisk/Distrib/mean").node().text().as_int();

	chanceCondomUsePerEvent[Person::LOW].Deserialize(node.child("chanceCondomUsePerEventLowRisk"));
	chanceCondomUsePerEvent[Person::HIGH].Deserialize(node.child("chanceCondomUsePerEventHighRisk"));

	partnershipDurationMth[Person::LOW].Deserialize(node.child("partnershipDurationMthLowRisk"));
	partnershipDurationMth[Person::HIGH].Deserialize(node.child("partnershipDurationMthHighRisk"));
}

void SexualBehaviorParams::SetHighRiskMultiplier(double multiplier)
{
	acquisitionRatePerMonth[Person::HIGH] = acquisitionRatePerMonth[Person::LOW];
	acquisitionRatePerMonth[Person::HIGH].mu += log(multiplier);
}

void SexualBehaviorParams::ApplyCoefficientVariation(double coefficient)
{
	throw std::runtime_error("not implemented");
}

void SexualBehaviorParams::Serialize(pugi::xml_node &)
{
	throw std::runtime_error("not implemented");
}

SexualBehaviorParams::SexualBehaviorParams()
{
}

unsigned int SexualBehaviorParams::getNumAvailableBuckets()  const
{
	return availableBuckets.size();
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

const double SexualBehaviorParams::getCoitalEventsPerMonth(Person::RiskLevel risk) const
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
