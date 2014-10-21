#pragma once

namespace transm {

enum class TransmissionType
{
    female_to_male,
    male_to_female,
    male_to_male
};

} // namespace transm

namespace std {

/// <summary>
/// Specialize std::hash for TransmissionType
/// </summary>
template<>
struct hash<transm::TransmissionType>
{
    using underlying_type = underlying_type<transm::TransmissionType>::type;

    size_t operator()(const transm::TransmissionType &t) const
    {
        return hasher((underlying_type)t);
    }

    hash<underlying_type> hasher;
};

} // namespace std
