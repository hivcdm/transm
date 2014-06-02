#include <iostream>
#include <numeric>
#include <set>
#include <boost/filesystem.hpp>
#include <include.h>

#include "Simulation.h"
#include "Constants.h"
#include "Population.h"
#include "../util/CepacInputParser.h"
#include "../data/EventParams.h"
#include "../entities/classifiers/DemographicProfile.h"
#include "../entities/behaviors/SexualBehavior.h"
#include "../util/HighResolutionTimer.h"
#include "../util/Utility.h"

TargetGroup::PopulationTarget TargetGroup::PopulationTarget::Any;

void TargetGroup::Partition::Update(Population &p, int current_time)
{
    for(auto intervention : interventions_)
    {
        if(intervention.IsActive(current_time))
        {
            for(auto person : members_)
            {
                intervention.Apply(p, person);
            }
        }
    }
}

void Intervention::Apply(Simulation &simulation)
{
    if(simulation_intervention_)
    {
        simulation_intervention_(simulation);
    }
}

void Intervention::Apply(Population &population)
{
    if(population_intervention_)
    {
        population_intervention_(population);
    }
}

void Intervention::Apply(Population &population, Person *person)
{
    if(population_individual_intervention_)
    {
        population_individual_intervention_(population, person);
    }
}

void Intervention::Apply(Person *person)
{
    if(individual_intervention_)
    {
        individual_intervention_(person);
    }
}

bool Intervention::IsActive(int current_time) const
{
    if(duration_ == -1)
    {
        return current_time >= time_;
    }

    return current_time >= time_ && current_time <= time_ + duration_;
}

bool Intervention::IsFirstMonth(int current_time) const
{
    return current_time == time_;
}

bool Intervention::IsCompleted(int current_time) const
{
    return current_time > time_ + duration_;
}

void TargetGroup::Update(Population &p, int current_time, RandomNumberGenerator &rng, const std::unordered_set<Person *> &newly_added, const std::unordered_set<Person *> &dead_people)
{
    if(enrollment_period_.start > current_time)
    {
        return;
    }

    for(auto person : dead_people)
    {
        for(auto &partition : partitions_)
        {
            partition.Remove(person);
        }
    }

    if(open_ || enrollment_period_.start == current_time)
    {
        auto match = [&](Person *person)
        {
            return (!((target_.value.employment.has_value
                && target_.value.employment.value != (DemographicProfile::Employment)person->getDemographicProfileVal(DemographicProfile::EMPLOYMENT))
                || (target_.value.gender.has_value
                && target_.value.gender.value != (DemographicProfile::Gender)person->getDemographicProfileVal(DemographicProfile::GENDER))
                || (target_.value.relationship_status.has_value
                && target_.value.relationship_status.value != (DemographicProfile::RelationshipStatus)person->getDemographicProfileVal(DemographicProfile::RELATIONSHIP_STATUS))
                || (target_.value.sexual_activity_status.has_value
                && target_.value.sexual_activity_status.value != (DemographicProfile::SexualActivityStatus)person->getDemographicProfileVal(DemographicProfile::SEXUAL_ACTIVITY_STATUS))
                || (target_.value.sexual_orientation.has_value
                && target_.value.sexual_orientation.value != (DemographicProfile::SexualOrientation)person->getDemographicProfileVal(DemographicProfile::SEXUAL_ORIENTATION))
                || (target_.value.age_lower.has_value
                && target_.value.age_lower.value < person->getAge(TimeGranularity::Month))
                || (target_.value.age_upper.has_value
                && target_.value.age_upper.value > person->getAge(TimeGranularity::Month))
                || (target_.value.observed_hiv_status.has_value
                && target_.value.observed_hiv_status.value != person->getHIVStatus())
                || (target_.value.on_treatment.has_value
                && target_.value.on_treatment.value != person->isOnArt())
                || (target_.value.risk_level.has_value
                && target_.value.risk_level.value != person->getRiskLevel())));
        };

        int num_matches = (int)std::count_if(newly_added.begin(), newly_added.end(), match);

        std::vector<std::pair<int, int>> partition_allocations;
        int num_allocated = 0;

        for(int i = 0; i < (int)partitions_.size() - 1; i++)
        {
            int partition_allocation = static_cast<int>(partitions_[i].GetProportion() * num_matches);
            if(partition_allocation == 0)
            {
                continue;
            }
            partition_allocations.push_back(std::make_pair(i, partition_allocation));
            num_allocated += partition_allocation;
        }

        partition_allocations.push_back(std::make_pair((int)partitions_.size() - 1, num_matches - num_allocated));

        auto person_iter = std::find_if(newly_added.begin(), newly_added.end(), match);
        while(!partition_allocations.empty())
        {
            int allocations_index = rng.randInt(0, (uint32_t)partition_allocations.size() - 1);
            int partition_index = partition_allocations[allocations_index].first;
            partitions_[partition_index].Add(*person_iter);
            if(--partition_allocations[allocations_index].second == 0)
            {
                partition_allocations.erase(partition_allocations.begin() + allocations_index);
            }
            person_iter++;
            person_iter = std::find_if(person_iter, newly_added.end(), match);
        }
    }

    for(auto &partition : partitions_)
    {
        partition.Update(p, current_time);
    }
}

