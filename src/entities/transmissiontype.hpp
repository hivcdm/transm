#ifndef TRANSMISSIONTYPE_HPP
#define TRANSMISSIONTYPE_HPP

namespace transm {

enum class TransmissionType
{
    female_to_male,
    male_to_female,
    male_to_male
};

} // namespace transm

namespace std {

 /** Specialize std::hash for TransmissionType */
template<>
struct hash<transm::TransmissionType>
{
    using underlying_type = underlying_type<transm::TransmissionType>::type;
    using hasher = hash<underlying_type>;

    size_t operator()(const transm::TransmissionType &t) const
    {
        return hasher()(static_cast<underlying_type>(t));
    }
};

} // namespace std


#endif /* TRANSMISSIONTYPE_HPP */