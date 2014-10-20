#pragma once

enum class TransmissionType
{
    female_to_male,
    male_to_female,
    male_to_male
};

namespace std {
  template<>
  struct hash<TransmissionType>
  {
    using underlying_type = std::underlying_type<TransmissionType>::type;

    size_t operator()(const TransmissionType &t) const
    {
      return hasher((underlying_type)t);
    }

    hash<std::underlying_type<TransmissionType>::type> hasher;
  };
} // namespace std
