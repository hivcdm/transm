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

    std::map<DemographicProfile, double> accessProfiles;
    std::map<DemographicProfile, double> adherenceProfiles;
    std::map<DemographicProfile, double> retentionProfiles;
    std::map<DemographicProfile, double> returnToCareProfiles;

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
        auto found = accessProfiles.find(profile);
        if (found != accessProfiles.end())
            found->second = value;
        else
            accessProfiles[profile] = value;
    }
    double GetAccess(DemographicProfile profile) const
    {
        double value = access;
        auto found = accessProfiles.find(profile);
        if (found != accessProfiles.end())
            value = found->second;

        return value;
    }

    void SetDefaultAdherence(double value) { adherence = value; }
    void SetProfileAdherence(DemographicProfile profile, double value)
    {
        auto found = adherenceProfiles.find(profile);
        if (found != adherenceProfiles.end())
            found->second = value;
    }
    double GetAdherence(DemographicProfile profile) const
    {
        double value = adherence;
        auto found = adherenceProfiles.find(profile);
        if (found != adherenceProfiles.end())
            value = found->second;

        return value;
    }

    void SetDefaultRetention(double value) { retention = value; }
    void SetProfileRetention(DemographicProfile profile, double value)
    {
        auto found = retentionProfiles.find(profile);
        if (found != retentionProfiles.end())
            found->second = value;
    }
    double GetRetention(DemographicProfile profile) const
    {
        double value = retention;
        auto found = retentionProfiles.find(profile);
        if (found != retentionProfiles.end())
            value = found->second;

        return value;
    }

    void SetDefaultReturnToCare(double value) { returnToCare = value; }
    void SetProfileReturnToCare(DemographicProfile profile, double value)
    {
        auto found = returnToCareProfiles.find(profile);
        if (found != returnToCareProfiles.end())
            found->second = value;
    }
    double GetReturnToCare(DemographicProfile profile) const
    {
        double value = returnToCare;
        auto found = returnToCareProfiles.find(profile);
        if (found != returnToCareProfiles.end())
            value = found->second;

        return value;
    }
};

} // namespace transm
