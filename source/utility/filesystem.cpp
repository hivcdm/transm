#include <array>
#include <cassert>
#include <stdexcept>

#ifdef _WIN32
#include <Windows.h>
#else
#include <glob.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "filesystem.hpp"

const path &path::dot()
{
#ifdef _WIN32
    static const path dot_(L".");
#else
    static const path dot_(".");
#endif
    return dot_;
}

const path &path::dotdot()
{
#ifdef _WIN32
    static const path dotdot_(L"..");
#else
    static const path dotdot_("..");
#endif
    return dotdot_;
}

path::path()
{
}

path::path(const path &p) : pathname_(p.pathname_)
{

}
path::path(path &&p)
{
    swap(*this, p);
}

path::path(const std::string &s) : pathname_(s.begin(), s.end())
{
}

path::path(const std::wstring &s) : pathname_(s.begin(), s.end())
{
}

path::~path()
{
}

path &path::operator=(path p)
{
    swap(*this, p);
    return *this;
}

path &path::operator/=(const path &p)
{
    return append(p);
}

path &path::append(const path &p)
{
    if(pathname_.back() != preferred_separator && pathname_.back() != alternate_separator)
    {
        pathname_.append(1, preferred_separator);
    }

    pathname_.append(p.pathname_);
    return *this;
}

bool operator==(const path &left, const path &right)
{
    return left.pathname_ == right.pathname_;
}

bool operator!=(const path &left, const path &right)
{
    return !(left == right);
}

bool operator<(const path &left, const path &right)
{
    return left.pathname_ < right.pathname_;
}

bool operator<=(const path &left, const path &right)
{
    return left.pathname_ <= right.pathname_;
}

bool operator>(const path &left, const path &right)
{
    return left.pathname_ > right.pathname_;
}

bool operator>=(const path &left, const path &right)
{
    return left.pathname_ >= right.pathname_;
}

path operator/(const path &left, const path &right)
{
    if(right == path::dot())
    {
        return left;
    }
    if(right == path::dotdot())
    {
        return left.parent_path();
    }

    return path(left).append(right);
}

std::ostream &operator<<(std::ostream &stream, const path &p)
{
    stream << p.string();
    return stream;
}

std::wostream &operator<<(std::wostream &stream, const path &p)
{
    stream << p.wstring();
    return stream;
}

std::istream &operator>>(std::istream &stream, path &p)
{
    std::string temp;
    stream >> temp;
    p.pathname_ = path::string_type(temp.begin(), temp.end());
    return stream;
}
std::wistream &operator>>(std::wistream &stream, path &p)
{
    std::wstring temp;
    stream >> temp;
    p.pathname_ = path::string_type(temp.begin(), temp.end());
    return stream;
}

std::string path::string() const
{
    return std::string(pathname_.begin(), pathname_.end());
}

std::wstring path::wstring() const
{
    return std::wstring(pathname_.begin(), pathname_.end());
}

path::string_type path::native() const
{
    return pathname_;
}

path path::root_name() const
{
    if(!empty())
    {
        auto p = split_path().front();

#ifdef _WIN32
        if(p.size() == 2 && p.back() == ':' && p.front() >= 'A' && p.front() <= 'Z')
        {
            return p;
        }
        else if(p.size() > 2 && p[0] == '\\' && p[1] == '\\')
        {
            return p;
        }
#else
        if(p.size() > 1 && p[0] == '\\')
        {
            return p;
        }
#endif
    }

    return path();
}

path path::root_directory() const
{
    return path();
}

path path::root_path() const
{
    return root_name() / root_directory();
}

path path::relative_path() const
{
    if(empty()) return path();
    if(root_name().empty()) return path(pathname_);
    auto split = split_path();
    if(split.size() == 1) return path(split[0]);
    split.erase(split.begin());
    path combined;
    for(auto part : split)
    {
        combined /= part;
    }
    return combined;
}

path path::parent_path() const
{
    auto split = split_path();
    if(empty() || split.front() == split.back()) return path();
    path combined = split.front();
    split.erase(split.begin());
    split.pop_back();

    for(auto part : split)
    {
	if(part != path::dot())
	{
	    combined /= part;
	}
    }

    return combined;
}

path path::filename() const
{
    return empty() ? path() : split_path().back();
}

bool filesystem::is_dotdir(const path &to_check)
{
    return to_check == path::dot() || to_check == path::dotdot();
}

