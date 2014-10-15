#include <sqlite3.h>

#include "BatchStatus.hpp"
#include "utility/utility.hpp"

BatchStatus::BatchStatus(const std::string &batch_name)
    : batch_name_(batch_name),
    initialized_(false),
    task_db_(nullptr)
{
}

BatchStatus::~BatchStatus()
{
    if(initialized_)
    {
        shutdown();
    }
}

void BatchStatus::initialize(const std::vector<std::string> &task_names)
{
    throw_if_not_ok(sqlite3_initialize());

    auto batches_directory = Utility::get_batches_directory();
    auto db_path = batches_directory / batch_name_ / batch_database_filename_;

    throw_if_not_ok(sqlite3_open(db_path.string().c_str(), &task_db_));
    create_task_table();

    for(auto task : task_names)
    {
        insert_task(task);
    }

    initialized_ = true;
}

void BatchStatus::change_process_id(const std::string &filename, std::size_t process_id)
{
    std::string query = "UPDATE task SET process_id=" + std::to_string(process_id) + " WHERE name=\"" + filename + "\"; ";
    throw_if_not_ok(sqlite3_exec(task_db_, query.c_str(), nullptr, nullptr, nullptr));
}

void BatchStatus::change_state(const std::string &filename, SimState new_state)
{
    std::string query = "UPDATE task SET state=" + std::to_string((int)new_state) + " WHERE name=\"" + filename + "\"; ";
    throw_if_not_ok(sqlite3_exec(task_db_, query.c_str(), nullptr, nullptr, nullptr));
}

void BatchStatus::change_percent_complete(const std::string &filename, int percent_complete)
{
    std::string query = "UPDATE task SET percent=" + std::to_string(percent_complete) + " WHERE name=\"" + filename + "\"; ";
    throw_if_not_ok(sqlite3_exec(task_db_, query.c_str(), nullptr, nullptr, nullptr));
}

void BatchStatus::shutdown()
{
    throw_if_not_ok(sqlite3_close(task_db_));
    task_db_ = nullptr;
    throw_if_not_ok(sqlite3_shutdown());
    initialized_ = false;
}

void BatchStatus::throw_if_not_ok(int function_result)
{
    if(function_result != SQLITE_OK)
    {
        auto error_string = sqlite3_errstr(function_result);
        throw std::runtime_error(error_string);
    }
}

bool BatchStatus::task_table_exists()
{
    std::string query = "SELECT name FROM sqlite_master WHERE type='table' AND name='task'";
    auto callback = [](void *opaque, int i, char **a, char **b)
    {
        BatchStatus &status = *((BatchStatus *)opaque);
        status.handle_query_results(i, a, b);
        return 0;
    };

    bool_query_result_ = false;
    throw_if_not_ok(sqlite3_exec(task_db_, query.c_str(), callback, this, nullptr));
    return bool_query_result_;
}

void BatchStatus::handle_query_results(int i, char ** /*a*/, char ** /*b*/)
{
    bool_query_result_ = i != 0;
}

void BatchStatus::create_task_table()
{
    if(!task_table_exists())
    {
        std::string query = "CREATE TABLE task (name CHAR(260) PRIMARY KEY NOT NULL, state INT, percent INT, process_id INT);";
        throw_if_not_ok(sqlite3_exec(task_db_, query.c_str(), nullptr, nullptr, nullptr));
    }
}

bool BatchStatus::task_exists(const std::string &task_name)
{
    std::string query = "SELECT * FROM task WHERE name='" + task_name + "'";

    auto callback = [](void *opaque, int i, char **a, char **b)
    {
        BatchStatus &status = *((BatchStatus *)opaque);
        status.handle_query_results(i, a, b);
        return 0;
    };

    bool_query_result_ = false;
    throw_if_not_ok(sqlite3_exec(task_db_, query.c_str(), callback, this, nullptr));
    return bool_query_result_;
}

void BatchStatus::insert_task(const std::string &task_name)
{
    if(!task_exists(task_name))
    {
        std::string query = "INSERT INTO task VALUES (\"" + task_name + "\", 0, 0, 0);";
        throw_if_not_ok(sqlite3_exec(task_db_, query.c_str(), nullptr, nullptr, nullptr));
    }
}
