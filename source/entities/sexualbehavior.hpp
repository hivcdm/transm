#pragma once

#include <vector>

#include "person.hpp"
#include "demographicprofile.hpp"
#include "sexualpartnership.hpp"

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
	/// <summary>
	/// Different SexualPartnership can only involve certain categories of people.
	/// </summary>
	struct AvailableBucket
	{
		/// <summary>
		/// Specifies an eligible type of person
		/// </summary>
		DemographicProfile dmgProfileSelector;

		/// <summary>
		/// Will weight the chance that someone with these BucketDemographicProfile dmgProfile will be chosen.
		/// </summary>
		double weight;
	};

    SexualBehavior() : SexualBehavior(SexualPartnership::Type::Last) { }

	SexualBehavior(SexualPartnership::Type type) : partnershipType(type) { }

	void AddAvailableBucket(const AvailableBucket &bucket) { availableBuckets.push_back(bucket); }

	void SetHighRiskMultiplier(double multiplier);
	void ApplyCoefficientVariation(double coefficient);

	/// <summary>
	/// Returns number of Buckets that are available for this kind of partnership.
	/// This is used to allow class Male to set aside some temporary space
	/// </summary>
	unsigned int getNumAvailableBuckets() const;

	const LogNormalDist getAcquisitionRatePerMonth(Person::RiskLevel risk) const;

	const AvailableBucket getAvailableBucket(int _bucket) const;

	double getCoitalEventsPerMonth(Person::RiskLevel risk) const;

	const BetaDist getChanceCondomUsePerEvent(Person::RiskLevel risk) const;

	const NormalDist getAverageYearsYounger() const;

	const ShiftedLogNormalDist getPartnershipDurationMth(Person::RiskLevel risk) const;

	SexualPartnership::Type getPartnershipType() const;

	void setChanceCondomUsePerEvent(Person::RiskLevel risk, BetaDist dist) { chanceCondomUsePerEvent[risk] = dist; }

	void setCoitalEventsPerMonth(Person::RiskLevel risk, double meanEvents) { coitalEventsPerMonth[risk] = meanEvents; }

	void setPartnershipDuration(Person::RiskLevel risk, ShiftedLogNormalDist dist) { partnershipDurationMth[risk] = dist; }

	void setAverageYearsYounger(NormalDist dist) { averageYearsYounger = dist; }

	void setAcquisitionRatePerMonth(Person::RiskLevel risk, LogNormalDist dist) { acquisitionRatePerMonth[risk] = dist; }

    double getAssortativeness() const { return assortativeness; }

    void setAssortativeness(double assortativeness) { this->assortativeness = assortativeness; }

private:
	friend class SimulationBuilder;

    double assortativeness;

	//the partnership type that these parameters represent
	SexualPartnership::Type partnershipType;

	//LogNormal distribution from which the people draw a rate to acquire this type of partner
	LogNormalDist acquisitionRatePerMonth[Person::ENDRiskLevel];
	//average number of partners men acquire at a time
	//double averagePartnersAtATime[Person::ENDRiskLevel];

	//selection criteria
	//particular buckets that are available for this kind of sexual partnership
	std::vector<AvailableBucket> availableBuckets;

	//The distribution the males will draw from to determine how many years younger their partner should be (resulting difference may be negative for older women)
	NormalDist averageYearsYounger;

	//avg events per month across all Couples; will be used as a mean in Poisson distribution
	double coitalEventsPerMonth[Person::ENDRiskLevel];

	//chance per event that this person will use a condom
	BetaDist chanceCondomUsePerEvent[Person::ENDRiskLevel];

	//avg duration if partnerships across all Couples
	ShiftedLogNormalDist partnershipDurationMth[Person::ENDRiskLevel];
};
