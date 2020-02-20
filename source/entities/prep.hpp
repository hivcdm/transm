#pragma once

#include <vector>

#include "entity.hpp"
#include "demographicprofile.hpp"
#include "utility/time.hpp"

namespace transm {

/// <summary>
/// Contains the specified parameters for pre-Exposure Prophylaxis
/// </summary>
/// <remarks>
/// </remarks>

struct PrepEligibility
{
    int currentPartnerCount = -1;
    HIVStatus partnerStatus;
    RiskLevel partnerRiskLevel;
    Time monthsSinceUnprotectedAct;
};

class PrepParameters
{
    bool enabled = false;

    PrepEligibility eligibility;
    double access = 0.0;
    double adherence = 0.0;
    double retention = 0.0;
    double returnToCare = 0.0;

    double efficacy;

    using ProfileMap = std::map<DemographicProfile, double>;
    ProfileMap accessProfiles;
    ProfileMap adherenceProfiles;
    ProfileMap retentionProfiles;
    ProfileMap returnToCareProfiles;

public:
    bool Enabled() { return enabled; }
    void SetEnabled(bool value) { enabled = value; }

    void SetEligibility(PrepEligibility elig) { eligibility = elig; }
    PrepEligibility GetEligibility() const { return eligibility; }

    void SetEfficacy(double value) { efficacy = value; }
    double GetEfficacy() const { return efficacy; }

    void SetDefaultAccess(double value) { access = value; }
    void SetProfileAccess(DemographicProfile profile, double value)
    {
        accessProfiles.emplace(profile, value);
    }
    double GetAccess(DemographicProfile profile) const
    {
        double value = access;
        for (auto pair : accessProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultAdherence(double value) { adherence = value; }
    void SetProfileAdherence(DemographicProfile profile, double value)
    {
        adherenceProfiles.emplace(profile, value);
    }
    double GetAdherence(DemographicProfile profile) const
    {
        double value = adherence;
        for (auto pair : adherenceProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultRetention(double value) { retention = value; }
    void SetProfileRetention(DemographicProfile profile, double value)
    {
        retentionProfiles.emplace(profile, value);
    }
    double GetRetention(DemographicProfile profile) const
    {
        double value = retention;
        for (auto pair : retentionProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultReturnToCare(double value) { returnToCare = value; }
    void SetProfileReturnToCare(DemographicProfile profile, double value)
    {
        returnToCareProfiles.emplace(profile, value);
    }
    double GetReturnToCare(DemographicProfile profile) const
    {
        double value = returnToCare;
        for (auto pair : returnToCareProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }
};

} // namespace transm
