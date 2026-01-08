#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable : 26495)

#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <memory>
#include <atomic>
#include <mutex> 

#include "sqlite3.h"
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

struct Task
{
    int id = 0;
    std::string title;
    std::string description;
    std::string status = "todo";
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Task, id, title, description, status)

std::atomic<int> total_requests{ 0 };

std::string get_safe_text(sqlite3_stmt* stmt, int col) {
    const char* text = (const char*)sqlite3_column_text(stmt, col);
    return text ? std::string(text) : std::string("");
}

class Database
{
    sqlite3* db;
    std::mutex mtx;

public:
    Database(const char* filename)
    {
        sqlite3_open(filename, &db);
        const char* sql = "CREATE TABLE IF NOT EXISTS tasks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "description TEXT,"
            "status TEXT NOT NULL);";
        sqlite3_exec(db, sql, 0, 0, 0);
    }
    ~Database() { sqlite3_close(db); }

    void addTask(Task& t)
    {
        std::lock_guard<std::mutex> lock(mtx);
        const char* sql = "INSERT INTO tasks (title, description, status) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;

        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
            std::cerr << "SQL Error: " << sqlite3_errmsg(db) << std::endl;
            return;
        }

        sqlite3_bind_text(stmt, 1, t.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, t.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, t.status.c_str(), -1, SQLITE_TRANSIENT);

        sqlite3_step(stmt);
        t.id = (int)sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
    }

    std::vector<Task> getAll()
    {
        std::lock_guard<std::mutex> lock(mtx);
        std::vector<Task> results;
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "SELECT id, title, description, status FROM tasks;", -1, &stmt, 0) != SQLITE_OK) {
            return results;
        }
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            results.push_back({
                sqlite3_column_int(stmt, 0),
                get_safe_text(stmt, 1),
                get_safe_text(stmt, 2),
                get_safe_text(stmt, 3)
                });
        }
        sqlite3_finalize(stmt);
        return results;
    }

    std::pair<bool, Task> getOne(int id) {
        std::lock_guard<std::mutex> lock(mtx);
        sqlite3_stmt* stmt;
        Task t;
        bool found = false;

        if (sqlite3_prepare_v2(db, "SELECT id, title, description, status FROM tasks WHERE id = ?;", -1, &stmt, 0) == SQLITE_OK) {
            sqlite3_bind_int(stmt, 1, id);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                t.id = sqlite3_column_int(stmt, 0);
                t.title = get_safe_text(stmt, 1);
                t.description = get_safe_text(stmt, 2);
                t.status = get_safe_text(stmt, 3);
                found = true;
            }
            sqlite3_finalize(stmt);
        }
        return { found, t };
    }

    bool updateStatus(int id, std::string status)
    {
        std::lock_guard<std::mutex> lock(mtx);
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "UPDATE tasks SET status = ? WHERE id = ?;", -1, &stmt, 0) != SQLITE_OK) return false;

        sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, id);
        sqlite3_step(stmt);
        bool changed = sqlite3_changes(db) > 0;
        sqlite3_finalize(stmt);
        return changed;
    }

    bool updateFull(int id, const Task& t) {
        std::lock_guard<std::mutex> lock(mtx);
        sqlite3_stmt* stmt;
        const char* sql = "UPDATE tasks SET title = ?, description = ?, status = ? WHERE id = ?;";
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) return false;

        sqlite3_bind_text(stmt, 1, t.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, t.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, t.status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 4, id);
        sqlite3_step(stmt);
        bool changed = sqlite3_changes(db) > 0;
        sqlite3_finalize(stmt);
        return changed;
    }

    bool deleteTask(int id)
    {
        std::lock_guard<std::mutex> lock(mtx);
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, "DELETE FROM tasks WHERE id = ?;", -1, &stmt, 0) != SQLITE_OK) return false;

        sqlite3_bind_int(stmt, 1, id);
        sqlite3_step(stmt);
        bool deleted = sqlite3_changes(db) > 0;
        sqlite3_finalize(stmt);
        return deleted;
    }
};

