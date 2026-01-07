#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <memory>

#include "sqlite3.h" 
#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

struct Task {
    int id = 0;
    std::string title;
    std::string description;
    std::string status = "todo";
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Task, id, title, description, status)

class Database {
    sqlite3* db;
public:
    Database(const char* filename) {
        sqlite3_open(filename, &db);
        const char* sql = "CREATE TABLE IF NOT EXISTS tasks ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "title TEXT NOT NULL,"
            "description TEXT,"
            "status TEXT NOT NULL);";
        sqlite3_exec(db, sql, 0, 0, 0);
    }
    ~Database() { sqlite3_close(db); }

    void addTask(Task& t) {
        const char* sql = "INSERT INTO tasks (title, description, status) VALUES (?, ?, ?);";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, t.title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, t.description.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, t.status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        t.id = (int)sqlite3_last_insert_rowid(db);
        sqlite3_finalize(stmt);
    }

    std::vector<Task> getAll() {
        std::vector<Task> results;
        const char* sql = "SELECT id, title, description, status FROM tasks;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql, -1, &stmt, 0);
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            results.push_back({
                sqlite3_column_int(stmt, 0),
                (const char*)sqlite3_column_text(stmt, 1),
                (const char*)sqlite3_column_text(stmt, 2) ? (const char*)sqlite3_column_text(stmt, 2) : "",
                (const char*)sqlite3_column_text(stmt, 3)
                });
        }
        sqlite3_finalize(stmt);
        return results;
    }

    bool updateStatus(int id, std::string status) {
        std::string sql = "UPDATE tasks SET status = ? WHERE id = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 2, id);
        int res = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return res == SQLITE_DONE;
    }

    bool deleteTask(int id) {
        std::string sql = "DELETE FROM tasks WHERE id = ?;";
        sqlite3_stmt* stmt;
        sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0);
        sqlite3_bind_int(stmt, 1, id);
        int res = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        return res == SQLITE_DONE;
    }
};

void enable_cors(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

void log_request(const Request& req, const Response& res) {
    auto сейчас = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "[" << std::put_time(std::localtime(&сейчас), "%H:%M:%S") << "] "
        << req.method << " " << req.path << " -> " << res.status << std::endl;
}

int main() {
    Database db("todo_list.db");
    auto svr = std::make_unique<Server>(); 
    svr->set_logger(log_request);

    svr->Get("/", [](const Request&, Response& res) {
        std::ifstream f("index.html");
        if (f) {
            std::stringstream ss; ss << f.rdbuf();
            res.set_content(ss.str(), "text/html");
        }
        else {
            res.status = 404;
            res.set_content("index.html not found in current directory!", "text/plain");
        }
        enable_cors(res);
        });

    
    svr->Get("/tasks", [&](const Request&, Response& res) {
        res.set_content(json(db.getAll()).dump(), "application/json"); 
        enable_cors(res);
        });

    svr->Post("/tasks", [&](const Request& req, Response& res) {
        auto body = json::parse(req.body);
        Task t{ 0, body.at("title"), body.value("description", ""), "todo" };
        db.addTask(t);
        res.status = 201; 
        res.set_content(json(t).dump(), "application/json");
        enable_cors(res);
        });

    svr->Patch(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        auto body = json::parse(req.body);
        if (db.updateStatus(id, body.at("status"))) {
            res.status = 200; 
        }
        else {
            res.status = 404;
        }
        enable_cors(res);
        });

    svr->Delete(R"(/tasks/(\d+))", [&](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        db.deleteTask(id) ? res.status = 200 : res.status = 404;
        enable_cors(res);
        });

    svr->Options(R"(.*)", [](const Request&, Response& res) {
        enable_cors(res);
        res.status = 204;
        });

    std::cout << "Server started at ПМПММПМПМП http://localhost:8081" << std::endl;
    svr->listen("0.0.0.0", 8081);
    return 0;
}
