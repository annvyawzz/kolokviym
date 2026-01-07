#define _CRT_SECURE_NO_WARNINGS // Убирает ошибку localtime
#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <memory> // Для std::unique_ptr (решение проблемы со стеком)

#include "httplib.h"
#include "json.hpp"

using json = nlohmann::json;
using namespace httplib;

// 2. Ресурсы: Задача (Task) [cite: 6, 7]
struct Task {
    int id = 0; // Инициализация (решает ошибку type.6)
    std::string title;
    std::string description;
    std::string status = "todo"; // По умолчанию "todo" 
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Task, id, title, description, status)

// Глобальное хранилище (Программа минимум: без БД) [cite: 52]
std::vector<Task> tasks;
int next_id = 1;
std::mutex mtx;

void log_request(const Request& req, const Response& res) {
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::cout << "[" << std::put_time(std::localtime(&now), "%H:%M:%S") << "] "
        << req.method << " " << req.path << " -> " << res.status << std::endl;
}

void enable_cors(Response& res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, PATCH, DELETE, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
}

std::string read_html_file() {
    std::ifstream f("index.html");
    if (f.is_open()) {
        std::stringstream buffer;
        buffer << f.rdbuf();
        return buffer.str();
    }
    return "<h1>index.html not found!</h1>";
}

int main() {
    // Перемещаем сервер в кучу (Heap), чтобы избежать ошибок переполнения стека (16480 байт)
    auto svr = std::make_unique<Server>();

    // Главная страница
    svr->Get("/", [](const Request& req, Response& res) {
        res.set_content(read_html_file(), "text/html");
        enable_cors(res);
        });

    // 1. GET /tasks - Список всех задач [cite: 9]
    svr->Get("/tasks", [](const Request& req, Response& res) {
        std::lock_guard<std::mutex> lock(mtx);
        res.status = 200;
        res.set_content(json(tasks).dump(), "application/json"); // Данные в JSON [cite: 50]
        enable_cors(res);
        log_request(req, res);
        });

    // 2. POST /tasks - Создать задачу [cite: 10, 22]
    svr->Post("/tasks", [](const Request& req, Response& res) {
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(mtx);
            Task t;
            t.id = next_id++;
            t.title = body.at("title").get<std::string>();
            t.description = body.value("description", "");
            t.status = body.value("status", "todo");
            tasks.push_back(t);
            res.status = 201; // 201 Created [cite: 28, 49]
            res.set_content(json(t).dump(), "application/json");
        }
        catch (...) { res.status = 400; }
        enable_cors(res);
        log_request(req, res);
        });

    // 3. GET /tasks/{id} - Одна задача [cite: 11]
    svr->Get(R"(/tasks/(\d+))", [](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        std::lock_guard<std::mutex> lock(mtx);
        auto it = std::find_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
        if (it != tasks.end()) {
            res.set_content(json(*it).dump(), "application/json");
        }
        else { res.status = 404; } // 404 Not Found [cite: 49]
        enable_cors(res);
        log_request(req, res);
        });

    // 4. PUT /tasks/{id} - Полное обновление [cite: 12]
    svr->Put(R"(/tasks/(\d+))", [](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(mtx);
            auto it = std::find_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
            if (it != tasks.end()) {
                it->title = body.at("title");
                it->description = body.at("description");
                it->status = body.at("status");
                res.set_content(json(*it).dump(), "application/json");
            }
            else { res.status = 404; }
        }
        catch (...) { res.status = 400; }
        enable_cors(res);
        log_request(req, res);
        });

    // 5. PATCH /tasks/{id} - Обновление статуса [cite: 36]
    svr->Patch(R"(/tasks/(\d+))", [](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        try {
            auto body = json::parse(req.body);
            std::lock_guard<std::mutex> lock(mtx);
            auto it = std::find_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
            if (it != tasks.end()) {
                if (body.contains("status")) it->status = body["status"];
                res.status = 200;
                res.set_content(json(*it).dump(), "application/json");
            }
            else { res.status = 404; }
        }
        catch (...) { res.status = 400; }
        enable_cors(res);
        log_request(req, res);
        });

    // 6. DELETE /tasks/{id} - Удаление [cite: 13]
    svr->Delete(R"(/tasks/(\d+))", [](const Request& req, Response& res) {
        int id = std::stoi(req.matches[1]);
        std::lock_guard<std::mutex> lock(mtx);
        auto it = std::remove_if(tasks.begin(), tasks.end(), [id](const Task& t) { return t.id == id; });
        if (it != tasks.end()) {
            tasks.erase(it, tasks.end());
            res.status = 200;
        }
        else { res.status = 404; }
        enable_cors(res);
        log_request(req, res);
        });

    svr->Options(R"(.*)", [](const Request& req, Response& res) {
        enable_cors(res);
        res.status = 204;
        });

    std::cout << "Server started at http://localhost:8080" << std::endl;
    svr->listen("0.0.0.0", 8080);
    return 0;
}
