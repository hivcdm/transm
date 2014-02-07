#include "CostsTracker.h"

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

void CostsTracker::RecordLifeMonth(const Person &person)
{
	undiscounted_.lifeMonthsByHivStatus[person.getHIVStatus()]++;
	undiscounted_.qalmsByHivStatus[person.getHIVStatus()] += person.getQualityOfLife();
}

void CostsTracker::RecordCondomUse(double cost)
{
	undiscounted_.condomCosts += cost;
}

void CostsTracker::RecordCircumcision(double cost)
{
	undiscounted_.circumcisionCosts += cost;
}

void CostsTracker::RecordCepacCosts(double costUndiscounted, double costDiscounted, const Person &person)
{
	undiscounted_.medicalCostsByGender[person.getDmgProfileVal(DmgProfile::GENDER)] += costUndiscounted;
	undiscounted_.medicalCostsByCd4[person.getCd4Stratum()] += costUndiscounted;
	undiscounted_.medicalCostsByHvl[person.getHVL()] += costUndiscounted;

	discounted_.medicalCostsByGender[person.getDmgProfileVal(DmgProfile::GENDER)] += costDiscounted;
	discounted_.medicalCostsByCd4[person.getCd4Stratum()] += costDiscounted;
	discounted_.medicalCostsByHvl[person.getHVL()] += costDiscounted;
}

void CostsTracker::RecordTreatmentCosts(const std::array<double, 3> &costsUndiscounted, int artLine, const Person &person)
{
	undiscounted_.artCosts[artLine] += costsUndiscounted[0];
	undiscounted_.drugCosts += costsUndiscounted[1];
	undiscounted_.toxicityCosts += costsUndiscounted[2];
}

void CostsTracker::RecordClinicalCosts(const std::array<double, 5> &costs, const Person &person)
{
	for(int i = 0; i < 5; i++)
	{
		undiscounted_.clinicalCosts[i] += costs[i];
	}
}

