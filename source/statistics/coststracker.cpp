#include "coststracker.hpp"

namespace transm {

CostsTracker::CostsTracker()
{
	Reset();
}

CostsTracker::~CostsTracker()
{
}

void CostsTracker::PrintCosts(Time time, std::ostream &_outStream)
{
	if(time == Time::Zero)
	{
		BuildHeader();
		PrintHeader(_outStream);
	}

	BuildRow(time);
	PrintRow(_outStream);
	Reset();
}

void CostsTracker::RecordLifeMonth(double qualityOfLife, double discountFactor, Entity::HIVStatus status)
{
    undiscounted_.lifeMonthsByHivStatus[(std::size_t)status]++;
    undiscounted_.qalmsByHivStatus[(std::size_t)status] += qualityOfLife;

    discounted_.lifeMonthsByHivStatus[(std::size_t)status] += discountFactor;
    discounted_.qalmsByHivStatus[(std::size_t)status] += qualityOfLife * discountFactor;
}

void CostsTracker::RecordCondomUse(double costUndiscounted, double costDiscounted)
{
	undiscounted_.condomCosts += costUndiscounted;
	discounted_.condomCosts += costDiscounted;
}

void CostsTracker::RecordCircumcision(double costUndiscounted, double costDiscounted)
{
	undiscounted_.circumcisionCosts += costUndiscounted;
	discounted_.circumcisionCosts += costDiscounted;
}

void CostsTracker::RecordVaginalMicrobicideCost(double costUndiscounted, double costDiscounted)
{
    undiscounted_.vaginalMicrobicideCosts += costUndiscounted;
    discounted_.vaginalMicrobicideCosts += costDiscounted;
}

void CostsTracker::RecordPrEPCost(double costUndiscounted, double costDiscounted)
{
    undiscounted_.prEPCosts += costUndiscounted;
    discounted_.prEPCosts += costDiscounted;
}

void CostsTracker::RecordCepacCosts(double costUndiscounted, double costDiscounted, 
    const std::string &entityType, Entity::CD4Strata cd4,
	Entity::HVLStrata hvl, Entity::HIVStatus status)
{
    undiscounted_.totalCostsByHivState[(std::size_t)status] += costUndiscounted;
    discounted_.totalCostsByHivState[(std::size_t)status] += costDiscounted;

    undiscounted_.totalCostsByEntityType[entityType] += costUndiscounted;
    discounted_.totalCostsByEntityType[entityType] += costDiscounted;

	if(status != Entity::HIVStatus::NEGATIVE)
	{
        undiscounted_.totalCostsByCd4[(std::size_t)cd4] += costUndiscounted;
        undiscounted_.totalCostsByHvl[(std::size_t)hvl] += costUndiscounted;

        discounted_.totalCostsByCd4[(std::size_t)cd4] += costDiscounted;
        discounted_.totalCostsByHvl[(std::size_t)hvl] += costDiscounted;
	}
}

void CostsTracker::RecordTreatmentCosts(const std::array<double, 3> &costsUndiscounted, const std::array<double, 3> &costsDiscounted, 
	int artLine)
{
	undiscounted_.artCosts[artLine] += costsUndiscounted[0];
	undiscounted_.drugCosts += costsUndiscounted[1];
	undiscounted_.toxicityCosts += costsUndiscounted[2];

	discounted_.artCosts[artLine] += costsDiscounted[0];
	discounted_.drugCosts += costsDiscounted[1];
	discounted_.toxicityCosts += costsDiscounted[2];
}

void CostsTracker::RecordClinicalCosts(const std::array<double, 5> &costsUndiscounted, const std::array<double, 5> &costsDiscounted)
{
	for(int i = 0; i < 5; i++)
	{
		undiscounted_.clinicalCosts[i] += costsUndiscounted[i];
		discounted_.clinicalCosts[i] += costsDiscounted[i];
	}
}

void CostsTracker::RecordMedicalCosts(const std::array<double, 4> &costsUndiscounted, const std::array<double, 4> &costsDiscounted)
{
	for(int i = 0; i < 4; i++)
	{
		undiscounted_.medicalCosts[i] += costsUndiscounted[i];
		discounted_.medicalCosts[i] += costsDiscounted[i];
	}
}

void CostsTracker::BuildHeader()
{
	SetHeaderCell(1, 1, "Cost Effectiveness");
	SetHeaderCell(1, 3, "Time");

	int column = 2;

	for(auto section_header : {"Undiscounted", "Discounted"})
	{
		SetHeaderCell(column, 1, section_header);
		SetHeaderCell(column, 2, "Life Months");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column, 3, "HIV Status");

		for(auto status_header : {"Negative", "Acute (Observed)", "Acute (Unobserved)", "Chronic (Observed)",
			"Chronic (Unobserved)", "Late-Stage (Observed)", "Late-Stage (Unobserved)"})
		{
			SetHeaderCell(column++, 4, status_header);
		}

		SetHeaderCell(column, 2, "QALMs");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column, 3, "HIV Status");

