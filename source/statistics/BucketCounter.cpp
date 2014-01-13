#include "BucketCounter.h"

static int sqliteCallback(void *result, int argc, char **argv, char ** /*columnName*/)
{
	int &resultInt = *reinterpret_cast<int *>(result);
	resultInt = 0;

	if(argc == 1)
	{
		if(argv[0])
		{
			std::stringstream ss(argv[0]);
			ss >> resultInt;
		}
	}

	return 0;
}

int BucketCounter::GetCount(DmgProfile::SexualActivityStatus sexualActivityStatus,
                            DmgProfile::Gender gender,
                            DmgProfile::SexualOrientation sexualOrientation,
                            DmgProfile::RelationshipStatus relationshipStatus,
                            DmgProfile::Employment employment,
                            Person::RiskLevel riskLevel,
                            int ageGroup,
                            Person::CD4Strata cd4Stratum)
{
	std::stringstream query;
	query << "SELECT SUM(count) FROM Bucket";
	BuildWhere(sexualActivityStatus, gender, sexualOrientation, relationshipStatus, employment, riskLevel, ageGroup,
	           cd4Stratum, query);
	query << ";";
	int sum = 0;
	bool success = ExecuteQuery(query.str(), sum);

	if(success)
	{
		return sum;
	}

	return -1;
}

void BucketCounter::Increment(Person *person)
{
	int count = GetCount(person);

	if(count == 0)
	{
		InsertBucket(person);
	}

	std::stringstream query;
	query << "UPDATE Bucket SET count=";
	query << count + 1;
	BuildWhere(person, query);
	ExecuteQuery(query.str());
}

void BucketCounter::SetAgeRanges(const std::vector<std::pair<int, int>> &ageRanges)
{
	this->ageRanges.assign(ageRanges.begin(), ageRanges.end());
}

int BucketCounter::GetCount(Person *person)
{
	std::stringstream query;
	query << "SELECT SUM(count) FROM Bucket";
	BuildWhere(person, query);
	query << ";";
	int count = 0;
	bool success = ExecuteQuery(query.str(), count);

	if(success)
	{
		return count;
	}

	return -1;
}

void BucketCounter::BuildWhere(Person *person, std::stringstream &query)
{
	const DmgProfile *demographicProfile = person->getDmgProfile();
	DmgProfile::SexualActivityStatus sexualActivityStatus = static_cast<DmgProfile::SexualActivityStatus>
	        (demographicProfile->get(DmgProfile::SEXUAL_ACTIVITY_STATUS));
	DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(demographicProfile->get(DmgProfile::GENDER));
	DmgProfile::SexualOrientation sexualOrientation = static_cast<DmgProfile::SexualOrientation>(demographicProfile->get(
	            DmgProfile::SEXUAL_ORIENTATION));
	DmgProfile::RelationshipStatus relationshipStatus = static_cast<DmgProfile::RelationshipStatus>(demographicProfile->get(
	            DmgProfile::RELATIONSHIP_STATUS));
	DmgProfile::Employment employment = static_cast<DmgProfile::Employment>(demographicProfile->get(
	                                        DmgProfile::EMPLOYMENT));
	Person::RiskLevel riskLevel = person->getRiskLevel();
	int ageGroup = -1;
	int age = person->getAge(MONTH);

	for(size_t i = 0; i < ageRanges.size(); ++i)
	{
		if(age >= ageRanges[i].first && age <= ageRanges[i].second)
		{
			ageGroup = i;
		}
	}

	assert(ageGroup != -1);
	Person::CD4Strata cd4Stratum = person->getCd4Stratum();
	return BuildWhere(sexualActivityStatus, gender, sexualOrientation, relationshipStatus, employment, riskLevel, ageGroup,
	                  cd4Stratum, query);
}

void BucketCounter::BuildWhere(DmgProfile::SexualActivityStatus sexualActivityStatus,
                               DmgProfile::Gender gender,
                               DmgProfile::SexualOrientation sexualOrientation,
                               DmgProfile::RelationshipStatus relationshipStatus,
                               DmgProfile::Employment employment,
                               Person::RiskLevel riskLevel,
                               int ageGroup,
                               Person::CD4Strata cd4Stratum,
                               std::stringstream &query)
{
	if((sexualActivityStatus != DmgProfile::ENDSexualActivityStatus) || (gender != DmgProfile::ENDGender) ||
	        (sexualOrientation != DmgProfile::ENDSexualOrientation) || (relationshipStatus != DmgProfile::ENDRelationshipStatus) ||
	        (employment != DmgProfile::ENDEmployment) || (riskLevel != Person::ENDRiskLevel) || (ageGroup != -1)
	        || (cd4Stratum != Person::ENDCD4Strata))
	{
		query << " WHERE ";
		bool first = true;

		if(sexualActivityStatus != DmgProfile::ENDSexualActivityStatus)
		{
			query << "sexualActivityStatus=" << sexualActivityStatus;
			first = false;
		}

		if(gender != DmgProfile::ENDGender)
		{
			if(!first)
			{
				query << " AND ";
			}

			query << "gender=" << gender;
			first = false;
		}

		if(sexualOrientation != DmgProfile::ENDSexualOrientation)
		{
			if(!first)
			{
				query << " AND ";
			}

			query << "sexualOrientation=" << sexualOrientation;
			first = false;
		}

		if(relationshipStatus != DmgProfile::ENDRelationshipStatus)
		{
			if(!first)
			{
				query << " AND ";
			}

			query << "relationshipStatus=" << relationshipStatus;
			first = false;
		}

		if(employment != DmgProfile::ENDEmployment)
		{
			if(!first)
			{
				query << " AND ";
			}

			query << "employment=" << employment;
			first = false;
		}

		if(riskLevel != Person::ENDRiskLevel)
		{
			if(!first)
			{
				query << " AND ";
			}

			query << "riskLevel=" << riskLevel;
			first = false;
		}

		if(ageGroup != -1)
		{
			if(!first)
			{
				query << " AND ";
			}

			query << "ageGroup=" << ageGroup;
			first = false;
		}

		if(cd4Stratum != Person::ENDCD4Strata)
		{
			if(!first)
			{
				query << " AND ";
			}

			query << "cd4Stratum=" << cd4Stratum;
			first = false;
		}
	}
}

