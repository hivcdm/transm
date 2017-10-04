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
	ageOfMajority = Age::from_months(180);
	birthProportions["hetero-male"] = 0.51;
	birthProportions["female"] = 1 - birthProportions["hetero-male"];
	proportionCircumcised = 0.20;
	chronicInfectionRate = 0.00;
}

PopulationParameters::~PopulationParameters()
{
}

double PopulationParameters::GetBirthRate() const
{
	return birthRate;
}

} // namespace transm
