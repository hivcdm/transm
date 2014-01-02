#include <string>
#include <unordered_map>
#include <utility>

#include "../entities/Person.h"
#include "../util/sqlite/sqlite3.h"

static int sqliteCallback(void *result, int argc, char **argv, char **azColName);

class BucketCounter
{
public:
	BucketCounter() : db(NULL), connected(false), ageRanges(0), previousResult(0)
	{
		OpenConnection();
		CreateTable();
	}

	~BucketCounter() 
	{
		CloseConnection();
	}

	int GetCount(DmgProfile::SexualActivityStatus sexualActivityStatus, DmgProfile::Gender gender, DmgProfile::SexualOrientation sexualOrientation,
		DmgProfile::RelationshipStatus relationshipStatus, DmgProfile::Employment employment, Person::RiskLevel riskLevel, int ageGroup,
		Person::CD4Strata cd4Stratum);

	int GetCount()
	{
		return GetCount(DmgProfile::ENDSexualActivityStatus, DmgProfile::ENDGender, DmgProfile::ENDSexualOrientation, 
			DmgProfile::ENDRelationshipStatus, DmgProfile::ENDEmployment, Person::ENDRiskLevel, -1, Person::ENDCD4Strata);
	}

	int GetCountByGender(DmgProfile::Gender gender)
	{
		return GetCount(DmgProfile::ENDSexualActivityStatus, gender, DmgProfile::ENDSexualOrientation,
			DmgProfile::ENDRelationshipStatus, DmgProfile::ENDEmployment, Person::ENDRiskLevel, -1, Person::ENDCD4Strata);
	}

	int GetCountByGenderSexualActivity(DmgProfile::Gender gender, DmgProfile::SexualActivityStatus sexualActivityStatus)
	{
		return GetCount(sexualActivityStatus, gender, DmgProfile::ENDSexualOrientation,
			DmgProfile::ENDRelationshipStatus, DmgProfile::ENDEmployment, Person::ENDRiskLevel, -1, Person::ENDCD4Strata);
	}

	int GetCountByGenderSexualActivityAge(DmgProfile::Gender gender, DmgProfile::SexualActivityStatus sexualActivityStatus, int ageGroup)
	{
		return GetCount(sexualActivityStatus, gender, DmgProfile::ENDSexualOrientation,
			DmgProfile::ENDRelationshipStatus, DmgProfile::ENDEmployment, Person::ENDRiskLevel, ageGroup, Person::ENDCD4Strata);
	}

	int GetCountByCd4(Person::CD4Strata cd4Stratum)
	{
		return GetCount(DmgProfile::ENDSexualActivityStatus, DmgProfile::ENDGender, DmgProfile::ENDSexualOrientation,
			DmgProfile::ENDRelationshipStatus, DmgProfile::ENDEmployment, Person::ENDRiskLevel, -1, cd4Stratum);
	}

	int GetCountByRiskGroup(Person::RiskLevel riskLevel, DmgProfile::Gender gender, DmgProfile::Employment employment)
	{
		return GetCount(DmgProfile::ENDSexualActivityStatus, gender, DmgProfile::ENDSexualOrientation,
			DmgProfile::ENDRelationshipStatus, employment, riskLevel, -1, Person::ENDCD4Strata);
	}

	void Increment(Person *person);

	void Callback(int argc, char **argv, char **columnName);

	void SetAgeRanges(const std::vector<std::pair<int, int> > &ageRanges);

	void Reset();

private:
	int GetCount(Person *person);

	void BuildWhere(Person *person, std::stringstream &query);

	void BuildWhere(DmgProfile::SexualActivityStatus sexualActivityStatus, DmgProfile::Gender gender, DmgProfile::SexualOrientation sexualOrientation,
		DmgProfile::RelationshipStatus relationshipStatus, DmgProfile::Employment employment, Person::RiskLevel riskLevel, int ageGroup,
		Person::CD4Strata cd4Stratum, std::stringstream &query);

	void CreateTable();

	void OpenConnection();

	void CloseConnection();

	bool InsertBucket(Person *person);

	bool BucketExists(Person *person);

	bool ExecuteQuery(const std::string &query);

	bool ExecuteQuery(const std::string &query, int &result);

	sqlite3 *db;
	bool connected;
	std::vector<std::pair<int, int> > ageRanges;
	int previousResult;
};
