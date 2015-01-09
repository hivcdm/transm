#pragma once

#include <string>

namespace transm {

class TimeSpan
{
public:
	static const TimeSpan Year;
	static const TimeSpan Month;

	TimeSpan() : years_(0), months_(0){}
	TimeSpan(int years, int months) : years_(years), months_(months) {}

	int get_years() const { return years_; }
	int get_months() const { return months_; }

	double get_total_years() const { return years_ + months_ / 12.0; }
	int get_total_months() const { return years_ * 12 + months_; }

	TimeSpan &operator++() { months_++; normalize(); return *this; }
	TimeSpan operator++(int) { TimeSpan copy(years_, months_); months_++; normalize(); return copy; }

	TimeSpan operator+(const TimeSpan &other) const { return TimeSpan{ years_ + other.years_, months_ + other.months_ }; }
	TimeSpan &operator+=(const TimeSpan &other) { years_ += other.years_; months_ += other.months_; return *this; }
	TimeSpan operator-(const TimeSpan &other) const { return TimeSpan{ years_ - other.years_, months_ - other.months_ }; }
	TimeSpan &operator-=(const TimeSpan &other) { years_ -= other.years_; months_ -= other.months_; return *this; }
	TimeSpan operator-() const { return TimeSpan{ -years_, -months_ }; }

	bool operator==(const TimeSpan &other) const { return normalized().get_total_months() == other.normalized().get_total_months(); }
	bool operator!=(const TimeSpan &other) const { return !(*this == other); }
	bool operator>(const TimeSpan &other) const { return normalized().get_total_months() > other.normalized().get_total_months(); }
	bool operator>=(const TimeSpan &other) const { return normalized().get_total_months() >= other.normalized().get_total_months(); }
	bool operator<(const TimeSpan &other) const { return normalized().get_total_months() < other.normalized().get_total_months(); }
	bool operator<=(const TimeSpan &other) const { return normalized().get_total_months() <= other.normalized().get_total_months(); }

private:
	friend class Time;

	void normalize()
	{
		years_ += months_ / 12;
		months_ %= 12;
	}

	TimeSpan normalized() const
	{
		TimeSpan copy(years_, months_);
		copy.normalize();
		return copy;
	}

	int years_;
	int months_;
};

class Time
{
public:
	static const Time Zero;
	static Time from_months(int months) { return Time(months / 12, months % 12); }

	Time() : year_(0), month_(0) {}
	Time(int year, int month) : year_(year), month_(month) {}

	int get_year() const { return year_; }
	int get_month() const { return month_; }

	double get_total_years() const { return year_ + month_ / 12.0; }
	int get_total_months() const { return year_ * 12 + month_; }

	Time &operator++() { month_++; normalize(); return *this; }
	Time operator++(int) { Time copy(year_, month_); month_++; normalize(); return copy; }

	Time operator+(const TimeSpan &other) const { return Time{ year_ + other.years_, month_ + other.months_ }.normalized(); }
	Time &operator+=(const TimeSpan &other) { year_ += other.years_; month_ += other.months_; normalize(); return *this; }
	Time operator-(const TimeSpan &other) const { return Time{ year_ - other.years_, month_ - other.months_ }.normalized(); }
	TimeSpan operator-(const Time &other) const { return TimeSpan{ year_ - other.year_, month_ - other.month_ }.normalized(); }
	Time &operator-=(const TimeSpan &other) { year_ -= other.years_; month_ += other.months_; normalize(); return *this; }

	bool operator==(const Time &other) const { return get_total_months() == other.get_total_months(); }
	bool operator!=(const Time &other) const { return !(*this == other); }
	bool operator>(const Time &other) const { return get_total_months() > other.get_total_months(); }
	bool operator>=(const Time &other) const { return get_total_months() >= other.get_total_months(); }
	bool operator<(const Time &other) const { return get_total_months() < other.get_total_months(); }
	bool operator<=(const Time &other) const { return get_total_months() <= other.get_total_months(); }

private:
	void normalize()
	{
		year_ += month_ / 12;
		month_ %= 12;
	}

	Time normalized() const
	{
		Time copy(year_, month_);
		copy.normalize();
		return copy;
	}

	int year_;
	int month_;
};

inline TimeSpan min(const TimeSpan &left, const TimeSpan &right)
{
	return left.get_total_months() < right.get_total_months() ? left : right;
}

inline TimeSpan max(const TimeSpan &left, const TimeSpan &right)
{
	return left.get_total_months() < right.get_total_months() ? right : left;
}

inline Time min(const Time &left, const Time &right)
{
	return left.get_total_months() < right.get_total_months() ? left : right;
}

inline Time max(const Time &left, const Time &right)
{
	return left.get_total_months() < right.get_total_months() ? right : left;
}

using Age = Time;

} // namespace transm
