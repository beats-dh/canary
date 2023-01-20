/**
 * Canary - A free and open-source MMORPG server emulator
 * Copyright (©) 2019-2022 OpenTibiaBR <opentibiabr@outlook.com>
 * Repository: https://github.com/opentibiabr/canary
 * License: https://github.com/opentibiabr/canary/blob/main/LICENSE
 * Contributors: https://github.com/opentibiabr/canary/graphs/contributors
 * Website: https://docs.opentibiabr.org/
*/

#ifndef SRC_DATABASE_DATABASETASKS_H_
#define SRC_DATABASE_DATABASETASKS_H_

#include "database/database.h"
#include "utils/thread_holder_base.h"

struct DatabaseTask {
	DatabaseTask(std::string&& initQuery, std::function<void(DBResult_ptr, bool)>&& initCallback, bool initStore) :
		query(std::move(initQuery)), callback(std::move(initCallback)), store(initStore) {}

	std::string query;
	std::function<void(DBResult_ptr, bool)> callback;
	bool store;
};

class DatabaseTasks : public ThreadHolder<DatabaseTasks>
{
	public:
		DatabaseTasks();

		// non-copyable
		DatabaseTasks(DatabaseTasks const&) = delete;
		void operator=(DatabaseTasks const&) = delete;

		static DatabaseTasks& getInstance() {
			// Guaranteed to be destroyed
			static DatabaseTasks instance;
			// Instantiated on first use
			return instance;
		}

		bool SetDatabaseInterface(Database *database);
		void start();
		void startThread();
		void flush();
		void shutdown();

		void addTask(std::string query, std::function<void(DBResult_ptr, bool)> callback = nullptr, bool store = false);

		void threadMain();
	private:
		void runTask(const DatabaseTask& task);

		Database *db_;
		std::thread thread;
		std::vector<DatabaseTask> tasks;
		std::mutex connection_lock;
		std::mutex task_lock;
		std::condition_variable_any task_signal;
		std::condition_variable_any flush_signal;
		std::atomic<bool> thread_state;
		std::once_flag init_flag;
		std::once_flag shutdown_flag;
		std::vector<std::future<DBResult_ptr>> task_handle_list;

};

constexpr auto g_databaseTasks = &DatabaseTasks::getInstance;

#endif  // SRC_DATABASE_DATABASETASKS_H_
