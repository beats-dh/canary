/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#include "pch.hpp"

#include "database/databasetasks.h"
#include "game/scheduling/tasks.h"

DatabaseTasks::DatabaseTasks() {
  db_ = &Database::getInstance();
}

bool DatabaseTasks::SetDatabaseInterface(Database *database) {
  if (database == nullptr) {
    return false;
  }

  db_ = database;
  return true;
}

void DatabaseTasks::start()
{
  if (db_ == nullptr) {
    return;
  }
  std::unique_lock<std::mutex> lock(connection_lock);
	db_->connect();
	ThreadHolder::start();
}

void DatabaseTasks::threadMain()
{
    std::unique_lock<std::mutex> taskLockUnique(task_lock, std::defer_lock);
    thread_state.store(true); //atribui o estado ativo a thread_state
    while (thread_state.load()) {
        if(!taskLockUnique.try_lock()){
            continue;
        }
        if (tasks.empty()) {
            task_signal.wait(taskLockUnique, [this] {
                if(!thread_state.load()) {
                    return true;
                }
                return !tasks.empty();
            });
        }

        if (!tasks.empty()) {
            DatabaseTask task = std::move(tasks.front());
            tasks.pop_back();
            taskLockUnique.unlock();
            if(!task.query.empty()){
                std::packaged_task<void()> task_handle(std::bind(&DatabaseTasks::runTask, this, task));
                task_handle();
            }
        } else {
            taskLockUnique.unlock();
        }
    }
}

void DatabaseTasks::addTask(std::string query, std::function<void(DBResult_ptr, bool)> callback/* = nullptr*/, bool store/* = false*/)
{
  std::unique_lock<std::mutex> lock(task_lock);
  if (getState() == THREAD_STATE_RUNNING) {
    tasks.emplace_back(std::move(query), std::move(callback), store);
    task_signal.notify_one();
  }
}


void DatabaseTasks::runTask(const DatabaseTask& task)
{
    if (db_ == nullptr) {
        return;
    }
    bool success;
    DBResult_ptr result;
    std::call_once(init_flag, [&] {
        if (task.store) {
            result = db_->storeQuery(task.query);
            success = true;
        } else {
            result = nullptr;
            success = db_->executeQuery(task.query);
        }
    });

    if (task.callback) {
        g_dispatcher().addTask(createTask(std::bind(task.callback, result, success)));
    }
}

void DatabaseTasks::flush() {
  std::unique_lock<std::mutex> guard(task_lock);
  while (!tasks.empty() || !task_handle_list.empty()) {
    task_signal.wait(guard, [this] {
      return tasks.empty() && task_handle_list.empty();
    });
  }
  tasks.clear();
  task_handle_list.clear();
}

void DatabaseTasks::shutdown()
{
    std::unique_lock<std::mutex> guard(task_lock, std::defer_lock);
    if(!guard.try_lock()){
        return;
    }
    setState(THREAD_STATE_TERMINATED);
    std::call_once(shutdown_flag, [&] {
        flush();
        task_signal.notify_one();
    });
}
