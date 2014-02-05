#include <utility>
#include <vector>

#include "Bucket.h"
#include "../entities/Person.h"

class LifeMonthsBucket : public Bucket
{
public:
	LifeMonthsBucket(const Person &person, const std::vector<std::pair<int, int> > &ageGroups)
	{
        values_.push_back(person.getHIVStatus());
	}
};