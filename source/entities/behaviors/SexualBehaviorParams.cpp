#include "SexualBehaviorParams.h"
#include "../../util/XMLUtil.h"
#include "../Person.h"
#include "boost/lexical_cast.hpp"

SexualBehaviorParams::SexualBehaviorParams(const PopulationSettings::MaleSettings &settings, SexualPartnership::Type type, EventParams &params)
{
	params.displayOut("SexualBehaviorParams::loadParamsXML(...)\n");

	double multiplier = settings.high_risk_multiplier;

	if(type == SexualPartnership::Type::Csw && settings.enable_high_risk_multiplier && settings.enable_csw_high_risk_multiplier)
	{
		multiplier = settings.csw_high_risk_multiplier;
	}

	acquisitionRatePerMonth[Person::LOW] = settings.partnership_settings.at(type).acquisition_rate_low_risk;

	params.displayOut("Average acquisition rate for relationship type "
		+ SexualPartnership::TypeStrings.at(type) + " and risk level LOW is " +
		std::to_string(acquisitionRatePerMonth[Person::LOW].getMean()) + "\n");

	if(settings.enable_high_risk_multiplier)
	{
		acquisitionRatePerMonth[Person::HIGH] = settings.partnership_settings.at(type).acquisition_rate_low_risk;
		acquisitionRatePerMonth[Person::HIGH].mu += log(multiplier);
	}
	else
	{
		acquisitionRatePerMonth[Person::HIGH] = settings.partnership_settings.at(type).acquisition_rate_high_risk;
	}

	params.displayOut("Average acquisition rate for relationship type "
		+ SexualPartnership::TypeStrings.at(type) + " and risk level HIGH is " +
		std::to_string(acquisitionRatePerMonth[Person::HIGH].getMean()) + "\n");

	for(const auto &bucket_settings : settings.partnership_settings.at(type).available_buckets)
	{
		SexualBehaviorParams::AvailableBucket bucket;
		bucket.dmgProfileSelector.parse(bucket_settings.first);
		bucket.weight = bucket_settings.second;
		availableBuckets.push_back(bucket);
	}

	averageYearsYounger = settings.partnership_settings.at(type).average_years_younger;

	coitalEventsPerMonth[Person::LOW] = settings.partnership_settings.at(type).coital_events_per_month_low_risk;
	coitalEventsPerMonth[Person::HIGH] = settings.partnership_settings.at(type).coital_events_per_month_high_risk;

	chanceCondomUsePerEvent[Person::LOW] = settings.partnership_settings.at(type).chance_condom_user_per_event_low_risk;
	chanceCondomUsePerEvent[Person::HIGH] = settings.partnership_settings.at(type).chance_condom_user_per_event_low_risk;

	partnershipDurationMth[Person::LOW] = settings.partnership_settings.at(type).partnership_duration_months_low_risk;
	partnershipDurationMth[Person::HIGH] = settings.partnership_settings.at(type).partnership_duration_months_high_risk;
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
