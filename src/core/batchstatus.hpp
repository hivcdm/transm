#ifndef _BATCHSTATUS_H_
#define _BATCHSTATUS_H_

//#pragma once

#include <string>
#include <vector>

#include "utility/filesystem.hpp"

struct sqlite3;

namespace transm {

/// <summary>
/// Every simulation is in one of these states.
/// </summary>
enum class SimState
{
    queued,
    running,
    paused,
    stopped,
    failed,
    error,
    completed
};

/// <summary>
/// A BatchStatus tracks the progress of all simulations that have been
/// registered with it through the initialize method. Simulations are referred
/// to by their name as a string.
/// </summary>
class BatchStatus
{
public:
    /// <summary>
    /// Construct a BatchStatus to keep track of the batch with the given name.
    /// </summary>
    BatchStatus(const std::string &batch_name);

    /// <summary>
    /// Destructor for BatchStatus.
    /// </summary>
    ~BatchStatus();

    /// <summary>
    /// Register the provided set of simulations with this batch status.
    /// This should be done before calling the set_* methods.
    /// </summary>
    void initialize(const std::vector<path> &sim_names);

    /// <summary>
    /// Set the percent complete for the simulation named sim_name to percent_complete.
    /// </summary>
    void set_percent_complete(const std::string &sim_name, int percent_complete);

    /// <summary>
    /// Set the process id for the simulation named sim_name to process_id.
    /// </summary>
    void set_process_id(const std::string &sim_name, std::size_t process_id);

    /// <summary>
    /// Set the sim state for the simulation named sim_name to new_state.
    /// </summary>
    void set_state(const std::string &sim_name, SimState new_state);

private:
    /// <summary>
    /// Create a table in the database to track simulation progress.
    /// </summary>
    void create_sim_table();

    /// <summary>
    /// Callback for SQLite functions.
    /// </summary>
    void handle_query_results(int i, char **a, char **b);

    /// <summary>
    /// Create a new record in the sim table to hold the simulation of the given name.
    /// </summary>
    void insert_sim(const std::string &sim_name);

    /// <summary>
    /// Returns true if a simulation with the given name already exists in the database.
    /// </summary>
    bool sim_exists(const std::string &sim_name);

    /// <summary>
    /// Returns true if a table for tacking simulations already exists.
    /// </summary>
    bool sim_table_exists();

    /// <summary>
    /// Close the database.
    /// </summary>
    void shutdown();

    /// <summary>
    /// Throw a std::exception if function_result indicates an error.
    /// Used as a callback for sqlite functions.
    /// </summary>
    void throw_if_not_ok(int function_result);

    /// <summary>
    /// Assignment operator.
    /// </summary>
    BatchStatus &operator=(const BatchStatus &other) = delete;

    /// <summary>
    /// The name of the associated batch (usually a folder containing input files).
    /// </summary>
    const std::string batch_name_;

    /// <summary>
    /// The name of the SQLite database that this object will use.
    /// </summary>
    const std::string batch_database_filename_ = "status.db";

    /// <summary>
    /// The result of the last query that returns a boolean is stored in this
    /// member.
    /// </summary>
    bool bool_query_result_;

    /// <summary>
    /// The SQLite database that acts as the backend for this object.
    /// </summary>
    sqlite3 *db_;

    /// <summary>
    /// True if initialize has been called.
    /// </summary>
    bool initialized_;
};

} // namespace transm

#endif