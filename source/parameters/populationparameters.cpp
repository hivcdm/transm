#include "core/population.hpp"
#include "entities/female.hpp"
#include "entities/male.hpp"
#include "entities/entity.hpp"
#include "entities/sexualbehavior.hpp"
#include "utility/utility.hpp"

namespace transm {

PopulationParameters::PopulationParameters()
{
	//set default values of fields
	initSize = 10000;
	birthRate = 0.0038;
	useBirthRate = true;
	ageOfMajority = Age::from_months(180);
	chronicInfectionRate = 0.00;
}

PopulationParameters::~PopulationParameters()
{
}

void PopulationParameters::SetAgeRanges()
{
	ageRanges.clear();
    for(auto ageBucket : GetInitialAgeBuckets())
    {
		AgeRange ageRange;
		ageRange.lower = ageBucket.GetMinAge();
		ageRange.upper = ageBucket.GetMaxAge();
        ageRanges.push_back(ageRange);
    }
}

double PopulationParameters::GetBirthProportion(DemographicProfile profile)
{
	auto iter = std::find_if(birthProportions.begin(), birthProportions.end(),
		[&](const DemographicProfile::DoublePair pair)
		{ return pair.first == profile; }
		);
	if (iter == birthProportions.end())
		throw std::runtime_error("birth proportion not found for profile: " + *profile.toString());

	return iter->second;
}


} // namespace transm
