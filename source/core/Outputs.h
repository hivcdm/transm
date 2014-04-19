#pragma once

#include <array>
#include <cassert>
#include <unordered_map>

#include "../util/Nullable.h"

class Variant
{
public:
	static const Variant Null;

	enum class Type
	{
		Null,
		Integer,
		Float,
		String
	};

	Variant() : type_(Type::Null) {}
	explicit Variant(int v);
	explicit Variant(double d);
	explicit Variant(const std::string &s);

	void operator=(const Variant &other)
	{
		type_ = other.type_;
		integer_value_ = other.integer_value_;
		float_value_ = other.float_value_;
		string_value_ = other.string_value_;
	}

	void operator=(int v) { type_ = Type::Integer; integer_value_ = v; }
	void operator=(double v) { type_ = Type::Integer; float_value_ = v; }
	void operator=(const std::string &v) { type_ = Type::String; string_value_ = v; }

	bool IsNull() const { return type_ == Type::Null; }
	bool IsInteger() const { return type_ == Type::Integer; }
	bool IsFloat() const { return type_ == Type::Float; }
	bool IsString() const { return type_ == Type::String; }
	Type GetType() const { return type_; }
	int GetInteger() const { assert(IsInteger()); return integer_value_; }
	double GetFloat() const { assert(IsFloat()); return float_value_; }
	std::string GetString() const { assert(IsString()); return string_value_; }

	template<typename T>
	T Get() const;

	std::string ToString() const
	{
		switch(type_)
		{
		case Type::Null: return "";
		case Type::Integer: return std::to_string(integer_value_);
		case Type::Float: return std::to_string(float_value_);
		case Type::String: return string_value_;
		}
		throw std::runtime_error("invalid type");
	}

private:
	Type type_;
	int integer_value_;
	double float_value_;
	std::string string_value_;
};


template<>
inline int Variant::Get() const { return GetInteger(); }

template<>
inline double Variant::Get() const { return GetFloat(); }

template<>
inline std::string Variant::Get() const { return GetString(); }

struct Cell
{
	Variant value;
};

template<std::size_t Width>
struct Row
{
	std::array<Cell, Width> cells;
};

template<std::size_t Width>
class TabularOutput2
{
	Nullable<Row<Width>> initial_row;
	std::unordered_map<int, Row<Width>> rows;
};

struct ArtRolloutStatistics
{
	TabularOutput2<178> data;
};

struct CalibrationStatistics
{
	TabularOutput2<178> data;
};

struct ClinicalStatistics
{
	TabularOutput2<178> data;
};

struct CostEffectivenessStatistics
{
	TabularOutput2<178> data;
};

struct Events
{
	TabularOutput2<178> data;
};

struct HealthStatistics
{
	TabularOutput2<178> data;
};

struct InfectionsStatistics
{
	TabularOutput2<178> data;
};

struct LifeExpectancyStatistics
{
	TabularOutput2<178> data;
};

struct PartnerAcquisitionStatistics
{
	TabularOutput2<178> data;
};

struct PartnershipStatistics
{
	TabularOutput2<178> data;
};

struct PopulationStatistics2
{
	TabularOutput2<178> data;
};

struct ShiftedOutcomes
{
	TabularOutput2<178> data;
};

struct SinglePersonTrace
{
	TabularOutput2<178> data;
};

struct SurvivalStatistics
{
	TabularOutput2<178> data;
};

struct CepacStatistics
{
	TabularOutput2<178> data;
};

class Outputs
{
public:
	Outputs() {}

	ArtRolloutStatistics art_rollout;
	CalibrationStatistics calibration;
	ClinicalStatistics clinical;
	CostEffectivenessStatistics cost_effectiveness;
	Events events;
	HealthStatistics health;
	InfectionsStatistics infections;
	LifeExpectancyStatistics life_expectancy;
	PartnerAcquisitionStatistics partner_acqusition;
	PartnershipStatistics partnership;
	PopulationStatistics2 population;
	ShiftedOutcomes shifted_outcomes;
	SinglePersonTrace single_person;
	SurvivalStatistics survival;
	CepacStatistics cepac;
};
