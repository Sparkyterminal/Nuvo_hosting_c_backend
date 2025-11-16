#include "locationTracker.h"
#include <iostream>

int main() {
    try {
        net::io_context ioc;
        tcp::acceptor acceptor(ioc, { tcp::v4(), 9090 });
        auto store = std::make_shared<LocationStore>();

        std::cout << "Server running on http://localhost:9090\n";

        std::function<void()> do_accept;
        do_accept = [&]() {
            acceptor.async_accept([&](boost::system::error_code ec, tcp::socket socket) {
                if (!ec)
                    std::make_shared<Session>(std::move(socket), store)->start();
                do_accept();
            });
        };

        do_accept();
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
    }
}