		for(auto status_header : {"Negative", "Acute (Observed)", "Acute (Unobserved)", "Chronic (Observed)",
			"Chronic (Unobserved)", "Late-Stage (Observed)", "Late-Stage (Unobserved)"})
		{
			SetHeaderCell(column++, 4, status_header);
		}

		SetHeaderCell(column, 2, "Costs");
		SetHeaderCell(column++, 4, "Overall");
		SetHeaderCell(column, 2, "CDM Costs");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column++, 4, "Circumcision");
		SetHeaderCell(column++, 4, "Condoms");
        SetHeaderCell(column++, 4, "PrEP");
        SetHeaderCell(column++, 4, "Vaginal Microbicides");

		SetHeaderCell(column, 2, "CEPAC Costs");

		SetHeaderCell(column++, 4, "Total");

		SetHeaderCell(column, 3, "Medical");
		SetHeaderCell(column++, 4, "Direct Medical");
		SetHeaderCell(column++, 4, "Direct Non-Medical");
		SetHeaderCell(column++, 4, "Time");
		SetHeaderCell(column++, 4, "Indirect");

		SetHeaderCell(column, 3, "Clinical");
		SetHeaderCell(column++, 4, "CD4 Testing");
		SetHeaderCell(column++, 4, "HVL Testing");
		SetHeaderCell(column++, 4, "Clinic Visits");
		SetHeaderCell(column++, 4, "HIV Screening Tests");
		SetHeaderCell(column++, 4, "HIV Screening Misc");

		SetHeaderCell(column, 3, "ART");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column++, 4, "ART1");
		SetHeaderCell(column++, 4, "ART2");
		SetHeaderCell(column++, 4, "ART3");
		SetHeaderCell(column++, 4, "ART4");
		SetHeaderCell(column++, 4, "Drugs");
		SetHeaderCell(column++, 4, "Toxicity");

		SetHeaderCell(column, 3, "Gender");
        SetHeaderCell(column++, 4, "Male");
		SetHeaderCell(column++, 4, "Male:Msw");
		SetHeaderCell(column++, 4, "Male:Msmw");
        SetHeaderCell(column++, 4, "Male:Msm");
        SetHeaderCell(column++, 4, "Female");

		SetHeaderCell(column, 3, "HIV Status");

		for(auto status_header : {"Negative", "Acute (Observed)", "Acute (Unobserved)", "Chronic (Observed)",
			"Chronic (Unobserved)", "Late-Stage (Observed)", "Late-Stage (Unobserved)"})
		{
			SetHeaderCell(column++, 4, status_header);
		}

		SetHeaderCell(column, 3, "CD4");
		SetHeaderCell(column++, 4, "Very Low");
		SetHeaderCell(column++, 4, "Low");
		SetHeaderCell(column++, 4, "Medium Low");
		SetHeaderCell(column++, 4, "Medium High");
		SetHeaderCell(column++, 4, "High");
		SetHeaderCell(column++, 4, "Very High");

		SetHeaderCell(column, 3, "HVL");
		SetHeaderCell(column++, 4, "Uninfected");
		SetHeaderCell(column++, 4, "Very Low");
		SetHeaderCell(column++, 4, "Low");
		SetHeaderCell(column++, 4, "Medium Low");
		SetHeaderCell(column++, 4, "Medium");
		SetHeaderCell(column++, 4, "Medium High");
		SetHeaderCell(column++, 4, "High");
		SetHeaderCell(column++, 4, "Very High");
		SetHeaderCell(column++, 4, "Primary");
		SetHeaderCell(column++, 4, "Late-Stage");
	}
}

