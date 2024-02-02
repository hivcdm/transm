#ifndef PREP_HPP
#define PREP_HPP

#include <vector>

#include "entity.hpp"
#include "demographicprofile.hpp"
#include "utility/time.hpp"

namespace transm {

/** Contains the specified parameters for pre-Exposure Prophylaxis */
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
    std::array<double, 4> adherence = {0.0, 0.0, 0.0, 0.0};
    double retention = 0.0;
    double returnToCare = 0.0;

    double efficacy;

    using ProfileMap = std::map<DemographicProfile, double>;
    using AdherenceProfileMap = std::map<DemographicProfile, std::array<double, 4>>;
    ProfileMap accessProfiles;
    AdherenceProfileMap adherenceProfiles;
    ProfileMap retentionProfiles;
    ProfileMap returnToCareProfiles;

public:
    bool Enabled() const { return enabled; }
    void SetEnabled(bool value) { enabled = value; }

    void SetEligibility(PrepEligibility elig) { eligibility = elig; }
    PrepEligibility GetEligibility() const { return eligibility; }

    void SetEfficacy(double value) { efficacy = value; }
    double GetEfficacy() const {
        return efficacy;
    }

    void SetDefaultAccess(double value) { access = value; }
    void SetProfileAccess(const DemographicProfile& profile, double value)
    {
        accessProfiles.emplace(profile, value);
    }
    double GetAccess(const DemographicProfile& profile) const
    {
        double value = access;
        for (const auto& pair : accessProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultAdherence(array<double, 4> value) { adherence = value; }
    void SetProfileAdherence(const DemographicProfile& profile, std::array<double, 4> value)
    {
        adherenceProfiles.emplace(profile, value);
    }
    std::array<double, 4> GetAdherence(const DemographicProfile& profile) const
    {
        std::array<double, 4> value = adherence;
        for (const auto& pair : adherenceProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultRetention(double value) { retention = value; }
    void SetProfileRetention(const DemographicProfile& profile, double value)
    {
        retentionProfiles.emplace(profile, value);
    }
    double GetRetention(const DemographicProfile& profile) const
    {
        double value = retention;
        for (const auto& pair : retentionProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultReturnToCare(double value) { returnToCare = value; }
    void SetProfileReturnToCare(const DemographicProfile& profile, double value)
    {
        returnToCareProfiles.emplace(profile, value);
    }
    double GetReturnToCare(const DemographicProfile& profile) const
    {
        double value = returnToCare;
        for (const auto& pair : returnToCareProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }


};

} // namespace transm


#endif /* PREP_HPP */