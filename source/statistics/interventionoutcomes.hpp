#pragma once

#include <fstream>

#include "core/targetgroup.hpp"
#include "entities/entity.hpp"

namespace transm {

class InterventionOutcomes
{
public:
    InterventionOutcomes() : group_container_(nullptr)
    {
    }

    void RegisterGroupContainer(const std::vector<TargetGroup> &group)
    {
        if(!rows_.empty())
        {
            throw std::runtime_error("can't register new groups after simulation has begun");
        }

        group_container_ = &group;
    }

    void Update(EventParams &parameters)
    {
        std::vector<double> current_row;

        for(auto &group : *group_container_)
        {
            for(auto partition : group.GetPartitionNames())
            {
                auto summary = group.GetPartitionSummary(partition, parameters);

                current_row.push_back((int)summary.population_size);
                current_row.push_back((int)summary.population_size_na);
                current_row.push_back((int)summary.population_size_sa);
                current_row.push_back((int)summary.pop_size_na_male);

                for(auto &age_range : summary.sa_size_by_age_range_male)
                {
                    current_row.push_back((int)std::get<2>(age_range));
                }

                for(auto entity_type : {"male", "msmw", "msm", "female"})
                {
                    current_row.push_back((int)summary.pop_size_na_entity_type[entity_type]);
                    for(auto &age_range : summary.sa_size_by_age_range_entity_type[entity_type])
                    {
                        current_row.push_back((int)std::get<2>(age_range));
                    }
                }

                for(auto &risk_group : summary.size_risk_group)
                {
                    current_row.push_back((int)risk_group.second);
                }

                current_row.push_back((int)summary.incident_cases);
                current_row.push_back((int)summary.incident_male);
                for(auto &age_range : summary.incident_by_age_range_male)
                {
                    current_row.push_back((int)std::get<2>(age_range));
                }

                for(auto entity_type : {"male", "msmw", "msm", "female"})
                {
                    current_row.push_back((int)summary.incident_entity_type[entity_type]);
                    for(auto &age_range : summary.incident_by_age_range_entity_type[entity_type])
                    {
                        current_row.push_back((int)std::get<2>(age_range));
                    }
                }

                for(auto &risk_group : summary.incident_risk_group)
                {
                    current_row.push_back((int)risk_group.second);
                }

                current_row.push_back((int)summary.prevalent_cases);
                current_row.push_back((int)summary.prevalent_male);
                for(auto &age_range : summary.prevalent_by_age_range_male)
                {
                    current_row.push_back((int)std::get<2>(age_range));
                }

                for(auto entity_type : {"male", "msmw", "msm", "female"})
                {
                    current_row.push_back((int)summary.prevalent_entity_type[entity_type]);
                    for(auto &age_range : summary.prevalent_by_age_range_entity_type[entity_type])
                    {
                        current_row.push_back((int)std::get<2>(age_range));
                    }
                }

                for(auto &risk_group : summary.prevalent_risk_group)
                {
                    current_row.push_back((int)risk_group.second);
                }

				current_row.push_back(summary.life_months_undiscounted);
				current_row.push_back(summary.cdm_costs_undiscounted);
				current_row.push_back(summary.cepac_costs_undiscounted);
				current_row.push_back(summary.life_months_discounted);
				current_row.push_back(summary.cdm_costs_discounted);
				current_row.push_back(summary.cepac_costs_discounted);
            }
        }

        rows_.push_back(current_row);

        if(parameters.currTime + 1 != (int)rows_.size())
        {
            throw std::runtime_error("missed month");
        }
    }

    void Write(const std::string &filename) const
    {
        if(group_container_ == nullptr) return;

        std::fstream file;
        file.open(filename, std::ios::out);

        for(auto &group : *group_container_)
        {
            file << "\t" << group.GetLabel();

            for(std::size_t i = 0; i < group.GetPartitionNames().size() * 90; i++)
            {
                file << "\t";
            }
        }

        file << std::endl;
        file << "\t";

        for(auto &group : *group_container_)
        {
            for(auto partition : group.GetPartitionNames())
            {
                file << partition;

                for(int i = 0; i < 89; i++)
                {
                    file << "\t";
                }
            }
        }

        file << std::endl;
        file << "\t";

        for(auto &group : *group_container_)
        {
            for(auto partition : group.GetPartitionNames())
            {
				file << "Population Sizes			Male Population Sizes										Female Population Sizes																Incident Cases																											Prevalent Cases																											Costs/LMs						";
            }
        }

        file << std::endl;
        file << "\t";

        for(auto &group : *group_container_)
        {
            for(auto partition : group.GetPartitionNames())
            {
				file << "			Non-Sexually Active Population	Sexually Active Population									Non-Sexually Active Population	Sexually Active Population									Risk Group							Male Incident Cases										Female Incident Cases										Risk Group							Male Prevalent Cases										Female Prevalent Cases										Risk Group						Undiscounted			Discounted			";
            }
        }

        file << std::endl;
        file << "Month";

        for(auto &group : *group_container_)
        {
            for(auto partition : group.GetPartitionNames())
            {
                file << "	Total	NA Population Size	SA Population Size	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	CSW High Risk	CSW Low Risk	Non-CSW High Risk Male	Non-CSW High Risk Msmw	Non-CSW High Risk Msm	Non-CSW High Risk Female	Non-CSW Low Risk Male	Non-CSW Low Risk Msmw	Non-CSW Low Risk Msm	Non-CSW Low Risk Female	Total	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	CSW High Risk	CSW Low Risk	Non-CSW High Risk Male	Non-CSW High Risk Msmw	Non-CSW High Risk Msm	Non-CSW High Risk Female	Non-CSW Low Risk Male	Non-CSW Low Risk Msmw	Non-CSW Low Risk Msm	Non-CSW Low Risk Female	Total	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	All ages	0-203	204-239	240-299	300-359	360-419	420-479	480-539	540-599	600-1211	CSW High Risk	CSW Low Risk	Non-CSW High Risk Male	Non-CSW High Risk Msmw	Non-CSW High Risk Msm	Non-CSW High Risk Female	Non-CSW Low Risk Male	Non-CSW Low Risk Msmw	Non-CSW Low Risk Msm	Non-CSW Low Risk Female";
				file << "\t" << "Life Months" << "\t" << "CDM" << "\t" << "CEPAC";
				file << "\t" << "Life Months" << "\t" << "CDM" << "\t" << "CEPAC";
            }
        }

        file << std::endl;
        file << "init";
        int month = 0;

        for(auto row : rows_)
        {
            if(month != 0)
            {
                file << month;
            }

            for(auto column : row)
            {
                file << "\t" << column;
            }

            file << std::endl;
            month++;
        }
    }

private:
    std::vector<std::vector<double>> rows_;
    const std::vector<TargetGroup> *group_container_;
};

} // namespace transm
