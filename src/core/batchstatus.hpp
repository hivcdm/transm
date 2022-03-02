#ifndef BATCHSTATUS_HPP
#define BATCHSTATUS_HPP

//#pragma once

#include <string>
#include <vector>

#include "utility/filesystem.hpp"

struct sqlite3;

namespace transm {

/**
 * Every simulation is in one of these states.
 */
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

/**
 * A BatchStatus tracks the progress of all simulations that have been
 * registered with it through the initialize method. Simulations are referred
 * to by their name as a string. */
class BatchStatus
{
public:

    /** Construct a BatchStatus to keep track of the batch with the given name. */
    explicit BatchStatus(std::string batch_name);

    /** Destructor for BatchStatus. */
    ~BatchStatus();

    /**
     * Register the provided set of simulations with this batch status.
     * This should be done before calling the set_* methods. */
    void initialize(const std::vector<path> &sim_names);

    /** Set the percent complete for the simulation named sim_name to percent_complete. */
    void set_percent_complete(const std::string &sim_name, int percent_complete);

    /** Set the process id for the simulation named sim_name to process_id.  */
    void set_process_id(const std::string &sim_name, std::size_t process_id);

    /** Set the sim state for the simulation named sim_name to new_state. */
    void set_state(const std::string &sim_name, SimState new_state);

    /** Assignment operator. */
    BatchStatus &operator=(const BatchStatus &other) = delete;

private:
    /** Create a table in the database to track simulation progress. */
    void create_sim_table();

    /** Callback for SQLite functions. */
    void handle_query_results(int i, char **a, char **b);

    /** Create a new record in the sim table to hold the simulation of the given name. */
    void insert_sim(const std::string &sim_name);

    /** Returns true if a simulation with the given name already exists in the database. */
    bool sim_exists(const std::string &sim_name);

    /** Returns true if a table for tacking simulations already exists. */
    bool sim_table_exists();

    /** Close the database. */
    void shutdown();

    /**
     * Throw a std::exception if function_result indicates an error.
     * Used as a callback for sqlite functions. */
    static void throw_if_not_ok(int function_result);

private:

     /** The name of the associated batch (usually a folder containing input files). */
    const std::string batch_name_;

     /** The name of the SQLite database that this object will use. */
    const std::string batch_database_filename_ = "status.db";

     /** The result of the last query that returns a boolean is stored in this member. */
    bool bool_query_result_;

    /** The SQLite database that acts as the backend for this object. */
    sqlite3 *db_;

    /** True if initialize has been called. */
    bool initialized_;
};

} // namespace transm

#endif /* BATCHSTATUS_HPP */