void logger(const Request& req, const Response& res)
{
    total_requests++;
    auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "[" << std::put_time(std::localtime(&t), "%H:%M:%S") << "] "
        << req.method << " " << req.path << " -> " << res.status << std::endl;
}

int main() {
    system("chcp 65001");
    Database db("todo_list.db");

    auto svr = std::make_unique<Server>();
    svr->set_logger(logger);

    auto enable_cors = [](Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, X-Auth-Token");
        };

    svr->Get("/", [](const Request&, Response& res) {
        std::ifstream f("index.html");
        if (f) {
            std::stringstream ss; ss << f.rdbuf();
            res.set_content(ss.str(), "text/html");
        }
        else {
            res.status = 404;
            res.set_content("Error: index.html not found", "text/plain");
        }
        });

    svr->Get("/metrics", [&](const Request&, Response& res) {
        enable_cors(res); // Сначала CORS
        json m;
        m["total_calls"] = (int)total_requests;
        m["db_size"] = db.getAll().size();
        res.set_content(m.dump(4), "application/json");
        });

    svr->Get("/tasks", [&](const Request&, Response& res) {
        enable_cors(res);
        auto tasks = db.getAll();
        res.set_content(json(tasks).dump(), "application/json");
        });

    svr->Get(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        enable_cors(res);
        int id = std::stoi(req.matches[1]);
        auto result = db.getOne(id);
        if (result.first) {
            res.set_content(json(result.second).dump(), "application/json");
        }
        else {
            res.status = 404;
            res.set_content("{}", "application/json");
        }
        });

    svr->Post("/tasks", [&](const Request& req, Response& res) {
        enable_cors(res); 
        try {
            auto body = json::parse(req.body);

            std::string title;
            if (body.contains("title") && body["title"].is_string()) {
                title = body["title"].get<std::string>();
            }

            if (title.empty()) {
                res.status = 400;
                res.set_content("{\"error\": \"Title is empty\"}", "application/json");
                return;
            }

            std::string desc = "";
            if (body.contains("description") && body["description"].is_string()) {
                desc = body["description"].get<std::string>();
            }

            Task t{ 0, title, desc, "todo" };
            db.addTask(t);

            res.status = 201;
            res.set_content(json(t).dump(), "application/json");
        }
        catch (const std::exception& e) {
            res.status = 400;
            std::cerr << "JSON Error: " << e.what() << std::endl; 
            res.set_content("{\"error\": \"Invalid JSON\"}", "application/json");
        }
        });

    svr->Put(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        enable_cors(res);
        int id = std::stoi(req.matches[1]);
        try {
            auto body = json::parse(req.body);
            Task t;

            if (!body.contains("title") || !body["title"].is_string()) throw std::runtime_error("Invalid title");

            t.title = body["title"].get<std::string>();
            t.description = body.contains("description") && body["description"].is_string() ? body["description"].get<std::string>() : "";
            t.status = body.contains("status") && body["status"].is_string() ? body["status"].get<std::string>() : "todo";

            if (db.updateFull(id, t)) {
                t.id = id;
                res.status = 200;
                res.set_content(json(t).dump(), "application/json");
            }
            else {
                res.status = 404;
            }
        }
        catch (...) { res.status = 400; }
        });

    svr->Patch(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        enable_cors(res);
        int id = std::stoi(req.matches[1]);
        try {
            auto body = json::parse(req.body);
            if (body.contains("status") && body["status"].is_string()) {
                if (db.updateStatus(id, body["status"].get<std::string>())) {
                    res.status = 200;
                    res.set_content("{\"status\": \"updated\"}", "application/json");
                }
                else {
                    res.status = 404;
                }
            }
            else {
                res.status = 400;
            }
        }
        catch (...) { res.status = 400; }
        });

    svr->Delete(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        enable_cors(res);
        int id = std::stoi(req.matches[1]);
        if (db.deleteTask(id)) {
            res.status = 200;
        }
        else {
            res.status = 404;
        }
        });

    svr->Options(R"(.*)", [&](const Request&, Response& res) {
        enable_cors(res);
        res.status = 204;
        });

    std::cout << "Сервер запущен: http://localhost:8081" << std::endl;
    svr->listen("0.0.0.0", 8081);
    return 0;
}
