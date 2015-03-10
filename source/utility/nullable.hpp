#pragma once

namespace transm {

template<typename T>
struct Nullable
{
	bool has_value = false;
	T value;

	bool operator==(const Nullable &other) const { return has_value ? other.has_value && value == other.value : !other.has_value; }
};

} // namespace transm
