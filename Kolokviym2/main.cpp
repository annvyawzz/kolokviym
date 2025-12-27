#include "crow_all.h"
#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#define _CRT_SECURE_NO_WARNINGS
#endif

// ============================ Логирование (пункт 5) ============================
class Logger {
private:
    std::mutex log_mutex;

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

#ifdef _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &in_time_t);
#else
        struct tm timeinfo;
        localtime_r(&in_time_t, &timeinfo);
#endif

        std::stringstream ss;
        ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    void info(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "[" << getCurrentTime() << "] INFO: " << message << std::endl;
    }

    void error(const std::string& message) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cerr << "[" << getCurrentTime() << "] ERROR: " << message << std::endl;
    }

    void request(const std::string& method, const std::string& endpoint, int status_code) {
        std::lock_guard<std::mutex> lock(log_mutex);
        std::cout << "[" << getCurrentTime() << "] REQUEST: "
            << method << " " << endpoint
            << " -> " << status_code << std::endl;
    }
};

Logger logger;

// ============================ Структура задачи ============================
struct Task {
    int id;
    std::string title;
    std::string description;
    std::string status; // "todo", "in_progress", "done"
    std::string created_at;
    std::string updated_at;

    // Конвертация в JSON (для ответов)
    crow::json::wvalue to_json() const {
        crow::json::wvalue json;
        json["id"] = id;
        json["title"] = title;
        json["description"] = description;
        json["status"] = status;
        json["created_at"] = created_at;
        json["updated_at"] = updated_at;
        return json;
    }
};

// ============================ Кэширование (пункт 3) ============================
template<typename T>
class Cache {
private:
    std::map<std::string, T> cache;
    std::mutex cache_mutex;
    std::chrono::seconds ttl; // Time To Live

public:
    Cache(int ttl_seconds = 60) : ttl(ttl_seconds) {}

    void set(const std::string& key, const T& value) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache[key] = value;
    }

    bool get(const std::string& key, T& value) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        auto it = cache.find(key);
        if (it != cache.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    void remove(const std::string& key) {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache.erase(key);
    }

    void clear() {
        std::lock_guard<std::mutex> lock(cache_mutex);
        cache.clear();
    }
};

// Глобальный кэш для GET запросов
Cache<std::string> response_cache(30); // 30 секунд хранения

// ============================ Менеджер задач ============================
class TaskManager {
private:
    std::vector<Task> tasks;
    int next_id = 1;
    std::mutex tasks_mutex;

    std::string getCurrentTime() {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);

#ifdef _WIN32
        struct tm timeinfo;
        localtime_s(&timeinfo, &in_time_t);
#else
        struct tm timeinfo;
        localtime_r(&in_time_t, &timeinfo);
#endif

        std::stringstream ss;
        ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    // Создать задачу
    Task create_task(const std::string& title, const std::string& description, const std::string& status) {
        std::lock_guard<std::mutex> lock(tasks_mutex);

        Task task;
        task.id = next_id++;
        task.title = title;
        task.description = description;
        task.status = status;
        task.created_at = getCurrentTime();
        task.updated_at = task.created_at;

        tasks.push_back(task);

        // Очищаем кэш при изменении данных
        response_cache.clear();

        logger.info("Created task #" + std::to_string(task.id) + ": " + title);
        return task;
    }

    // Получить все задачи
    std::vector<Task> get_all_tasks() {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        return tasks;
    }

    // Получить задачу по ID
    Task* get_task_by_id(int id) {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        for (auto& task : tasks) {
            if (task.id == id) {
                return &task;
            }
        }
        return nullptr;
    }

    // Обновить задачу (полностью)
    bool update_task(int id, const std::string& title, const std::string& description, const std::string& status) {
        std::lock_guard<std::mutex> lock(tasks_mutex);

        for (auto& task : tasks) {
            if (task.id == id) {
                task.title = title;
                task.description = description;
                task.status = status;
                task.updated_at = getCurrentTime();

                // Очищаем кэш при изменении данных
                response_cache.clear();

                logger.info("Updated task #" + std::to_string(id));
                return true;
            }
        }
        return false;
    }

    // Обновить только статус (PATCH)
    bool update_task_status(int id, const std::string& status) {
        std::lock_guard<std::mutex> lock(tasks_mutex);

        for (auto& task : tasks) {
            if (task.id == id) {
                task.status = status;
                task.updated_at = getCurrentTime();

                // Очищаем кэш при изменении данных
                response_cache.clear();

                logger.info("Updated task #" + std::to_string(id) + " status to: " + status);
                return true;
            }
        }
        return false;
    }

    // Удалить задачу
    bool delete_task(int id) {
        std::lock_guard<std::mutex> lock(tasks_mutex);

        auto it = std::remove_if(tasks.begin(), tasks.end(),
            [id](const Task& task) { return task.id == id; });

        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());

            // Очищаем кэш при изменении данных
            response_cache.clear();

            logger.info("Deleted task #" + std::to_string(id));
            return true;
        }
        return false;
    }

    // Получить статистику
    std::map<std::string, int> get_stats() {
        std::lock_guard<std::mutex> lock(tasks_mutex);

        std::map<std::string, int> stats;
        for (const auto& task : tasks) {
            stats[task.status]++;
        }
        stats["total"] = (int)tasks.size();

        return stats;
    }
};