std::vector<std::string> split_string(const std::string &string, char delim)
{
    std::vector<std::string> split;
    std::stringstream ss(string);
    std::string part;
    while(std::getline(ss, part, delim))
    {
        split.push_back(part);
    }
    return split;
}

enum class KnownIntervention
{
    Circumcise,
    BirthRate,
    ProportionMale,
    ProportionCircumcised,
    ChanceBecomeSexWorker,
    AgeSexualDebut,
    TransmissionCoefficient,
    ProportionHighRisk,
    AverageYearsYounger,
    PartnerAcquisitionRate,
    CoitalEventsPerMonth,
    ChanceCondomUse,
    PartnershipDuration,
    RolloutEligibility
};

const std::map<KnownIntervention, std::string> KnownInterventionStrings = 
{
    {KnownIntervention::Circumcise, "circumcise"},
    {KnownIntervention::BirthRate, "birthRate"},
    {KnownIntervention::ProportionMale, "proportionMale"},
    {KnownIntervention::ProportionCircumcised, "proportionCircumcised"},
    {KnownIntervention::ChanceBecomeSexWorker, "chanceBecomeSexWorker"},
    {KnownIntervention::AgeSexualDebut, "ageSexualDebut"},
    {KnownIntervention::TransmissionCoefficient, "transmissionCoefficient"},
    {KnownIntervention::ProportionHighRisk, "proportionHighRisk"},
    {KnownIntervention::AverageYearsYounger, "averageYearsYounger"},
    {KnownIntervention::PartnerAcquisitionRate, "partnerAcquisitionRate"},
    {KnownIntervention::CoitalEventsPerMonth, "coitalEventsPerMonth"},
    {KnownIntervention::ChanceCondomUse, "chanceCondomUse"},
    {KnownIntervention::PartnershipDuration, "partnershipDuration"},
    {KnownIntervention::RolloutEligibility, "rolloutEligibility"}
};

template<typename T>
T from_string(const std::string &string);

template<>
KnownIntervention from_string(const std::string &intervention)
{
    for(auto pair : KnownInterventionStrings)
    {
        if(pair.second == intervention)
        {
            return pair.first;
        }
    }

    throw std::runtime_error("unknown intervention: " + intervention);
}

template<>
Person::RiskLevel from_string(const std::string &risk)
{
    if(risk == "high") return Person::RiskLevel::HIGH;
    if(risk == "low") return Person::RiskLevel::LOW;

    throw std::runtime_error("unknown risk level: " + risk);
}

template<>
DemographicProfile::Gender from_string(const std::string &gender)
{
    if(gender == "male") return DemographicProfile::Gender::MALE;
    if(gender == "female") return DemographicProfile::Gender::FEMALE;

    throw std::runtime_error("unknown gender: " + gender);
}

template<>
DemographicProfile::Employment from_string(const std::string &employment)
{
    if(employment == "csw") return DemographicProfile::Employment::CSW;
    if(employment == "non-csw") return DemographicProfile::Employment::NON_CSW;

    throw std::runtime_error("unknown employment: " + employment);
}

template<>
Person::HVLStrata from_string(const std::string &hvl_string)
{
    if(hvl_string == "-1" || hvl_string == "uninfected") return Person::HVLStrata::UNINFECTED;
    if(hvl_string == "0") return Person::HVLStrata::HVL_ZERO;
    if(hvl_string == "1") return Person::HVLStrata::HVL_ONE;
    if(hvl_string == "2") return Person::HVLStrata::HVL_TWO;
    if(hvl_string == "3") return Person::HVLStrata::HVL_THREE;
    if(hvl_string == "4") return Person::HVLStrata::HVL_FOUR;
    if(hvl_string == "5") return Person::HVLStrata::HVL_FIVE;
    if(hvl_string == "6") return Person::HVLStrata::HVL_SIX;
    if(hvl_string == "7" || hvl_string == "primary") return Person::HVLStrata::HVL_PRIMARY;
    if(hvl_string == "8" || hvl_string == "late-stage") return Person::HVLStrata::HVL_LATESTAGE;

    throw std::runtime_error("unknown hvl stratum: " + hvl_string);
}

template<>
SexualPartnership::Type from_string(const std::string &type_string)
{
    if(type_string == "steady") return SexualPartnership::Type::Steady;
    if(type_string == "regular") return SexualPartnership::Type::Regular;
    if(type_string == "casual") return SexualPartnership::Type::Casual;
    if(type_string == "csw") return SexualPartnership::Type::Csw;

    throw std::runtime_error("unknown partnership type: " + type_string);
}

template<>
NormalDist from_string(const std::string &type_string)
{
    NormalDist dist;

    auto comma_index = type_string.find(',');

    dist.mean = std::stod(type_string.substr(0, comma_index));
    dist.stddev = std::stod(type_string.substr(comma_index + 1));

    return dist;
}

