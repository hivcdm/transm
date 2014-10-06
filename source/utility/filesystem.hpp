#pragma once

#include <iostream>
#include <string>
#include <vector>

class path
{
public:
#if defined(_WIN32) && defined(_UNICODE)
    using value_type = wchar_t;
    const value_type preferred_separator = L'\\';
    const value_type alternate_separator = L'/';
#else
    using value_type = char;
#if defined(_WIN32)
    const value_type preferred_separator = '\\';
    const value_type alternate_separator = '/';
#else
    const value_type preferred_separator = '/';
    const value_type alternate_separator = '\\';
#endif
#endif
    using string_type = std::basic_string<value_type>;

    static const path &dot();
    static const path &dotdot();

    path();
    path(const path &p);
    path(path &&p);
    path(const std::string &s);
    path(const std::wstring &s);

    ~path();

    path &operator=(path p);

    path &operator/=(const path &p);
    path &append(const path &p);

    friend bool operator==(const path &left, const path &right);
    friend bool operator!=(const path &left, const path &right);
    friend bool operator<(const path &left, const path &right);
    friend bool operator<=(const path &left, const path &right);
    friend bool operator>(const path &left, const path &right);
    friend bool operator>=(const path &left, const path &right);

    friend path operator/(const path &left, const path &right);

    friend void swap(path &left, path &right);

    friend std::ostream &operator<<(std::ostream &stream, const path &p);
    friend std::wostream &operator<<(std::wostream &stream, const path &p);
    friend std::istream &operator>>(std::istream &stream, path &p);
    friend std::wistream &operator>>(std::wistream &stream, path &p);

    std::string string() const;
    std::wstring wstring() const;
    string_type native() const;

    path root_name() const;
    path root_directory() const;
    path root_path() const;
    path relative_path() const;
    path parent_path() const;
    path filename() const;
    path stem() const;
    path extension() const;

    bool empty() const;
    bool is_absolute() const;
    bool is_relative() const;

private:
    std::vector<string_type> split_path() const;
    string_type pathname_;
};

class filesystem
{
public:
    static std::vector<path> listdir(const path &p);
    static path absolute(const path &p);
    static path absolute(const path &p, const path &base);
    static path canonical(const path &p);
    static path canonical(const path &p, const path &base);
    static void copy_directory(const path &from, const path &to);
    static void copy_file(const path &from, const path &to);
    static bool create_directory(const path &p);
    static path current_path();
    static void current_path(const path &p);
    static bool exists(const path &p);
    static bool is_directory(const path &p);
    static bool is_regular_file(const path &p);
    static bool is_dotdir(const path &p);
    static bool remove_file(const path &p);
    static std::size_t remove_directory(const path &p);
    static void rename(const path &from, const path &to);
    static path temp_directory_path();
};
