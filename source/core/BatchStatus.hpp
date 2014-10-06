#pragma once

#include <string>
#include <vector>

class BatchStatus;
struct sqlite3;

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

class SimStatus
{
public:
    SimStatus(const std::string &name, BatchStatus &batch);

    void set_progress(double progress);
    void set_state(SimState state);
    SimState get_state() const;

private:
    SimStatus &operator=(const SimStatus &s) = delete;

    const std::string name_;
    BatchStatus &batch_;
    SimState state_;
    double progress_;
};

class BatchStatus
{
public:
    BatchStatus(const std::string &batch_name);
    ~BatchStatus();

    void initialize(const std::vector<std::string> &task_names);
    void change_state(const std::string &filename, SimState new_state);
    void change_percent_complete(const std::string &filename, int percent_complete);

private:
    BatchStatus &operator=(const BatchStatus &other) = delete;

    void shutdown();
    void throw_if_not_ok(int function_result);
    bool task_table_exists();
    void handle_query_results(int i, char **a, char **b);
    void create_task_table();
    bool task_exists(const std::string &task_name);
    void insert_task(const std::string &task_name);

    const std::string batch_name_;
    const std::string batch_database_filename_ = "status.db";
    bool initialized_;
    sqlite3 *task_db_;
    bool bool_query_result_;
};