template<>
BetaDist from_string(const std::string &type_string)
{
    return BetaDist::FromNormal(from_string<NormalDist>(type_string));
}

template<>
LogNormalDist from_string(const std::string &type_string)
{
    return LogNormalDist::FromNormal(from_string<NormalDist>(type_string));
}

template<>
ShiftedLogNormalDist from_string(const std::string &type_string)
{
    std::size_t last_comma_index = 0;

    for(std::size_t i = 0; i < type_string.size(); i++)
    {
        if(type_string[i] == ',')
        {
            last_comma_index = i;
        }
    }

    auto shift = std::stod(type_string.substr(last_comma_index + 1));
    auto dist = from_string<NormalDist>(type_string.substr(0, last_comma_index));

    return ShiftedLogNormalDist::FromShiftedNormal(dist, shift);
}

Intervention::Intervention(const std::string &parameter, const std::string &value, 
    const std::unordered_map<std::string, std::string> &parameters, bool individual)
    : duration_(-1)
{
    try
    {
        time_ = std::stoi(parameters.at("time"));
    }
    catch(std::out_of_range)
    {
        throw std::runtime_error("must have a defined time for this intervention");
    }

    auto intervention_type = from_string<KnownIntervention>(parameter);

    if(individual)
    {
        switch(intervention_type)
        {
            case KnownIntervention::Circumcise:
            {
                population_individual_intervention_ = [=](Population &population, Person *person) { population.Circumcise(person); };
                break;
            }
            case KnownIntervention::ChanceBecomeSexWorker:
            {
                auto chance = std::stod(value);
                individual_intervention_ = [=](Person *person) { person->SetChanceBecomeSexWorker(chance); };
                break;
            }
            case KnownIntervention::AgeSexualDebut:
            {
                auto age_in_years = std::stoi(value);
                individual_intervention_ = [=](Person *person) { person->SetAgeSexualDebut(age_in_years, TimeGranularity::Year); };
                break;
            }
            case KnownIntervention::TransmissionCoefficient:
            {
                auto hvl_stratum = from_string<Person::HVLStrata>(parameters.at("hvl"));
                auto coefficient = std::stod(value);
                individual_intervention_ = [=](Person *person) { person->SetTransmissionCoefficient(hvl_stratum, coefficient); };
                break;
            }
            case KnownIntervention::AverageYearsYounger:
            {
                auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
                auto dist = from_string<NormalDist>(value);
                individual_intervention_ = [=](Person *person) { person->SetAverageYearsYounger(partnership_type, dist); };
                break;
            }
            case KnownIntervention::PartnerAcquisitionRate:
            {
                auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
                auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
                auto dist = from_string<LogNormalDist>(value);
                individual_intervention_ = [=](Person *person) { person->SetAcquisitionRatePerMonth(risk, partnership_type, dist); };
                break;
            }
            case KnownIntervention::CoitalEventsPerMonth:
            {
                auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
                auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
                auto dist = std::stod(value);
                individual_intervention_ = [=](Person *person) { person->SetCoitalEventsPerMonth(risk, partnership_type, dist); };
                break;
            }
            case KnownIntervention::ChanceCondomUse:
            {
                auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
                auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
                auto dist = from_string<BetaDist>(value);
                individual_intervention_ = [=](Person *person) { person->SetChanceCondomUsePerEvent(risk, partnership_type, dist); };
                break;
            }
            case KnownIntervention::PartnershipDuration:
            {
                auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
                auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
                auto dist = from_string<ShiftedLogNormalDist>(value);
                individual_intervention_ = [=](Person *person) { person->SetPartnershipDuration(risk, partnership_type, dist); };
                break;
            }
            default:
            {
                std::string message = "Intervention cannot be applied to a specific sub-population: ";
                message.append(KnownInterventionStrings.at(intervention_type));
                throw std::runtime_error(message);
            }
        }
    }
    else
    {
        switch(intervention_type)
        {
        case KnownIntervention::BirthRate:
        {
            population_intervention_ = [=](Population &p) { p.popWideParams.setBirthRate(std::stod(value)); };
            break;
        }
        case KnownIntervention::ProportionMale:
        {
            population_intervention_ = [=](Population &p) { p.popWideParams.setProportionMale(std::stod(value)); };
            break;
        }
        case KnownIntervention::ProportionCircumcised:
        {
            population_intervention_ = [=](Population &p) { p.popWideParams.setProportionCircumcised(std::stod(value)); };
            break;
        }
        case KnownIntervention::ChanceBecomeSexWorker:
        {
            auto gender = from_string<DemographicProfile::Gender>(parameters.at("gender"));
            auto chance = std::stod(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetChanceBecomeCsw(gender, chance); };
            individual_intervention_ = [=](Person *person) { person->SetChanceBecomeSexWorker(chance); };
            break;
        }
        case KnownIntervention::AgeSexualDebut:
        {
            auto age_in_years = std::stoi(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.setAgeSexualDebut(age_in_years, TimeGranularity::Year); };
            individual_intervention_ = [=](Person *person) { person->SetAgeSexualDebut(age_in_years, TimeGranularity::Year); };
            break;
        }
        case KnownIntervention::TransmissionCoefficient:
        {
            auto gender = from_string<DemographicProfile::Gender>(parameters.at("gender"));
            auto hvl_stratum = from_string<Person::HVLStrata>(parameters.at("hvl"));
            auto coefficient = std::stod(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetTransmissionCoefficient(gender, hvl_stratum, coefficient); };
            individual_intervention_ = [=](Person *person) 
            { 
                if(person->getDemographicProfileVal(DemographicProfile::GENDER) == gender)
                {
                    person->SetTransmissionCoefficient(hvl_stratum, coefficient);
                }
            };
            break;
        }
        case KnownIntervention::ProportionHighRisk:
        {
            auto gender = from_string<DemographicProfile::Gender>(parameters.at("gender"));
            auto employment = from_string<DemographicProfile::Employment>(parameters.at("employment"));
            auto proportion = std::stod(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetProportionHighRisk(gender, employment, proportion); };
            break;
        }
        case KnownIntervention::AverageYearsYounger:
        {
            auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
            auto dist = from_string<NormalDist>(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetAverageYearsYounger(partnership_type, dist); };
            individual_intervention_ = [=](Person *person) { person->SetAverageYearsYounger(partnership_type, dist); };
            break;
        }
        case KnownIntervention::PartnerAcquisitionRate:
        {
            auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
            auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
            auto dist = from_string<LogNormalDist>(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetAcquisitionRatePerMonth(risk, partnership_type, dist); };
            individual_intervention_ = [=](Person *person) { person->SetAcquisitionRatePerMonth(risk, partnership_type, dist); };
            break;
        }
        case KnownIntervention::CoitalEventsPerMonth:
        {
            auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
            auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
            auto dist = std::stod(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetCoitalEventsPerMonth(risk, partnership_type, dist); };
            individual_intervention_ = [=](Person *person) { person->SetCoitalEventsPerMonth(risk, partnership_type, dist); };
            break;
        }
        case KnownIntervention::ChanceCondomUse:
        {
            auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
            auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
            auto dist = from_string<BetaDist>(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetChanceCondomUsePerEvent(risk, partnership_type, dist); };
            individual_intervention_ = [=](Person *person) { person->SetChanceCondomUsePerEvent(risk, partnership_type, dist); };
            break;
        }
        case KnownIntervention::PartnershipDuration:
        {
            auto risk = from_string<Person::RiskLevel>(parameters.at("risk"));
            auto partnership_type = from_string<SexualPartnership::Type>(parameters.at("type"));
            auto dist = from_string<ShiftedLogNormalDist>(value);
            population_intervention_ = [=](Population &p) { p.popWideParams.SetPartnershipDuration(risk, partnership_type, dist); };
            individual_intervention_ = [=](Person *person) { person->SetPartnershipDuration(risk, partnership_type, dist); };
            break;
        }
        case KnownIntervention::RolloutEligibility:
        {
            int new_value = std::stoi(value);
            std::string criterion = parameters.at("criterion");
            std::string parameter_name = parameters.at("parameter");

            if(criterion == "oi-hist")
            {
                if(parameter_name == "rank")
                {
                    simulation_intervention_ = [=](Simulation &s) { s.parameters_.rolloutEligibility.oiHistRank = new_value; };
                }
                else if(parameter_name.substr(0, 2) == "oi")
                {
                    int oi_number = std::stoi(parameter_name.substr(2));
                    simulation_intervention_ = [=](Simulation &s) { s.parameters_.rolloutEligibility.oiHistOIs[oi_number] = new_value != 0; };
                }
            }
            else if(criterion == "cd4")
            {
                if(parameter_name == "rank")
                {
                    simulation_intervention_ = [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4Rank = new_value; };
                }
                else if(parameter_name == "lower")
                {
                    simulation_intervention_ = [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4Bounds.lower = new_value; };
                }
                else if(parameter_name == "upper")
                {
                    simulation_intervention_ = [=](Simulation &s) { s.parameters_.rolloutEligibility.cd4Bounds.upper = new_value; };
                }
            }

            break;
        }
        default:
        {
            std::string message = "Intervention cannot be applied to population: ";
            message.append(KnownInterventionStrings.at(intervention_type));
            throw std::runtime_error(message);
        }
        }
    }
}

