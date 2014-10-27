#include "coststracker.hpp"

namespace transm {

CostsTracker::CostsTracker()
{
	Reset();
}

CostsTracker::~CostsTracker()
{
}

void CostsTracker::PrintCosts(int time, std::ostream &_outStream)
{
	if(time == 0)
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

void CostsTracker::RecordCepacCosts(double costUndiscounted, double costDiscounted, 
    DemographicProfile::Gender gender, Entity::CD4Strata cd4, 
	Entity::HVLStrata hvl, Entity::HIVStatus status)
{
    undiscounted_.totalCostsByHivState[(std::size_t)status] += costUndiscounted;
    discounted_.totalCostsByHivState[(std::size_t)status] += costDiscounted;

    undiscounted_.totalCostsByGender[(std::size_t)gender] += costUndiscounted;
    discounted_.totalCostsByGender[(std::size_t)gender] += costDiscounted;

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

void CostsTracker::BuildRow(int time)
{
	if(time == 0)
	{
		PushElement("init");
	}
	else
	{
		PushElement(time);
	}

	for(const auto &costs : {undiscounted_, discounted_})
	{
		double totalLifeMonths = 0;
		double totalQalms = 0;
		double cepacTotalCost = 0;

		for(int i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			totalLifeMonths += costs.lifeMonthsByHivStatus[i];
			totalQalms += costs.qalmsByHivStatus[i];
		}

		for(int i = 0; i < (std::size_t)Entity::CD4Strata::Last; i++)
		{
			cepacTotalCost += costs.totalCostsByCd4[i];
		}

		PushElement(totalLifeMonths);

		for(int i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			PushElement(costs.lifeMonthsByHivStatus[i]);
		}

		PushElement(totalQalms);

		for(int i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			PushElement(costs.qalmsByHivStatus[i]);
		}

		double cdmTotalCost = costs.circumcisionCosts + costs.condomCosts;

		PushElement(cdmTotalCost + cepacTotalCost);

		PushElement(cdmTotalCost);
		PushElement(costs.circumcisionCosts);
		PushElement(costs.condomCosts);

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

		for(std::size_t i = 0; i < (std::size_t)DemographicProfile::Gender::Last; i++)
		{
			PushElement(costs.totalCostsByGender[i]);
		}

		for(int i = 0; i < (std::size_t)Entity::HIVStatus::Last; i++)
		{
			PushElement(costs.totalCostsByHivState[i]);
		}

		for(int i = 0; i < (std::size_t)Entity::CD4Strata::Last; i++)
		{
			PushElement(costs.totalCostsByCd4[i]);
		}

		for(int i = -1; i < (std::size_t)Entity::HVLStrata::Last; i++)
		{
			PushElement(costs.totalCostsByHvl[i + 1]);
		}
	}
}

void CostsTracker::Reset()
{
	discounted_ = Costs();
	undiscounted_ = Costs();
}

} // namespace transm