path path::stem() const
{
    if(empty()) return path();
    auto name = filename().pathname_;
    if(filesystem::is_dotdir(name)) return name;
    auto dot_index = name.find_last_of('.');
    if(dot_index == string_type::npos) return name;
    return name.substr(0, dot_index);
}

path path::extension() const
{
    if(empty()) return path();
    auto name = filename().pathname_;
    if(filesystem::is_dotdir(name)) return path();
    auto dot_index = name.find_last_of('.');
    if(dot_index == string_type::npos) return path();
    return name.substr(dot_index);
}

bool path::empty() const
{
    return pathname_.empty();
}

bool path::is_absolute() const
{
    if(empty()) return false;

    auto is_root = [](const string_type &p)
    {
#ifdef _WIN32
        if(p.size() == 2 && p.back() == ':' && p.front() >= 'A' && p.front() <= 'Z')
        {
            return true;
        }
        else if(p.size() > 2 && p[0] == '\\' && p[1] == '\\')
        {
            return true;
        }
#else
        if(p.size() > 1 && p[0] == '\\')
        {
            return true;
        }
#endif
        return false;
    };

    return is_root(split_path().front());
}

bool path::is_relative() const
{
    return !is_absolute();
}

std::vector<path::string_type> path::split_path() const
{
    std::vector<path::string_type> parts;

    auto find_next_separator = [this](string_type::size_type offset)
    {
        auto index = pathname_.find(preferred_separator, offset);
        if(index == string_type::npos)
        {
            index = pathname_.find(alternate_separator, offset);
        }
        return index;
    };

    string_type::size_type previous_index = 0;
    auto separator_index = find_next_separator(previous_index);

    while(separator_index != string_type::npos)
    {
        auto part = pathname_.substr(previous_index, separator_index - previous_index);

        if(part == path::dotdot().native())
        {
            parts.pop_back();
        }
        else
        {
            parts.push_back(part);
        }

        previous_index = separator_index + 1;
        separator_index = find_next_separator(previous_index);
    }

    parts.push_back(pathname_.substr(previous_index, separator_index - previous_index));

    return parts;
}

void swap(path &left, path &right)
{
    using std::swap;
    swap(left.pathname_, right.pathname_);
}

std::vector<path> filesystem::listdir(const path &directory)
{
    std::vector<path> contents;

#ifdef _WIN32
    WIN32_FIND_DATA ffd;
    auto hfind = FindFirstFile((directory / path("*")).native().c_str(), &ffd);

    if(hfind != INVALID_HANDLE_VALUE)
    {
        do
        {
            path current_path(path::string_type(ffd.cFileName));
            if(current_path == path::dot() || current_path == path::dotdot())
            {
                continue;
            }
            contents.push_back(directory / current_path);
        } while(FindNextFile(hfind, &ffd) != 0);
    }
#else
    glob_t glob_result;
    glob(directory.native().c_str(), GLOB_TILDE, nullptr, &glob_result);
    for(std::size_t i = 0; i < glob_result.gl_pathc; ++i)
    {
        contents.push_back(path::string_type(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
#endif

    return contents;
}

bool filesystem::is_directory(const path &p)
{
#ifdef _WIN32
    auto attributes = GetFileAttributes(p.native().c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#else
    struct stat st_buf;
    auto status = stat(p.native().c_str(), &st_buf);
    if(status != 0)
    {
      return false;
    }
    return S_ISDIR(st_buf.st_mode);
#endif
}

bool filesystem::is_regular_file(const path &p)
{
#ifdef _WIN32
    auto attributes = GetFileAttributes(p.native().c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_DIRECTORY) == 0;
#else
    struct stat st_buf;
    auto status = stat(p.native().c_str(), &st_buf);
    if(status != 0)
    {
      return false;
    }
    return S_ISREG(st_buf.st_mode);
#endif
}


bool filesystem::exists(const path &p)
{
    return is_directory(p) || is_regular_file(p);
}

path filesystem::current_path()
{
#ifdef _WIN32
    std::array<TCHAR, MAX_PATH> buffer;
    GetCurrentDirectory((DWORD)buffer.size(), buffer.data());
    return path(path::string_type(buffer.begin(), buffer.end()));
#else
    std::array<char, 512> buffer;
    assert(getcwd(buffer.data(), buffer.size()) != nullptr);
    return path(std::string(buffer.begin(), buffer.end()));
#endif
}