TargetGroup::TargetGroup(int start, int end, bool open, bool permanent, Nullable<PopulationTarget> target)
: enrollment_period_({start, end}),
open_(open),
permanent_effect_(permanent),
target_(target)
{

}

void TargetGroup::AddPartition(const std::string &label, bool trace, double proportion,
    std::vector<Intervention> interventions)
{
    Partition p;
    p.label_ = label;
    p.trace_ = trace;
    p.proportion_ = proportion;
    p.interventions_ = interventions;
    partitions_.push_back(p);
}

TargetGroup::PopulationTarget TargetGroup::PopulationTarget::FromString(const std::string &s)
{
    TargetGroup::PopulationTarget target;

	std::stringstream ss(s);
	int i = 0;
	while(ss)
	{
		std::string line;
		std::getline(ss, line, ':');
		switch(i++)
		{
		case 0:
			target.risk_level.has_value = line != "*";
			if(target.risk_level.has_value)
			{
				target.risk_level.value = static_cast<Person::RiskLevel>(std::stoi(line));
			}
			break;
		case 1:
			target.employment.has_value = line != "*";
			if(target.employment.has_value)
			{
				target.employment.value = static_cast<DemographicProfile::Employment>(std::stoi(line));
			}
			break;
		case 2:
			target.sexual_activity_status.has_value = line != "*";
			if(target.sexual_activity_status.has_value)
			{
				target.sexual_activity_status.value = static_cast<DemographicProfile::SexualActivityStatus>(std::stoi(line));
			}
			break;
		case 3:
			target.gender.has_value = line != "*";
			if(target.gender.has_value)
			{
				target.gender.value = static_cast<DemographicProfile::Gender>(std::stoi(line));
			}
			break;
		case 4:
			target.relationship_status.has_value = line != "*";
			if(target.relationship_status.has_value)
			{
				target.relationship_status.value = static_cast<DemographicProfile::RelationshipStatus>(std::stoi(line));
			}
			break;
		case 5:
			target.sexual_orientation.has_value = line != "*";
			if(target.sexual_orientation.has_value)
			{
				target.sexual_orientation.value = static_cast<DemographicProfile::SexualOrientation>(std::stoi(line));
			}
			break;
		case 6:
			target.age_lower.has_value = line != "*";
			if(target.age_lower.has_value)
			{
				target.age_lower.value = std::stoi(line);
			}
			break;
		case 7:
			target.age_upper.has_value = line != "*";
			if(target.age_upper.has_value)
			{
				target.age_upper.value = std::stoi(line);
			}
			break;
		case 8:
			target.observed_hiv_status.has_value = line != "*";
			if(target.observed_hiv_status.has_value)
			{
				target.observed_hiv_status.value = static_cast<Person::HIVStatus>(std::stoi(line));
			}
			break;
		case 9:
			target.on_treatment.has_value = line != "*";
			if(target.on_treatment.has_value)
			{
				target.on_treatment.value = line[0] != 'f' && line[0] != 'n';
			}
			break;
		}
	}
	return target;
}

