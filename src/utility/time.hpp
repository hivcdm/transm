#ifndef TIME_HPP
#define TIME_HPP

#include <string>

namespace transm {

class TimeSpan
{
public:
	static const TimeSpan Year;
	static const TimeSpan Month;

	TimeSpan() : months_(0) {}
	TimeSpan(int years, int months) : months_(years * 12 + months) {}

	int get_years() const { return months_ / 12; }
	int get_months() const { return months_ % 12; }

	double in_years() const { return months_ / 12.0; }
	int in_months() const { return months_; }

	std::size_t years_as_index() const { return static_cast<std::size_t>(get_years()); }
	std::size_t months_as_index() const { return static_cast<std::size_t>(months_); }

	TimeSpan &operator++() { months_++; return *this; }
	TimeSpan operator++(int) { TimeSpan copy(months_); months_++; return copy; }

	TimeSpan operator+(const TimeSpan &other) const { return TimeSpan{ months_ + other.months_ }; }
	TimeSpan &operator+=(const TimeSpan &other) { months_ += other.months_; return *this; }
	TimeSpan operator-(const TimeSpan &other) const { return TimeSpan{ months_ - other.months_ }; }
	TimeSpan &operator-=(const TimeSpan &other) { months_ -= other.months_; return *this; }
	TimeSpan operator-() const { return TimeSpan{ -months_ }; }

	bool operator==(const TimeSpan &other) const { return months_ == other.months_; }
	bool operator!=(const TimeSpan &other) const { return !(*this == other); }
	bool operator>(const TimeSpan &other) const { return months_ > other.months_; }
	bool operator>=(const TimeSpan &other) const { return months_ >= other.months_; }
	bool operator<(const TimeSpan &other) const { return months_ < other.months_; }
	bool operator<=(const TimeSpan &other) const { return months_ <= other.months_; }

private:
	friend class Time;
	TimeSpan(int months) : months_(months) {}

	int months_;
};

class Time
{
public:
	static const Time Zero;
	static Time from_months(int months) { return Time(months / 12, months % 12); }

	Time() : month_(0) {}
	Time(int year, int month) : month_(year * 12 + month) {}

	int get_year() const { return month_ / 12; }
	int get_month() const { return month_ % 12; }

	double in_years() const { return month_ / 12.0; }
	int in_months() const { return month_; }

	std::size_t year_as_index() const { return static_cast<std::size_t>(get_year()); }
	std::size_t month_as_index() const { return static_cast<std::size_t>(month_); }

	Time &operator++() { month_++; return *this; }
	Time operator++(int) { Time copy(month_); month_++; return copy; }

	Time operator+(const TimeSpan &other) const { return Time{ month_ + other.months_ }; }
	Time &operator+=(const TimeSpan &other) { month_ += other.months_; return *this; }
	Time operator-(const TimeSpan &other) const { return Time{ month_ - other.months_ }; }
	TimeSpan operator-(const Time &other) const { return TimeSpan{ month_ - other.month_ }; }
	Time &operator-=(const TimeSpan &other) { month_ -= other.months_; return *this; }

	bool operator==(const Time &other) const { return month_ == other.month_; }
	bool operator!=(const Time &other) const { return !(*this == other); }
	bool operator>(const Time &other) const { return month_ > other.month_; }
	bool operator>=(const Time &other) const { return month_ >= other.month_; }
	bool operator<(const Time &other) const { return month_ < other.month_; }
	bool operator<=(const Time &other) const { return month_ <= other.month_; }

private:
	Time(int month) : month_(month) {}

	int month_;
};

inline TimeSpan min(const TimeSpan &left, const TimeSpan &right)
{
	return left.in_months() < right.in_months() ? left : right;
}

inline TimeSpan max(const TimeSpan &left, const TimeSpan &right)
{
	return left.in_months() < right.in_months() ? right : left;
}

inline Time min(const Time &left, const Time &right)
{
	return left.in_months() < right.in_months() ? left : right;
}

inline Time max(const Time &left, const Time &right)
{
	return left.in_months() < right.in_months() ? right : left;
}

using Age = Time;

} // namespace transm


#endif /* TIME_HPP */