int main() {
#ifdef _WIN32
    // Устанавливаем UTF-8 кодировку для Windows
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    logger.info("Starting To-Do List API Server...");

    TaskManager task_manager;
    task_manager.create_task("Buy milk", "3.2% fat", "todo");
    task_manager.create_task("Launch API", "Setup and test REST API", "in_progress");

    crow::SimpleApp app;

    // GET / - Homepage with API documentation
    CROW_ROUTE(app, "/")([](const crow::request& req, crow::response& res) {
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.write(R"({
    "service": "To-Do List API",
    "version": "1.0",
    "endpoints": [
        "GET /tasks - Get all tasks",
        "POST /tasks - Create new task",
        "GET /tasks/<id> - Get task by ID",
        "PUT /tasks/<id> - Update task",
        "DELETE /tasks/<id> - Delete task",
        "GET /stats - Get statistics"
    ],
    "example_request": {
        "POST /tasks": {
            "title": "Task title",
            "description": "Task description",
            "status": "todo|in_progress|done"
        }
    }
})");
        });

    // GET /tasks - Get all tasks
    CROW_ROUTE(app, "/tasks")
        .methods("GET")
        ([&task_manager](const crow::request& req, crow::response& res) {
        logger.request("GET", "/tasks", 200);

        std::string cached_response;
        if (response_cache.get("/tasks", cached_response)) {
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.set_header("X-Cache", "HIT");
            res.write(cached_response);
            return;
        }

        auto tasks = task_manager.get_all_tasks();
        crow::json::wvalue json;
        for (size_t i = 0; i < tasks.size(); i++) {
            json[(int)i]["id"] = tasks[i].id;
            json[(int)i]["title"] = tasks[i].title;
            json[(int)i]["status"] = tasks[i].status;
        }

        std::string response_str = json.dump();
        response_cache.set("/tasks", response_str);

        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.set_header("X-Cache", "MISS");
        res.write(response_str);
            });

    // POST /tasks - Create new task
    CROW_ROUTE(app, "/tasks")
        .methods("POST")
        ([&task_manager](const crow::request& req, crow::response& res) {
        logger.request("POST", "/tasks", 201);

        try {
            auto json = crow::json::load(req.body);
            if (!json.has("title")) {
                res.code = 400;
                res.write(R"({"error": "Title is required", "code": 400})");
                return;
            }

            std::string title = json["title"].s();
            std::string description = json.has("description") ? json["description"].s() : "";
            std::string status = json.has("status") ? json["status"].s() : "todo";

            // Validate status
            if (status != "todo" && status != "in_progress" && status != "done") {
                status = "todo";
            }

            Task task = task_manager.create_task(title, description, status);
            res.code = 201;
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.write(task.to_json().dump());
        }
        catch (...) {
            res.code = 500;
            res.write(R"({"error": "Internal server error", "code": 500})");
        }
            });

    // GET /tasks/1 - Get task by ID (simplified)
    CROW_ROUTE(app, "/tasks/1")
        .methods("GET")
        ([&task_manager](const crow::request& req, crow::response& res) {
        auto task = task_manager.get_task_by_id(1);
        if (task) {
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.write(task->to_json().dump());
        }
        else {
            res.code = 404;
            res.write(R"({"error": "Task not found", "code": 404})");
        }
            });

    CROW_ROUTE(app, "/tasks/2")
        .methods("GET")
        ([&task_manager](const crow::request& req, crow::response& res) {
        auto task = task_manager.get_task_by_id(2);
        if (task) {
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.write(task->to_json().dump());
        }
        else {
            res.code = 404;
            res.write(R"({"error": "Task not found", "code": 404})");
        }
            });

    // DELETE /tasks/1 - Delete task
    CROW_ROUTE(app, "/tasks/1")
        .methods("DELETE")
        ([&task_manager](const crow::request& req, crow::response& res) {
        if (task_manager.delete_task(1)) {
            res.set_header("Content-Type", "application/json; charset=utf-8");
            res.write(R"({"message": "Task deleted successfully", "id": 1})");
        }
        else {
            res.code = 404;
            res.write(R"({"error": "Task not found", "code": 404})");
        }
            });

    // GET /stats - Get statistics
    CROW_ROUTE(app, "/stats")
        .methods("GET")
        ([&task_manager](const crow::request& req, crow::response& res) {
        auto stats = task_manager.get_stats();
        crow::json::wvalue json;
        for (const auto& item : stats) {
            json[item.first] = item.second;
        }
        res.set_header("Content-Type", "application/json; charset=utf-8");
        res.write(json.dump());
            });

    // PUT /tasks/update - Update task (alternative to PUT /tasks/{id})
    CROW_ROUTE(app, "/tasks/update")
        .methods("POST")
        ([&task_manager](const crow::request& req, crow::response& res) {
        logger.request("POST", "/tasks/update", 200);

        try {
            auto json = crow::json::load(req.body);
            if (!json.has("id")) {
                res.code = 400;
                res.write(R"({"error": "Task ID is required", "code": 400})");
                return;
            }

            int id = json["id"].i();
            std::string title = json.has("title") ? json["title"].s() : "";
            std::string description = json.has("description") ? json["description"].s() : "";
            std::string status = json.has("status") ? json["status"].s() : "";

            if (task_manager.update_task(id, title, description, status)) {
                auto task = task_manager.get_task_by_id(id);
                res.set_header("Content-Type", "application/json; charset=utf-8");
                res.write(task->to_json().dump());
            }
            else {
                res.code = 404;
                res.write(R"({"error": "Task not found", "code": 404})");
            }
        }
        catch (...) {
            res.code = 400;
            res.write(R"({"error": "Bad request", "code": 400})");
        }
            });

    logger.info("Server running on http://127.0.0.1:8080");
    logger.info("Open browser and go to: http://127.0.0.1:8080");
    logger.info("Use Postman or curl for testing");
    logger.info("Example: curl -X GET http://127.0.0.1:8080/tasks");

    app.port(8080).multithreaded().run();

    return 0;
}