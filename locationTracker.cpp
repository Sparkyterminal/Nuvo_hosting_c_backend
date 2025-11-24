#include "locationTracker.h"
#include <iostream>
#include <ctime>

// ============ LocationStore ============

void LocationStore::update(const std::string& id, double lat, double lng, const std::string& timestamp) {
    std::unique_lock lock(mutex_);
    data_[id] = { lat, lng, timestamp };
}

bool LocationStore::get(const std::string& id, Location& loc) {
    std::shared_lock lock(mutex_);
    auto it = data_.find(id);
    if (it == data_.end()) return false;
    loc = it->second;
    return true;
}

std::string current_time() {
    std::time_t t = std::time(nullptr);
    char buf[64];
    std::strftime(buf, sizeof(buf), "%FT%TZ", std::gmtime(&t));
    return buf;
}

// ============ Session ============

Session::Session(tcp::socket socket, std::shared_ptr<LocationStore> store)
    : socket_(std::move(socket)), store_(std::move(store)) {}

void Session::start() {
    read_request();
}

void Session::read_request() {
    auto self = shared_from_this();
    auto req = std::make_shared<http::request<http::string_body>>();

    http::async_read(socket_, buffer_, *req,
        [this, self, req](boost::system::error_code ec, std::size_t) {
            if (!ec) handle_request(*req);
        });
}

void Session::handle_request(http::request<http::string_body>& req) {
    http::response<http::string_body> res{ http::status::ok, req.version() };
    res.set(http::field::content_type, "application/json");

    try {
        if (req.method() == http::verb::post && req.target() == "/api/location/update") {
            auto j = json::parse(req.body());
            std::string id = j["Employee"];
            double lat = j["lat"];
            double lng = j["lng"];

            store_->update(id, lat, lng, current_time());
            res.body() = R"({"status":"ok"})";

        } else if (req.method() == http::verb::get) {
            std::string target = std::string(req.target());
            std::string prefix = "/api/location/";
            if (target.rfind(prefix, 0) == 0) {
                std::string id = target.substr(prefix.size());
                Location loc;
                if (store_->get(id, loc)) {
                    json j = {
                        {"id", id},
                        {"lat", loc.lat},
                        {"lng", loc.lng},
                        {"timestamp", loc.timestamp}
                    };
                    res.body() = j.dump();
                } else {
                    res.result(http::status::not_found);
                    res.body() = R"({"error":"not found"})";
                }
            } else {
                res.result(http::status::not_found);
                res.body() = R"({"error":"invalid endpoint"})";
            }
        } else {
            res.result(http::status::not_found);
            res.body() = R"({"error":"invalid endpoint"})";
        }
    } catch (const std::exception& e) {
        res.result(http::status::bad_request);
        res.body() = std::string("{\"error\":\"") + e.what() + "\"}";
    }

    res.prepare_payload();
    write_response(std::move(res));
}

void Session::write_response(http::response<http::string_body>&& res) {
    auto self = shared_from_this();
    auto sp = std::make_shared<http::response<http::string_body>>(std::move(res));
    http::async_write(socket_, *sp, [this, self, sp](boost::system::error_code ec, std::size_t) {
        socket_.shutdown(tcp::socket::shutdown_send, ec);
    });
}