void CostsTracker::RecordMedicalCosts(const std::array<double, 4> &costs, const Person &person)
{
	for(int i = 0; i < 4; i++)
	{
		undiscounted_.medicalCosts[i] += costs[i];
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
		SetHeaderCell(column, 3, "Behavior");
		SetHeaderCell(column++, 4, "Circumcision");
		SetHeaderCell(column++, 4, "Condoms");
		SetHeaderCell(column, 2, "CEPAC Costs");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column, 3, "Medical");
		SetHeaderCell(column++, 4, "Direct Medical");
		SetHeaderCell(column++, 4, "Direct Non-Medical");
		SetHeaderCell(column++, 4, "Time");
		SetHeaderCell(column++, 4, "Indirect");
		SetHeaderCell(column, 3, "Gender");
		SetHeaderCell(column++, 4, "Male");
		SetHeaderCell(column++, 4, "Female");
		SetHeaderCell(column, 3, "Clinical");
		SetHeaderCell(column++, 4, "CD4 Testing");
		SetHeaderCell(column++, 4, "HVL Testing");
		SetHeaderCell(column++, 4, "Clinic Visits");
		SetHeaderCell(column++, 4, "HIV Screening Tests");
		SetHeaderCell(column++, 4, "HIV Screening Misc");
		SetHeaderCell(column, 3, "HIV State");
		SetHeaderCell(column++, 4, "HIV Positive");
		SetHeaderCell(column++, 4, "HIV Negative");
		SetHeaderCell(column++, 4, "HIV Positive Unidentified");
		SetHeaderCell(column++, 4, "HIV Positive Identified");
		SetHeaderCell(column, 3, "CD4");
		SetHeaderCell(column++, 4, "Very Low");
		SetHeaderCell(column++, 4, "Low");
		SetHeaderCell(column++, 4, "Medium Low");
		SetHeaderCell(column++, 4, "Medium High");
		SetHeaderCell(column++, 4, "High");
		SetHeaderCell(column++, 4, "Very High");
		SetHeaderCell(column, 3, "HVL");
		SetHeaderCell(column++, 4, "Very Low");
		SetHeaderCell(column++, 4, "Low");
		SetHeaderCell(column++, 4, "Medium Low");
		SetHeaderCell(column++, 4, "Medium");
		SetHeaderCell(column++, 4, "Medium High");
		SetHeaderCell(column++, 4, "High");
		SetHeaderCell(column++, 4, "Very High");
		SetHeaderCell(column, 3, "HVL Setpoint");
		SetHeaderCell(column++, 4, "Very Low");
		SetHeaderCell(column++, 4, "Low");
		SetHeaderCell(column++, 4, "Medium Low");
		SetHeaderCell(column++, 4, "Medium");
		SetHeaderCell(column++, 4, "Medium High");
		SetHeaderCell(column++, 4, "High");
		SetHeaderCell(column++, 4, "Very High");
		SetHeaderCell(column, 3, "ART");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column++, 4, "ART1");
		SetHeaderCell(column++, 4, "ART2");
		SetHeaderCell(column++, 4, "ART3");
		SetHeaderCell(column++, 4, "ART4");
		SetHeaderCell(column++, 4, "Drugs");
		SetHeaderCell(column++, 4, "Toxicity");
		SetHeaderCell(column, 3, "No OI History");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column, 3, "No OI History by CD4");
		SetHeaderCell(column++, 4, "Very Low");
		SetHeaderCell(column++, 4, "Low");
		SetHeaderCell(column++, 4, "Medium Low");
		SetHeaderCell(column++, 4, "Medium High");
		SetHeaderCell(column++, 4, "High");
		SetHeaderCell(column++, 4, "Very High");
		SetHeaderCell(column, 3, "With OI History");
		SetHeaderCell(column++, 4, "Total");
		SetHeaderCell(column, 3, "With OI History by CD4");
		SetHeaderCell(column++, 4, "Very Low");
		SetHeaderCell(column++, 4, "Low");
		SetHeaderCell(column++, 4, "Medium Low");
		SetHeaderCell(column++, 4, "Medium High");
		SetHeaderCell(column++, 4, "High");
		SetHeaderCell(column++, 4, "Very High");
		SetHeaderCell(column++, 4, "Proph");
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

	int column = 2;

	for(const auto &costs : {undiscounted_, discounted_})
	{
		int totalLifeMonths = 0;
		double totalQalms = 0;
		double cepacTotalCost = 0;

		for(int i = 0; i < Person::ENDHIVStatus; i++)
		{
			totalLifeMonths += costs.lifeMonthsByHivStatus[i];
			totalQalms += costs.qalmsByHivStatus[i];
		}

		for(int i = 0; i < Person::ENDCD4Strata; i++)
		{
			cepacTotalCost += costs.medicalCostsByCd4[i];
		}

		PushElement(totalLifeMonths);

		for(int i = 0; i < Person::ENDHIVStatus; i++)
		{
			PushElement(costs.lifeMonthsByHivStatus[i]);
		}

		PushElement(totalQalms);

		for(int i = 0; i < Person::ENDHIVStatus; i++)
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

		for(int i = 0; i < DmgProfile::ENDGender; i++)
		{
			PushElement(costs.medicalCostsByGender[i]);
		}

		for(int i = 0; i < (int)ClinicalCostTypes::Last; i++)
		{
			PushElement(costs.clinicalCosts[i]);
		}

		PushElement(0);// costs.hivPositiveCosts);
		for(int i = 0; i < SimContext::HIV_ID_NUM; i++)
		{
			PushElement(costs.medicalCostsByHivState[i]);
		}

		for(int i = 0; i < Person::ENDCD4Strata; i++)
		{
			PushElement(costs.medicalCostsByCd4[i]);
		}

		for(int i = 0; i < Person::ENDHIVStatus; i++)
		{
			PushElement(costs.medicalCostsByHvl[i]);
		}

		for(int i = 0; i < Person::ENDHIVStatus; i++)
		{
			PushElement(costs.medicalCostsByHvlSetpoint[i]);
		}

		double artTotalCost = 0;
		for(int i = 0; i < NumArtLinesToRecord; i++)
		{
			artTotalCost += costs.artCosts[i];
		}

		for(int i = 0; i < NumArtLinesToRecord; i++)
		{
			PushElement(costs.artCosts[i]);
		}

		PushElement(costs.drugCosts);
		PushElement(costs.toxicityCosts);

		double noOiHistTotal = 0;
		double oiHistTotal = 0;
		for(int i = 0; i < Person::ENDCD4Strata; i++)
		{
			noOiHistTotal += costs.medicalCostsByCd4NoOiHist[i];
			oiHistTotal += costs.medicalCostsByCd4WithOiHist[i];
		}

		PushElement(noOiHistTotal);
		for(int i = 0; i < Person::ENDCD4Strata; i++)
		{
			PushElement(costs.medicalCostsByCd4NoOiHist[i]);
		}

		PushElement(oiHistTotal);
		for(int i = 0; i < Person::ENDCD4Strata; i++)
		{
			PushElement(costs.medicalCostsByCd4WithOiHist[i]);
		}

		PushElement(0); //Proph
	}
}

void CostsTracker::Reset()
{
	discounted_ = Costs();
	undiscounted_ = Costs();
}
