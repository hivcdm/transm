#pragma once

#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "filesystem.hpp"
#include "randomnumbergenerator.hpp"
#include "core/constants.hpp"

namespace transm {

struct Version
{
	static Version from_string(const std::string &version_string);
	static std::string to_string(const Version &version);
	static int compare(const Version &v1, const Version &v2, bool ignore_patch = false);

	int major = 0;
	int minor = 1;
	int patch = 0;
};

class Utility
{
    //usually we would divide to convert between these time increments
    //but division is more expensive, so multiply by inverse instead.
    static double day_to_month_multiplier;
    static double day_to_year_multiplier;
    static double month_to_year_multiplier;

public:
    static std::size_t get_current_process_id();

    /// <summary>
    /// Return the location in which the currently executing model is stored.
    /// </summary>
    static path get_model_directory();

    /// <summary>
    /// Return the user directory ($HOME). This depends on platform.
    /// </summary>
    static path get_user_directory();

    /// <summary>
    /// Return the location of the config file. This depends on platform.
    /// </summary>
    static path get_config_file_path();

    /// <summary>
    /// Return a version object that represents the version of the running model.
    /// </summary>
    static Version get_model_version();

    /// <summary>
    /// Return an unordered map of key->value config options.
    /// </summary>
    static std::unordered_map<std::string, std::string> load_config();

    /// <summary>
    /// Returns the directory in which batches are stored.
    /// </summary>
    static path get_batches_directory();

    // TODO: can we use types to replace this method? e.g. TimeDays, TimeMonths, etc.
    /// <summary>
	/// Convert val from "from" TimeGranularity to "to" TimeGranularity
    /// </summary>
	static unsigned int convert_time(TimeGranularity from, TimeGranularity to, double val);

	// TODO: replace with standard library functions like std::stod
	/// <summary>
    /// Converts a string value to another datatype.
	/// </summary>
	template <class T>
	static T from_string(std::string s);

    // TODO: replace
    /// <summary>
	/// Returns true if need is a member of haystack
    /// </summary>
	template <class T>
	static bool member_of(const std::set<T> &haystack, const T &needle);

    // TODO: replace
    /// <summary>
    /// Returns true if need is a member of haystack
    /// </summary>
    template <class T>
    static bool member_of(const std::vector<T> &haystack, const T &needle);

    /// <summary>
    /// Divide a vector of relative probabilities by the sum so that they add up to 1.
    /// </summary>
    /// <remarks>
    /// This is used for sampling from a categorical distribution.
    /// </summary>
	static void normalize(std::vector<double> &weights);

    // TODO: is this being used?
    /// <summary>
	/// Convert a probability, prob, to a rate
    /// </summary>
	static double prob_to_rate(double prob);

    // TODO: is this being used?
	/// <summary>
    /// Converts a rate, rate, to a probability.
    /// </summary>
	static double rate_to_prob(double rate);

	template <typename T>
    static T round(double d);

	static bool is_norm_dist_zero(const NormalDist &dist);
	
    /// <summary>
	/// returns true if _val is within [_min,_max]
	/// </summary>
	template <class T>
	static bool within_range(T val, T min, T max);

    /// <summary>
	/// Returns true if 0.0 <= prob <= 1.0
    /// </summary>
	static bool valid_probability(double prob);

    /// <summary>
    /// Return the set of tokens resulting form splitting str on provided delimiters.
    /// </summary>
	static std::vector<std::string> tokenize(const std::string &str, const std::string &delimiters);
};

// Template implementations.

template <typename T>
T Utility::round(double d)
{
    double decimals = d - std::floor(d);

    if(decimals >= 0.5)
    {
        return static_cast<T>(std::ceil(d));
    }
    else
    {
        return static_cast<T>(std::floor(d));
    }
}

template <class T>
bool Utility::member_of(const std::set<T> &haystack, const T &needle)
{
    return haystack.find(needle) != haystack.end();
}

template <class T>
bool Utility::member_of(const std::vector<T> &haystack, const T &needle)
{
    return std::find(haystack.begin(), haystack.end(), needle) != haystack.end();
}

template <class T>
T Utility::from_string(const std::string s)
{
	std::istringstream converter(s);
	T converted;
	converter >> converted;
	return converted;
}

template <class T>
bool Utility::within_range(T val, T min, T max)
{
	return min <= val && max >= val;
}

} // namespace transm