void BucketCounter::CreateTable()
{
	std::string query = "CREATE TABLE Bucket (\
							idBucket INTEGER NOT NULL,\
							sexualActivityStatus INTEGER NOT NULL,\
							gender INTEGER NOT NULL,\
							sexualOrientation INTEGER NOT NULL,\
							relationshipStatus INTEGER NOT NULL,\
							employment INTEGER NOT NULL,\
							riskLevel INTEGER NOT NULL,\
							ageGroup INTEGER NOT NULL,\
							cd4Stratum INTEGER NOT NULL,\
							count INTEGER NOT NULL,\
							UNIQUE(\
							    sexualActivityStatus,\
								gender,\
								sexualOrientation,\
								relationshipStatus,\
								employment,\
								riskLevel,\
								ageGroup,\
								cd4Stratum) ON CONFLICT REPLACE,\
							PRIMARY KEY(idBucket ASC));";
	ExecuteQuery(query);
}

void BucketCounter::OpenConnection()
{
	connected = !sqlite3_open(":memory:", &db);
}

void BucketCounter::CloseConnection()
{
	if(connected)
	{
		sqlite3_close(db);
		connected = false;
	}
}

bool BucketCounter::InsertBucket(Person *person)
{
	const DmgProfile *demographicProfile = person->getDmgProfile();
	DmgProfile::SexualActivityStatus sexualActivityStatus = static_cast<DmgProfile::SexualActivityStatus>
	        (demographicProfile->get(DmgProfile::SEXUAL_ACTIVITY_STATUS));
	DmgProfile::Gender gender = static_cast<DmgProfile::Gender>(demographicProfile->get(DmgProfile::GENDER));
	DmgProfile::SexualOrientation sexualOrientation = static_cast<DmgProfile::SexualOrientation>(demographicProfile->get(
	            DmgProfile::SEXUAL_ORIENTATION));
	DmgProfile::RelationshipStatus relationshipStatus = static_cast<DmgProfile::RelationshipStatus>(demographicProfile->get(
	            DmgProfile::RELATIONSHIP_STATUS));
	DmgProfile::Employment employment = static_cast<DmgProfile::Employment>(demographicProfile->get(
	                                        DmgProfile::EMPLOYMENT));
	Person::RiskLevel riskLevel = person->getRiskLevel();
	int ageGroup = -1;
	int age = person->getAge(MONTH);

	for(size_t i = 0; i < ageRanges.size(); ++i)
	{
		if(age >= ageRanges[i].first && age <= ageRanges[i].second)
		{
			ageGroup = i;
		}
	}

	assert(ageGroup != -1);
	Person::CD4Strata cd4Stratum = person->getCd4Stratum();
	std::stringstream query;
	query << "INSERT INTO Bucket";
	query << " (idBucket, sexualActivityStatus, gender, sexualOrientation, relationshipStatus, employment, riskLevel, ageGroup, cd4Stratum, count)";
	query << " VALUES (NULL, ";
	query << sexualActivityStatus << ",";
	query << gender << ",";
	query << sexualOrientation << ",";
	query << relationshipStatus << ",";
	query << employment << ",";
	query << riskLevel << ",";
	query << ageGroup << ",";
	query << cd4Stratum << ",";
	query << 0 << ");";
	return ExecuteQuery(query.str());
}

bool BucketCounter::BucketExists(Person *person)
{
	std::stringstream query;
	query << "SELECT COUNT(*) FROM Bucket";
	BuildWhere(person, query);
	query << ";";
	int count = 0;
	ExecuteQuery(query.str(), count);
	return count > 0;
}

void BucketCounter::Reset()
{
	std::string query = "DELETE FROM Bucket";
	ExecuteQuery(query);
}

bool BucketCounter::ExecuteQuery(const std::string &query)
{
	char *errorMessage = NULL;
	sqlite3_exec(db, query.c_str(), NULL, 0, &errorMessage);

	if(errorMessage != NULL)
	{
		sqlite3_free(errorMessage);
		return false;
	}

	return true;
}

bool BucketCounter::ExecuteQuery(const std::string &query, int &result)
{
	char *errorMessage = NULL;
	sqlite3_exec(db, query.c_str(), sqliteCallback, &result, &errorMessage);

	if(errorMessage != NULL)
	{
		sqlite3_free(errorMessage);
		return false;
	}

	return true;
}
