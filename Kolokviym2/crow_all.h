#pragma once

#include <string>
#include <functional>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <regex>
#include <thread>
#include <mutex>
#include <chrono>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

namespace crow
{
    
    namespace json {
        class wvalue {
        private:
            enum Type { Null, String, Number, Object, Array, Boolean };
            Type type = Null;
            std::string str_val;
            double num_val;
            bool bool_val;
            std::unordered_map<std::string, wvalue> obj_val;
            std::vector<wvalue> arr_val;

        public:
            wvalue() = default;
            wvalue(const char* s) : type(String), str_val(s) {}
            wvalue(const std::string& s) : type(String), str_val(s) {}
            wvalue(int n) : type(Number), num_val((double)n) {}
            wvalue(double n) : type(Number), num_val(n) {}
            wvalue(bool b) : type(Boolean), bool_val(b) {}

            wvalue& operator[](const std::string& key) {
                type = Object;
                return obj_val[key];
            }

            wvalue& operator[](size_t index) {
                type = Array;
                if (index >= arr_val.size()) arr_val.resize(index + 1);
                return arr_val[index];
            }

            bool has(const std::string& key) const {
                return obj_val.find(key) != obj_val.end();
            }

            std::string s() const {
                if (type == String) return str_val;
                if (type == Number) return std::to_string((long long)num_val);
                if (type == Boolean) return bool_val ? "true" : "false";
                return "";
            }

            double d() const {
                if (type == Number) return num_val;
                if (type == String) try { return std::stod(str_val); }
                catch (...) {}
                return 0;
            }

            int i() const { return static_cast<int>(d()); }
            bool b() const { return type == Boolean ? bool_val : false; }

            std::string dump() const {
                switch (type) {
                case String: return "\"" + str_val + "\"";
                case Number: return std::to_string((long long)num_val);
                case Boolean: return bool_val ? "true" : "false";
                case Array: {
                    std::string result = "[";
                    for (size_t i = 0; i < arr_val.size(); i++) {
                        if (i > 0) result += ",";
                        result += arr_val[i].dump();
                    }
                    return result + "]";
                }
                case Object: {
                    std::string result = "{";
                    bool first = true;
                    for (const auto& item : obj_val) {
                        if (!first) result += ",";
                        first = false;
                        result += "\"" + item.first + "\":" + item.second.dump();
                    }
                    return result + "}";
                }
                default: return "null";
                }
            }
        };

        inline wvalue load(const std::string& str) {
            wvalue result;
            if (str.empty()) return result;

            std::string trimmed = str;
            trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
            trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

            if (trimmed[0] == '{' && trimmed.back() == '}') {
                
                std::string content = trimmed.substr(1, trimmed.size() - 2);
                size_t pos = 0;
                while (pos < content.size()) {
                    size_t key_start = content.find('"', pos);
                    if (key_start == std::string::npos) break;
                    size_t key_end = content.find('"', key_start + 1);
                    if (key_end == std::string::npos) break;

                    std::string key = content.substr(key_start + 1, key_end - key_start - 1);

                    size_t val_start = content.find(':', key_end + 1);
                    if (val_start == std::string::npos) break;

                    // Пропускаем пробелы
                    val_start = content.find_first_not_of(" \t\n\r", val_start + 1);
                    if (val_start == std::string::npos) break;

                    size_t val_end;
                    if (content[val_start] == '"') {
                        val_end = content.find('"', val_start + 1);
                        if (val_end == std::string::npos) break;
                        std::string value = content.substr(val_start + 1, val_end - val_start - 1);
                        result[key] = value;
                        pos = val_end + 1;
                    }
                    else if (std::isdigit(content[val_start]) || content[val_start] == '-') {
                        val_end = content.find_first_of(",}", val_start + 1);
                        if (val_end == std::string::npos) break;
                        std::string value = content.substr(val_start, val_end - val_start);
                        try {
                            result[key] = std::stod(value);
                        }
                        catch (...) {}
                        pos = val_end;
                    }
                    else {
                        break;
                    }
                }
            }
            return result;
        }
    }

    class response {
    public:
        std::string body;
        int code = 200;
        std::unordered_map<std::string, std::string> headers;

        response() = default;
        response(int c) : code(c) {}
        response(const std::string& b) : body(b) {}
        response(int c, const std::string& b) : code(c), body(b) {}

        void set_header(const std::string& key, const std::string& value) {
            headers[key] = value;
        }

        void write(const std::string& text) {
            body += text;
        }
    };

    class request {
    public:
        std::string url;
        std::string method;
        std::string body;
        std::unordered_map<std::string, std::string> headers;
        std::unordered_map<std::string, std::string> url_params;

        const std::string& get_header_value(const std::string& key) const {
            static std::string empty;
            auto it = headers.find(key);
            return it != headers.end() ? it->second : empty;
        }
    };