void Simulation::RegisterIntervention(const Intervention &intervention)
{
    interventions_.push_back(intervention);
}

Simulation::Simulation()
    : time_(0),
      parameters_(),
      population_(parameters_),
      failedCalibration_(false),
      hasPassedFirstMonthCalibPrev_(false),
      monthOfFirstMonthCalibPrev_(0),
      incidence_(0),
      prevalence_(0)
{
}

Simulation::~Simulation()
{
}

void Simulation::SetFixedSeed(int seed)
{
	fixedSeed_ = seed;

	CepacUtil::setRandomSeedType(seed == -1);

	if(seed > -1)
	{
		//Seed is Minnesota Twins retired numbers... yes, I am a dork
		parameters_.randomNums.reset(seed == 0 ? 36291434 : seed);
	}
}

void Simulation::FirstStep()
{
	//No longer creating a CEPAC trace file, but we still need to change over to the results folder before creating any other output files
	CepacUtil::changeDirectoryToResults();

	if(parameters_.calibrationInputs.useCalibration)
	{
		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			std::string fileName = parameters_.simName;
			fileName.append("-popState" + boost::lexical_cast<std::string>(i)+".pop");
			parameters_.popStateStream[i].open(fileName.c_str(), ios::out);
		}
	}

	for(auto batchstat : enum_iterator<BatchStatsVariables>())
	{
        auto filename = "batchstats-" + Constants::BatchStatFileName.at(batchstat) + ".out";
		parameters_.BatchStatsStream[batchstat].open(filename, ios::out | ios::app);
	}

	//output seed used for this run
	if(parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::Events] << "Seed = " << parameters_.randomNums.getSeed() << std::endl;
		parameters_.trace_files[EventParams::TraceFile::Type::Events] << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << "Sexually Active Population"
			<< Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB << Constants::TAB <<
			"Non Sexually Active Population" << std::endl;
	}

    if(parameters_.monthOf1990 > 0 && parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].enabled)
	{
		population_.populationStatistics.enableShiftedOutcomes(parameters_.monthOf1990);
	}

    UpdateInterventions(population_.GetNewPeopleThisMonth(), population_.GetDeadPeopleThisMonth());
    population_.new_people_this_month_.clear();

	//initialize/reset monthly stats
	population_.ResetMonthlyStats();
	//initialize incident infections by age
	population_.InitIncidentInfectionsByAge();

	if(parameters_.delayPrevalence == 0)
	{
		population_.ApplyIncidentPrevalence(parameters_);
	}

	//print out prevalent infection stats & headers for rest of infection stats
	population_.CalcPrevalentPopulation(0);

	//Print out run name for first column of BatchStats files (if streams are open)
    for(auto batchstat : enum_iterator<BatchStatsVariables>())
	{
		if(parameters_.BatchStatsStream[batchstat].is_open())
		{
			parameters_.BatchStatsStream[batchstat] << parameters_.simName << Constants::TAB;
		}
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled)
	{
        population_.populationStatistics.infectionsTracker.printInfections(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Infection].file, &population_);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Partnership].enabled)
	{
		population_.PrintPartnerships(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Partnership].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Clinical].enabled)
	{
        population_.PrintClinical(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Clinical].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Population].enabled)
	{
        population_.PrintPopulation(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Population].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].enabled)
	{
        population_.PrintARTRolloutOutcomes(parameters_, parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].enabled)
	{
        population_.populationStatistics.costsTracker.PrintCosts(parameters_.currTime, parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].file);
	}

	if(parameters_.debugLevel == DebugLevel::One)
	{
		population_.PrintMethodResults(parameters_, "--", "initialization", 0, "--", true);
	}
}

