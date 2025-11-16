#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include "nlohmann/json.hpp"

namespace net = boost::asio;
namespace http = boost::beast::http;
using tcp = net::ip::tcp;
using json = nlohmann::json;

struct Location {
    double lat;
    double lng;
    std::string timestamp;
};

class LocationStore {
public:
    void update(const std::string& id, double lat, double lng, const std::string& timestamp);
    bool get(const std::string& id, Location& loc);

private:
    std::unordered_map<std::string, Location> data_;
    mutable std::shared_mutex mutex_;
};

std::string current_time();

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, std::shared_ptr<LocationStore> store);
    void start();

private:
    void read_request();
    void handle_request(http::request<http::string_body>& req);
    void write_response(http::response<http::string_body>&& res);

    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    std::shared_ptr<LocationStore> store_;
};
