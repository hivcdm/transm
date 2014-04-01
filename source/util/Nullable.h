#pragma once

template<typename T>
struct Nullable
{
	bool has_value = false;
	T value;
};