void Simulation::Step()
{
    time_++;

    double begin = timer_.GetTime();

    std::remove_if(interventions_.begin(), interventions_.end(), [=](const Intervention &i) { return i.IsCompleted(time_); });

    for(auto &intervention : interventions_)
    {
        if(intervention.IsActive(time_))
        {
            if(intervention.AffectsSimulation())
            {
                intervention.Apply(*this);
            }

            if(intervention.AffectsPopulation())
            {
                intervention.Apply(population_);
            }

            if(intervention.AffectsIndividual())
            {
                if(intervention.IsFirstMonth(time_))
                {
                    population_.entities->forEach([&](Person *p) { intervention.Apply(p); });
                }
                else
                {
                    for(auto person : population_.new_people_this_month_)
                    {
                        intervention.Apply(person);
                    }
                }
            }
        }
    }

	if(parameters_.useRollout)
	{
		population_.ApplyRolloutContext(parameters_, time_);
	}

    std::size_t totalSize = SimulateMonth();

	//print out new infection stats
	population_.CalcPrevalentPopulation(time_);

    if(parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled)
	{
        population_.populationStatistics.infectionsTracker.printInfections(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Infection].file, &population_);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Population].enabled)
	{
        population_.PrintPopulation(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Population].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Partnership].enabled)
	{
        population_.PrintPartnerships(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Partnership].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Clinical].enabled)
	{
        population_.PrintClinical(parameters_, time_, parameters_.trace_files[EventParams::TraceFile::Type::Clinical].file);
	}

	//For now, this must come after infectionsTracker.printInfections as it is what calculate prevalence
    if(parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].enabled)
	{
        population_.RecordShiftedOutcomes(parameters_, parameters_.trace_files[EventParams::TraceFile::Type::ShiftedOutcomes].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].enabled)
	{
        population_.PrintARTRolloutOutcomes(parameters_, parameters_.trace_files[EventParams::TraceFile::Type::ArtRollout].file);
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].enabled)
	{
        population_.populationStatistics.costsTracker.PrintCosts(parameters_.currTime, parameters_.trace_files[EventParams::TraceFile::Type::CostEffectiveness].file);
	}

	if(parameters_.calibrationInputs.useCalibration && parameters_.calibrationInputs.monthOfCalibration == time_)
	{
		//If this run doesn't pass the partnership calibration stop the run and discard specified trace files
		if(!population_.PassesPartnershipCalibration(parameters_))
		{
			return;
		}
	}

	if(parameters_.calibrationInputs.useCalibration)
	{
		if(!hasPassedFirstMonthCalibPrev_)
		{
            double SAPrev = population_.populationStatistics.infectionsTracker.getSAPrev(population_);

			if(SAPrev != -1 && SAPrev >= parameters_.calibrationInputs.thresholdPrevMult * parameters_.calibrationInputs.calendarPrevs[0])
			{
				hasPassedFirstMonthCalibPrev_ = true;
				monthOfFirstMonthCalibPrev_ = parameters_.currTime;
			}
		}

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			if(hasPassedFirstMonthCalibPrev_ && parameters_.currTime == monthOfFirstMonthCalibPrev_ + parameters_.calibrationInputs.saveStateTimePoints[i])
			{
				population_.SaveState(parameters_.popStateStream[i], parameters_.currTime);
			}
		}
	}

	population_.ResetMonthlyStats();

	double end = timer_.GetTime();
	std::ostringstream elapsedStringStream;
	elapsedStringStream.precision(3);
	elapsedStringStream << std::fixed << (end - begin);
	std::string elapsedString = elapsedStringStream.str();

	parameters_.displayOut("Timestep(");
	std::string timeString = boost::lexical_cast<std::string>(time_);
	parameters_.displayOut(timeString.c_str());
	parameters_.displayOut("): compute time elapsed = ");
	parameters_.displayOut(elapsedString.c_str());
	parameters_.displayOut(". size = ");
	parameters_.displayOut(boost::lexical_cast<std::string>(totalSize).c_str());
	parameters_.displayOut("\n");

    prevalence_ = population_.GetPopulationStatistics().infectionsTracker.getSAPrev(population_);
    incidence_ = population_.GetPopulationStatistics().infectionsTracker.getCurrTimeStepIncidentInfsTotal() / (double)population_.GetSize();
}

