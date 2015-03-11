#include <fstream>
#include <iostream>

#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ShlObj.h>
#include <tchar.h>
#else
#include <unistd.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#include "utility.hpp"

namespace transm {

Version Version::from_string(const std::string &version_string)
{
	Version v;
	auto major_minor_separator = version_string.find('.');
	v.major = std::stoi(version_string.substr(0, major_minor_separator));
	auto revision_separator = version_string.find('.', major_minor_separator + 1);
	v.minor = std::stoi(version_string.substr(major_minor_separator + 1, revision_separator));
	if(revision_separator != std::string::npos)
	{
		v.patch = std::stoi(version_string.substr(revision_separator + 1));
	}
	return v;
}

std::string Version::to_string(const Version &version)
{
	return std::to_string(version.major) + "." + std::to_string(version.minor) + "." + std::to_string(version.patch);
}

int Version::compare(const Version &v1, const Version &v2, bool ignore_patch)
{
	if(v1.major != v2.major)
	{
		return v1.major - v2.major;
	}

	if(v1.minor != v2.minor)
	{
		return v1.minor - v2.minor;
	}

	if(!ignore_patch && v1.patch != v2.patch)
	{
		return v1.patch - v2.patch;
	}

	return 0;
}

double Utility::day_to_month_multiplier = 1.0 / 30;
double Utility::day_to_year_multiplier = 1.0 / 365;
double Utility::month_to_year_multiplier = 1.0 / 12;

std::size_t Utility::get_current_process_id()
{
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return getpid();
#endif
}

path Utility::get_model_directory()
{
#ifdef __APPLE__
    std::array<char, 1024> path;
    uint32_t size = static_cast<uint32_t>(path.size());

    if(_NSGetExecutablePath(path.data(), &size) == 0)
    {
        std::string executable_string(path.begin(), std::find(path.begin(), path.end(), '\0'));
        class path executable_path(executable_string);
        return executable_path.parent_path();
    }

    throw std::runtime_error("buffer too small, " + std::to_string(path.size()) + ", should be: " + std::to_string(size));
#elif defined(_WIN32)
    std::array<TCHAR, MAX_PATH> buffer;
    DWORD result = GetModuleFileName(nullptr, buffer.data(), (DWORD)buffer.size());

    if(result == 0 || result == buffer.size())
    {
        throw std::runtime_error("GetModuleFileName failed or buffer was too small");
    }

    auto full_string = std::string(buffer.begin(), buffer.begin() + result);
    return path(full_string).parent_path();
#else
    char arg1[20];
    char exepath[PATH_MAX + 1] = {0};

    sprintf(arg1, "/proc/%d/exe", getpid());
    readlink(arg1, exepath, 1024);
    return std::string(exepath).substr(0, std::strlen(exepath) - 9);
#endif
}

path Utility::get_user_directory()
{
    path user_directory;
#ifdef _WIN32
    std::array<TCHAR, MAX_PATH> path_array;
    assert(SHGetFolderPath(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, path_array.data()) == S_OK);
    path::string_type path_string(path_array.begin(), path_array.begin() + _tcslen(path_array.data()));
    user_directory = path_string;
#else
    char *home_path = getenv("HOME");
    assert(home_path != nullptr);
    user_directory = std::string(home_path);
#endif
    return user_directory;
}

path Utility::get_config_file_path()
{
#ifdef _WIN32
    return get_user_directory() / path("transm") / path("transm.config");
#else
    return get_user_directory() / path(".transm") / path("transm.config");
#endif
}

std::string get_executable_name()
{
#ifdef __APPLE__
    std::array<char, 1024> path;
    uint32_t size = static_cast<uint32_t>(path.size());

    if(_NSGetExecutablePath(path.data(), &size) == 0)
    {
        std::string executable_string(path.begin(), std::find(path.begin(), path.end(), '\0'));
        class path executable_path(executable_string);
        return executable_path.filename().string();
    }

    throw std::runtime_error("buffer too small, " + std::to_string(path.size()) + ", should be: " + std::to_string(size));
#elif defined(_WIN32)
    std::array<TCHAR, MAX_PATH> buffer;
    DWORD result = GetModuleFileName(nullptr, buffer.data(), (DWORD)buffer.size());

    if(result == 0 || result == buffer.size())
    {
        throw std::runtime_error("GetModuleFileName failed or buffer was too small");
    }

    auto full_string = std::string(buffer.begin(), buffer.begin() + result);
    return path(full_string).stem().string();
#else
    char arg1[20];
    char exepath[PATH_MAX + 1] = {0};

    sprintf(arg1, "/proc/%d/exe", getpid());
    readlink(arg1, exepath, 1024);
    std::string full_path(exepath);
    return full_path.substr(full_path.find_last_of('/'));
#endif
}

Version Utility::get_model_version()
{
    auto exe_name = get_executable_name();
    auto hyphen_index = exe_name.find_last_of('-');
    assert(hyphen_index != std::string::npos);
    auto version_string = exe_name.substr(hyphen_index + 2);
    return Version::from_string(version_string);
}

std::unordered_map<std::string, std::string> Utility::load_config()
{
    if(!filesystem::exists(get_config_file_path()))
    {
        throw std::runtime_error("config not found. run model using transm wrapper script.");
    }

    std::ifstream config_file(get_config_file_path().string());
    std::string line;
    static const std::string separator = " ::: ";
    std::unordered_map<std::string, std::string> config;

    while(std::getline(config_file, line))
    {
        bool found = false;
        std::size_t match_offset = 0;

        for(std::size_t offset = 0; offset < line.size() - separator.size(); offset++)
        {
            found = true;

            for(std::size_t i = 0; i < separator.size(); i++)
            {
                if(line[offset + i] != separator[i])
                {
                    found = false;
                    break;
                }
            }

            if(found)
            {
                match_offset = offset;
                break;
            }
        }

        if(found)
        {
            std::string key = line.substr(0, match_offset);
            std::string value = line.substr(match_offset + separator.size());
            config[key] = value;
        }
    }

    return config;
}

path Utility::get_batches_directory()
{
    static auto config = load_config();
    return config.at("batches_directory");
}

bool Utility::is_norm_dist_zero(const NormalDist &dist)
{
	return dist.mean == 0 && dist.stddev == 0;
}

void Utility::normalize(std::vector<double> &_weights)
{
	double total = 0;

	//see what the values currently total to
	for(unsigned int i = 0; i < _weights.size(); i++)
	{
		total += _weights.at(i);
	}

	//we want to divide by the total, but division is slow. so we will multiply by the inverse
	total = 1 / total;

	//normalize each proportionate value so that the sum of them ~ 1
	for(unsigned int i = 0; i < _weights.size(); i++)
	{
		_weights.at(i) = _weights.at(i) * total;
	}
}

double Utility::prob_to_rate(double _prob)
{
	assert(Utility::within_range<double>(_prob, 0.0, 1.0));
	return -log(1 - _prob);
}

double Utility::rate_to_prob(double _rate)
{
	//convert the cumulative rate back into a probability
	return 1 - exp(-_rate);
}

//this implementation was taken from
// http://www.oopweb.com/CPP/Documents/CPPHOWTO/Volume/C++Programming-HOWTO-7.html
std::vector<std::string> Utility::tokenize(const std::string &str,
                    const std::string &delimiters = " ")
{
    std::vector<std::string> tokens;

	// Skip delimiters at beginning.
	std::string::size_type lastPos = str.find_first_not_of(delimiters, 0);
	// Find first "non-delimiter".
	std::string::size_type pos = str.find_first_of(delimiters, lastPos);

	while(std::string::npos != pos || std::string::npos != lastPos)
	{
		// Found a token, add it to the vector.
		tokens.push_back(str.substr(lastPos, pos - lastPos));
		// Skip delimiters.  Note the "not_of"
		lastPos = str.find_first_not_of(delimiters, pos);
		// Find next "non-delimiter"
		pos = str.find_first_of(delimiters, lastPos);
	}

    return tokens;
}


bool Utility::valid_probability(double _prob)
{
	return Utility::within_range(_prob, 0.0, 1.0);
}

} // namespace transm
