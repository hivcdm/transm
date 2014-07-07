#pragma once

#include <fstream>

#include "../core/TargetGroup.h"
#include "../entities/Person.h"

class InterventionOutcomes
{
public:
    void RegisterGroup(const TargetGroup &group)
    {
        if(!rows_.empty())
        {
            throw std::runtime_error("can't register new groups after simulation has begun");
        }

        registered_groups_.push_back(&group);
    }

    void Update(int current_month)
    {
        std::vector<int> current_row;

        for(auto group : registered_groups_)
        {
            for(auto partition : group->GetPartitionNames())
            {
                auto summary = group->GetPartitionSummary(partition, false);

                current_row.push_back(summary.population_size);
                current_row.push_back(summary.incident_cases);
                current_row.push_back(summary.prevalent_cases);
            }
        }

        rows_.push_back(current_row);

        if(current_month + 1 != (int)rows_.size())
        {
            throw std::runtime_error("missed month");
        }
    }

    void Write(const std::string &filename) const
    {
        std::fstream file;
        file.open(filename, std::ios::out);

        for(auto group : registered_groups_)
        {
            file << "\t" << group->GetLabel();

            for(std::size_t i = 0; i < group->GetPartitionNames().size() * 3; i++)
            {
                file << "\t";
            }
        }

        file << std::endl;
        file << "\t";

        for(auto group : registered_groups_)
        {
            for(auto partition : group->GetPartitionNames())
            {
                file << partition;

                for(int i = 0; i < 2; i++)
                {
                    file << "\t";
                }
            }
        }

        file << std::endl;
        file << "Month";

        for(auto group : registered_groups_)
        {
            for(auto partition : group->GetPartitionNames())
            {
                file << "\tPopulation Size\tIncident Cases\tPrevalent Cases";
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
    std::vector<std::vector<int>> rows_;
    std::vector<const TargetGroup *> registered_groups_;
};