void Simulation::LastStep()
{
	//print survival statistics
    if(parameters_.trace_files[EventParams::TraceFile::Type::Survival].enabled)
	{
        population_.populationStatistics.printSurvivalStats(parameters_.trace_files[EventParams::TraceFile::Type::Survival].file);
	}

	//Run every infected person left through CEPAC until they die
	if(failedCalibration_)
	{
		parameters_.displayOut("Partnership Calibration Failed...Deleting specified trace files...\n");
	}
	else
	{
		parameters_.displayOut("Running all remaining persons through CEPAC until they die...\n");
		population_.UpdateFinalPhysicalState(parameters_);
		parameters_.displayOut("Done!\n");
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::Infection].enabled)
	{
        population_.populationStatistics.printLMStats(parameters_.trace_files[EventParams::TraceFile::Type::Infection].file);
	}

	//finalize and print CEPAC output, but only if at least one patient went through CEPAC
	try
	{
		if(parameters_.cepacRunStats->getPopulationSummary()->numCohorts > 0)
		{
			parameters_.cepacRunStats->finalizeStats();
			parameters_.cepacRunStats->writeStatsFile();
		}
	}
	catch(std::string errorString)
	{
		parameters_.displayOut(errorString);
	}

	//if failed partnership calibration toss unneeded files
	if(failedCalibration_)
	{
        for(auto &trace_file : parameters_.trace_files)
        {
            if(trace_file.second.toss)
            {
                trace_file.second.file.close();
                std::string fileName = parameters_.simName;
                fileName.append("-" + trace_file.second.extension);
                remove(fileName.c_str());
            }
        }

		for(int i = 0; i < Constants::NUMBER_TIME_POINTS_SAVE_STATE; i++)
		{
			std::string fileName = parameters_.simName;
			fileName.append("-popState" + boost::lexical_cast<std::string>(i)+".pop");
			remove(fileName.c_str());
		}
	}
}

/*
* This function records all of the SimContext files to be used by the CEPAC disease model throughout the run of the transmission model
*/

bool Simulation::LoadCepacSimContexts(const CepacTreatmentFiles &treatment_files)
{
	parameters_.displayOut("CEPAC Files: \n");

	for(const auto &treatment_file : treatment_files)
	{
		parameters_.displayOut("\t" + std::to_string(treatment_file.file_number) + ": " 
			+ treatment_file.file_name + "\n");

		//Set the CEPAC simContext from the specified CEPAC .in file
		auto stem = boost::filesystem::path(treatment_file.file_name).stem().string();
		parameters_.cepacSimContexts.push_back(new SimContext(stem));
		assert(parameters_.cepacSimContexts.size() == static_cast<size_t>(treatment_file.file_number) + 1);
		//parameters_.cepacSimContext = new SimContext(cepacInputFile.substr(0, cepacInputFile.find(CepacUtility::FILE_EXTENSION_FOR_INPUT)));
		parameters_.cepacSimContexts.at(treatment_file.file_number)->numPatientsToTrace = 0;

		//Read in the inputs
		try
		{
			parameters_.cepacSimContexts.back()->readInputs();
		}
		catch(std::string errorString)
		{
			//if we can't find it and we wanted to use CEPAC, display error
			parameters_.displayOut("*****************************************\n");
			parameters_.displayOut("WARNING!\n");
			parameters_.displayOut("*****************************************\n");
			parameters_.displayOut("File '" + treatment_file.file_name + "' generates error:\n");
			parameters_.displayOut("\t" + errorString + "\n");
			return false;
		}

		//From the first file only, get the death tables for non-AIDS death
		if(treatment_file.file_number == 0)
		{
			CepacInputParser cepacInput(treatment_file.file_name);
			auto probabilities = cepacInput.parseNonAidsDeathProbabilities();
			Person::probDeathNatCauses[DemographicProfile::MALE] = probabilities[0];
			Person::probDeathNatCauses[DemographicProfile::FEMALE] = probabilities[1];
		}
	}

	for(int i = 0; i < Constants::NUMBER_OF_CEPAC_FILES; i++)
	{
		//By default, the first "time to switch" should be 0 (i.e. the first CEPAC .in file applies at time 0)
		parameters_.timesToSwitchSimContext[i] = 
			i == 0 ? 0 : cepac_treatment_files_[i].time;
	}

	return true;
}

/***
Sets the Non aids death from a cepac simcontext
***/
void Simulation::SetNonAidsDeathFromCepac(SimContext &cepacSimContext, std::vector<double> &male, std::vector<double> &female)
{
	male.clear();
	female.clear();

	for(int i = 0; i <= SimContext::AGE_YRS; i++)
	{
		male.push_back(cepacSimContext.getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_MALE][i]);
		female.push_back(cepacSimContext.getNatHistInputs()->monthlyNonAIDSDeathProb[SimContext::GENDER_FEMALE][i]);
	}
}

