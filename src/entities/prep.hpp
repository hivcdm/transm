#ifndef PREP_HPP
#define PREP_HPP

#include <memory>
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
    std::array<double, 4> adherence = {0.0, 0.0, 0.0, 0.0};
    std::array<double, 12> retention = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
                                        0.0, 0.0, 0.0, 0.0, 0.0};
    double returnToCare = 0.0;

    double efficacy;

    using ProfileMap = std::map<DemographicProfile, double>;
    using AdherenceProfileMap = std::map<DemographicProfile, std::array<double, 4>>;
    using RetentionProfileMap = std::map<DemographicProfile, std::array<double, 12>>;
    using AccessProfileMap = std::map<DemographicProfile, std::vector<double>>;
    struct ProfileTables
    {
        std::vector<double> access;
        AccessProfileMap accessProfiles;
        AdherenceProfileMap adherenceProfiles;
        RetentionProfileMap retentionProfiles;
        ProfileMap returnToCareProfiles;
    };

    /* Copies share the configuration tables. Individual defaults, including
     * adherence changed by an intervention, remain local to each person. */
    std::shared_ptr<ProfileTables> profiles = std::make_shared<ProfileTables>();

    ProfileTables &MutableProfiles()
    {
        if (!profiles.unique())
            profiles = std::make_shared<ProfileTables>(*profiles);
        return *profiles;
    }

public:
    bool Enabled() const { return enabled; }
    void SetEnabled(bool value) { enabled = value; }

    void SetEligibility(PrepEligibility elig) { eligibility = elig; }
    PrepEligibility GetEligibility() const { return eligibility; }

    void SetEfficacy(double value) { efficacy = value; }
    double GetEfficacy() const {
        return efficacy;
    }

    void SetDefaultAccess(vector<double> value) { MutableProfiles().access = std::move(value); }
    void SetProfileAccess(const DemographicProfile& profile, vector<double> value)
    {
        MutableProfiles().accessProfiles.emplace(profile, value);
    }
    const std::vector<double> &GetAccess(const DemographicProfile& profile) const
    {
        const std::vector<double> *value = &profiles->access;
        for (const auto& pair : profiles->accessProfiles)
            if (profile.match(pair.first))
                value = &pair.second;

        return *value;
    }

    void SetDefaultAdherence(array<double, 4> value) { adherence = value; }
    void SetProfileAdherence(const DemographicProfile& profile, std::array<double, 4> value)
    {
        MutableProfiles().adherenceProfiles.emplace(profile, value);
    }
    std::array<double, 4> GetAdherence(const DemographicProfile& profile) const
    {
        std::array<double, 4> value = adherence;
        for (const auto& pair : profiles->adherenceProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultRetention(array<double,  12> value) { retention = value; }
    void SetProfileRetention(const DemographicProfile& profile, std::array<double, 12> value)
    {
        MutableProfiles().retentionProfiles.emplace(profile, value);
    }
    std::array<double, 12> GetRetention(const DemographicProfile& profile) const
    {
        std::array<double, 12> value = retention;
        for (const auto& pair : profiles->retentionProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }

    void SetDefaultReturnToCare(double value) { returnToCare = value; }
    void SetProfileReturnToCare(const DemographicProfile& profile, double value)
    {
        MutableProfiles().returnToCareProfiles.emplace(profile, value);
    }
    double GetReturnToCare(const DemographicProfile& profile) const
    {
        double value = returnToCare;
        for (const auto& pair : profiles->returnToCareProfiles)
            if (profile.match(pair.first))
                value = pair.second;

        return value;
    }


};

} // namespace transm


#endif /* PREP_HPP */