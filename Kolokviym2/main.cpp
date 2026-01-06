#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <sstream>

#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

struct Task 
{
    int id;
    std::string title;
    std::string description;
    std::string status;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Task, id, title, description, status)

std::vector<Task> tasks;
int next_id = 1;
std::mutex mtx;

// --- ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ---

void enable_cors(Response& res)
{
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

std::string read_html_file() {

    std::ifstream f("index.html");
    if (f.is_open()) {
        std::stringstream buffer;
        buffer << f.rdbuf();
        return buffer.str();
    }

    return "<h1>Error: index.html not found!</h1><p>Please put index.html near the executable.</p>";
}

int main() {
    Server svr;

    // 0. ГЛАВНАЯ СТРАНИЦА (Читаем из файла)
    svr.Get("/", [](const Request& req, Response& res) {
        std::string html = read_html_file();
        res.set_content(html, "text/html");
        });

    // 1. GET /tasks
    svr.Get("/tasks", [](const Request& req, Response& res) {
        std::lock_guard<std::mutex> lock(mtx);
        json response_json = tasks;
        res.set_content(response_json.dump(), "application/json");
        enable_cors(res);
        });

    // 2. POST /tasks
    svr.Post("/tasks", [](const Request& req, Response& res) {
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(mtx);
            Task t;
            t.id = next_id++;
            t.title = body.value("title", "No Title");
            t.description = body.value("description", "");
            t.status = body.value("status", "todo");
            tasks.push_back(t);

            res.status = 201;
            res.set_content(json(t).dump(), "application/json");
        }
        catch (...) {
            res.status = 400;
        }
        enable_cors(res);
        });

    // 3. DELETE /tasks/{id}
    svr.Delete(R"(/tasks/(\d+))", [](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        std::lock_guard<std::mutex> lock(mtx);
        auto it = std::remove_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            res.status = 200;
        }
        else {
            res.status = 404;
        }
        enable_cors(res);
        });

    // OPTIONS
    svr.Options(R"(.*)", [](const Request& req, Response& res) {
        enable_cors(res);
        res.status = 204;
        });

    std::cout << "Server started at http://localhost:8080" << std::endl;

    svr.listen("0.0.0.0", 8080);
}