    class SimpleServer {
    private:
#ifdef _WIN32
        SOCKET server_socket;
        WSADATA wsa_data;
#else
        int server_socket;
#endif
        int port_;
        bool running = false;
        std::mutex server_mutex;

        std::unordered_map<std::string,
            std::function<void(const request&, response&)>> routes;
        std::unordered_map<std::string, std::string> route_methods;

        void init_socket() {
#ifdef _WIN32
            WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
            server_socket = socket(AF_INET, SOCK_STREAM, 0);

            sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_addr.s_addr = INADDR_ANY;
            server_addr.sin_port = htons(port_);

            int opt = 1;
#ifdef _WIN32
            setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
                (char*)&opt, sizeof(opt));
#else
            setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
#endif

            bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr));
            listen(server_socket, 10);
        }

        void close_socket() {
#ifdef _WIN32
            closesocket(server_socket);
            WSACleanup();
#else
            close(server_socket);
#endif
        }

        void handle_connection(int client_socket) {
            char buffer[4096] = { 0 };
            recv(client_socket, buffer, sizeof(buffer), 0);

            std::string request_str(buffer);
            std::istringstream iss(request_str);
            std::string line;

            request req;
            response res;

            if (std::getline(iss, line)) {
                std::istringstream first_line(line);
                first_line >> req.method >> req.url;
            }

            while (std::getline(iss, line) && line != "\r") {
                size_t colon = line.find(':');
                if (colon != std::string::npos) {
                    std::string key = line.substr(0, colon);
                    std::string value = line.substr(colon + 2); // +2 для ": "
                    if (!value.empty() && value.back() == '\r') value.pop_back();
                    req.headers[key] = value;
                }
            }

            size_t body_pos = request_str.find("\r\n\r\n");
            if (body_pos != std::string::npos) {
                req.body = request_str.substr(body_pos + 4);
            }

            auto it = routes.find(req.url);
            if (it != routes.end()) {
                if (route_methods[req.url] == req.method || route_methods[req.url].empty()) {
                    it->second(req, res);
                }
                else {
                    res.code = 405;
                    res.body = "Method Not Allowed";
                }
            }
            else {
                res.code = 404;
                res.body = "Not Found";
            }

            // Формируем ответ
            std::string response_str =
                "HTTP/1.1 " + std::to_string(res.code) + " OK\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: " + std::to_string(res.body.size()) + "\r\n";

            for (const auto& item : res.headers) {
                response_str += item.first + ": " + item.second + "\r\n";
            }

            response_str += "\r\n" + res.body;

            send(client_socket, response_str.c_str(), (int)response_str.size(), 0);

#ifdef _WIN32
            closesocket(client_socket);
#else
            close(client_socket);
#endif
        }

    public:
        SimpleServer(int port = 8080) : port_(port) {
            init_socket();
        }

        ~SimpleServer() {
            close_socket();
        }

        SimpleServer& port(int port) {
            port_ = port;
            return *this;
        }

        SimpleServer& multithreaded() {
            return *this;
        }

        void route(const std::string& path,
            const std::string& method,
            std::function<void(const request&, response&)> handler) {
            routes[path] = handler;
            route_methods[path] = method;
        }

        void run() {
            running = true;
            std::cout << "Server running on http://localhost:" << port_ << std::endl;

            while (running) {
                sockaddr_in client_addr;
#ifdef _WIN32
                int addr_len = sizeof(client_addr);
                SOCKET client_socket = accept(server_socket,
                    (sockaddr*)&client_addr,
                    &addr_len);
#else
                socklen_t addr_len = sizeof(client_addr);
                int client_socket = accept(server_socket,
                    (sockaddr*)&client_addr,
                    &addr_len);
#endif

                if (client_socket > 0) {
                    std::thread([this, client_socket]() {
                        handle_connection((int)client_socket);
                        }).detach();
                }
            }
        }

        void stop() {
            running = false;
        }
    };

    class SimpleApp : public SimpleServer {
    public:
        SimpleApp() : SimpleServer(8080) {}
    };

    template<typename App>
    class RouteBuilder {
    private:
        App& app;
        std::string path;
        std::string method;

    public:
        RouteBuilder(App& a, const std::string& p) : app(a), path(p), method("GET") {}

        RouteBuilder& methods(const std::string& m) {
            method = m;
            return *this;
        }

        void operator()(std::function<void(const request&, response&)> handler) {
            app.route(path, method, handler);
        }

        template<typename Func>
        void operator()(Func handler) {
            app.route(path, method,
                [handler](const request& req, response& res) {
                    handler(req, res);
                });
        }
    };

    template<typename App>
    auto CROW_ROUTE(App& app, const std::string& path) {
        return RouteBuilder<App>(app, path);
    }
}
using crow::SimpleApp;
using crow::request;
using crow::response;
using crow::json::wvalue;
using crow::json::load;