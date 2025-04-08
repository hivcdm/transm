#include <sqlite3.h>

#include <utility>

#include "batchstatus.hpp"
#include "utility/utility.hpp"

namespace transm {

BatchStatus::BatchStatus(std::string batch_name)
    : batch_name_(std::move(batch_name)),
    bool_query_result_(false),
    db_(nullptr),
    initialized_(false)
{
}

BatchStatus::~BatchStatus()
{
    if(initialized_)
    {
        shutdown();
    }
}

/** Create the database if it doesn't exists and open it.
 * Insert each given simulation as a new row in the database.
 * After this is called, the object is "initialized". */
void BatchStatus::initialize(const std::vector<path> &sim_names)
{
    throw_if_not_ok(sqlite3_initialize());

    assert(!sim_names.empty());
    auto batch_directory = sim_names.front().parent_path();
    auto db_path = batch_directory / batch_database_filename_;

    int rc = sqlite3_open(db_path.string().c_str(), &db_);
    if (rc != SQLITE_OK) {
        sqlite3_close(db_);
        throw_if_not_ok(rc);
    }

    try {
        create_sim_table();

        for(const auto& sim : sim_names)
        {
            insert_sim(sim.stem().string());
        }

        initialized_ = true;
    } catch (...) {
        sqlite3_close(db_);
        db_ = nullptr;
        throw;
    }
}

void BatchStatus::set_process_id(const std::string &sim_name, std::size_t process_id)
{
    std::string query = "UPDATE sim SET process_id=" + std::to_string(process_id) + " WHERE name=\"" + sim_name + "\"; ";
    throw_if_not_ok(sqlite3_exec(db_, query.c_str(), nullptr, nullptr, nullptr));
}

void BatchStatus::set_state(const std::string &sim_name, SimState new_state)
{
    std::string query = "UPDATE sim SET state=" + std::to_string((int)new_state) + " WHERE name=\"" + sim_name + "\"; ";
    throw_if_not_ok(sqlite3_exec(db_, query.c_str(), nullptr, nullptr, nullptr));
}

void BatchStatus::set_percent_complete(const std::string &sim_name, int percent_complete)
{
    std::string query = "UPDATE sim SET percent=" + std::to_string(percent_complete) + " WHERE name=\"" + sim_name + "\"; ";
    throw_if_not_ok(sqlite3_exec(db_, query.c_str(), nullptr, nullptr, nullptr));
}

void BatchStatus::shutdown()
{
    throw_if_not_ok(sqlite3_close(db_));
    db_ = nullptr;
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

bool BatchStatus::sim_table_exists()
{
    std::string query = "SELECT name FROM sqlite_master WHERE type='table' AND name='sim'";
    auto callback = [](void *opaque, int i, char **a, char **b)
    {
        BatchStatus &status = *((BatchStatus *)opaque);
        status.handle_query_results(i, a, b);
        return 0;
    };

    bool_query_result_ = false;
    throw_if_not_ok(sqlite3_exec(db_, query.c_str(), callback, this, nullptr));
    return bool_query_result_;
}

void BatchStatus::handle_query_results(int i, char ** /*a*/, char ** /*b*/)
{
    bool_query_result_ = i != 0;
}

void BatchStatus::create_sim_table()
{
    if(!sim_table_exists())
    {
        std::string query = "CREATE TABLE sim (name CHAR(260) PRIMARY KEY NOT NULL, state INT, percent INT, process_id INT);";
        throw_if_not_ok(sqlite3_exec(db_, query.c_str(), nullptr, nullptr, nullptr));
    }
}

bool BatchStatus::sim_exists(const std::string &sim_name)
{
    std::string query = "SELECT * FROM sim WHERE name='" + sim_name + "'";

    auto callback = [](void *opaque, int i, char **a, char **b)
    {
        BatchStatus &status = *((BatchStatus *)opaque);
        status.handle_query_results(i, a, b);
        return 0;
    };

    bool_query_result_ = false;
    throw_if_not_ok(sqlite3_exec(db_, query.c_str(), callback, this, nullptr));
    return bool_query_result_;
}

void BatchStatus::insert_sim(const std::string &sim_name)
{
    if(!sim_exists(sim_name))
    {
        std::string query = "INSERT INTO sim VALUES (\"" + sim_name + "\", 0, 0, 0);";
        throw_if_not_ok(sqlite3_exec(db_, query.c_str(), nullptr, nullptr, nullptr));
    }
}

} // namespace transm