void Simulation::UpdateInterventions(const std::unordered_set<Person *> &new_people, const std::unordered_set<Person *> &dead_people)
{
    for(auto &group : groups_)
    {
        group.Update(population_, time_, parameters_.randomNums, new_people, dead_people);
    }
}

void Simulation::RegisterTargetGroup(const TargetGroup &group)
{
    groups_.push_back(group);
}

/***
This function executes one timestep of the simulation
The ordering of events within this function determines the ordering of events in each timestep
****/
std::size_t Simulation::SimulateMonth()
{
	parameters_.currTime = time_;

	//change non AIDS death if it is time to switch cepac files
	if(parameters_.itIsTimeToSwitchSimContext() && !parameters_.useRollout)
	{
        int simIndex = 0;

        for(std::size_t i = 0; i < parameters_.cepacSimContexts.size(); i++)
        {
            if(parameters_.currTime > parameters_.timesToSwitchSimContext[i])
            {
                simIndex = static_cast<int>(i);
            }
        }

		SetNonAidsDeathFromCepac(*parameters_.cepacSimContexts[simIndex], Person::probDeathNatCauses[DemographicProfile::MALE], Person::probDeathNatCauses[DemographicProfile::FEMALE]);
	}

	//output the current timestep of the simulation
    if(parameters_.debugLevel > DebugLevel::One && parameters_.trace_files[EventParams::TraceFile::Type::Events].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::Events] << "T:" << time_ << " : Start of Timestep" << std::endl;
	}

    if(parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson].enabled)
	{
		parameters_.trace_files[EventParams::TraceFile::Type::SinglePerson] << std::endl << "** Time " << time_ << ": " << std::endl;
	}

	bool recordLE = false;
	bool recordPartAcq  =  false;
	bool firstMonthToRecord = false;
	bool lastMonthToRecord = false;

    if(population_.populationStatistics.isTimeToRecordLE(time_))
	{
		recordLE = true;
	}

    if(population_.populationStatistics.isTimeToRecordPartAcq(time_))
	{
		recordPartAcq = true;
	}

    if(population_.populationStatistics.isFirstMonthToRecordLE(time_))
	{
		firstMonthToRecord = true;
	}

    if(population_.populationStatistics.isTimeToPrintLE(time_))
	{
		lastMonthToRecord = true;
	}

	population_.UpdatePhysicalState(parameters_, recordLE, firstMonthToRecord);

    UpdateInterventions(population_.GetNewPeopleThisMonth(), population_.GetDeadPeopleThisMonth());

	if(parameters_.useRollout)
	{
		population_.ApplyARTRollout(parameters_);
	}

	population_.Births(parameters_);

	if(lastMonthToRecord)
	{
		population_.UpdateAgeBucketsLE();

        if(parameters_.trace_files[EventParams::TraceFile::Type::LifeExpectancy].enabled)
		{
            population_.populationStatistics.printLEStats(parameters_.trace_files[EventParams::TraceFile::Type::LifeExpectancy].file, time_);
		}

        delete population_.populationStatistics.selectedLEStats;
        population_.populationStatistics.selectedLEStats = nullptr;
	}

	//steadyCouple, flings, and dissolveSexualPartnerships
	population_.UpdatePartnerships(parameters_);

	if(recordPartAcq)
	{
        population_.populationStatistics.selectedPartAcqStats = new PopulationStatistics::SinglePartAcqStats();
		population_.RecordPartAcqFreq();

        if(parameters_.trace_files[EventParams::TraceFile::Type::PartnerAcquisition].enabled)
		{
            population_.populationStatistics.printPartAcqStats(parameters_.trace_files[EventParams::TraceFile::Type::PartnerAcquisition].file, time_);
		}

        delete population_.populationStatistics.selectedPartAcqStats;
        population_.populationStatistics.selectedPartAcqStats = nullptr;
	}

	//apply incident prevalence
	if(parameters_.delayPrevalence != 0 && parameters_.delayPrevalence == time_)
	{
		population_.ApplyIncidentPrevalence(parameters_);
	}

	//Will confirm that population_.currSize is correct and update size of age ranges
	return population_.UpdateSize();
}

RunStats &Simulation::GetCEPACRunStats()
{
	return *parameters_.cepacRunStats;
}

PopulationStatistics &Simulation::GetPopulationStatistics()
{
    return population_.populationStatistics;
}

EventParams &Simulation::GetEventParams()
{
	return parameters_;
}

Outputs Simulation::Run(MessageCallback message_callback)
{
	MessageCallback old = parameters_.messageCallback;
	parameters_.messageCallback = message_callback;

	if(time_ == 0)
	{
		FirstStep();
	}

	while(time_ < duration_)
	{
		Step();
	}

	LastStep();

	parameters_.messageCallback = old;

	return outputs_;
}