void CostsTracker::BuildRow(Time time)
{
	if(time == Time::Zero)
	{
		PushElement("init");
	}
	else
	{
		PushElement((int)time.in_months());
	}

	for(const auto &costs : {undiscounted_, discounted_})
	{
		double totalLifeMonths = 0;
		double totalQalms = 0;
		double cepacTotalCost = 0;

		for(std::size_t i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			totalLifeMonths += costs.lifeMonthsByHivStatus[i];
			totalQalms += costs.qalmsByHivStatus[i];
		}

		for(std::size_t i = 0; i < (std::size_t)Entity::CD4Strata::Last; i++)
		{
			cepacTotalCost += costs.totalCostsByCd4[i];
		}

		PushElement(totalLifeMonths);

		for(std::size_t i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			PushElement(costs.lifeMonthsByHivStatus[i]);
		}

		PushElement(totalQalms);

		for(std::size_t i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			PushElement(costs.qalmsByHivStatus[i]);
		}

		double cdmTotalCost = costs.circumcisionCosts + costs.condomCosts + costs.prEPCosts + costs.vaginalMicrobicideCosts;

		PushElement(cdmTotalCost + cepacTotalCost);

		PushElement(cdmTotalCost);
		PushElement(costs.circumcisionCosts);
		PushElement(costs.condomCosts);
        PushElement(costs.prEPCosts);
        PushElement(costs.vaginalMicrobicideCosts);

		PushElement(cepacTotalCost);
		for(int i = 0; i < SimContext::COST_NUM_TYPES; i++)
		{
			PushElement(costs.medicalCosts[i]);
		}

		for(int i = 0; i < (int)ClinicalCostTypes::Last; i++)
		{
			PushElement(costs.clinicalCosts[i]);
		}

		double artTotalCost = costs.toxicityCosts;
		for(int i = 0; i < NumArtLinesToRecord; i++)
		{
			artTotalCost += costs.artCosts[i];
		}

		PushElement(artTotalCost);

		for(int i = 0; i < NumArtLinesToRecord; i++)
		{
			PushElement(costs.artCosts[i]);
		}

		PushElement(costs.drugCosts);
		PushElement(costs.toxicityCosts);

        double sum_male_costs = 0;

        for(auto entity_type : {"msw", "msmw", "msm"})
        {
            if(costs.totalCostsByEntityType.find(entity_type) != costs.totalCostsByEntityType.end())
            {
                sum_male_costs += costs.totalCostsByEntityType.at(entity_type);
            }
        }

        PushElement(sum_male_costs);

        for(auto entity_type : {"msw", "msmw", "msm", "female"})
		{
            if(costs.totalCostsByEntityType.find(entity_type) != costs.totalCostsByEntityType.end())
            {
                PushElement(costs.totalCostsByEntityType.at(entity_type));
            }
            else
            {
                PushElement(0);
            }
		}

		for(std::size_t i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			PushElement(costs.totalCostsByHivState[i]);
		}

		for(std::size_t i = 0; i < (std::size_t)Entity::CD4Strata::Last; i++)
		{
			PushElement(costs.totalCostsByCd4[i]);
		}

		for(std::size_t i = 0; i < (std::size_t)Entity::HVLStrata::Last; i++)
		{
			PushElement(costs.totalCostsByHvl[i]);
		}
	}
}

void CostsTracker::Reset()
{
    for(int i = 0; i < 2; i++)
    {
        auto &costs = (i == 0) ? discounted_ : undiscounted_;
        costs.artCosts.fill(0);
        costs.circumcisionCosts = 0;
        costs.clinicalCosts.fill(0);
        costs.condomCosts = 0;
        costs.drugCosts = 0;
        costs.lifeMonthsByHivStatus.fill(0);
        costs.medicalCosts.fill(0);
        costs.qalmsByHivStatus.fill(0);
        costs.totalCostsByCd4.fill(0);
        costs.totalCostsByEntityType.clear();
        costs.totalCostsByHivState.fill(0);
        costs.totalCostsByHvl.fill(0);
        costs.toxicityCosts = 0;
    }
}

} // namespace transm
