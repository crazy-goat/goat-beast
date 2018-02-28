//------------------------------------------------------------------------------
//
// Example: HTTP server
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio.hpp>
#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <iostream>

namespace ip = boost::asio::ip;         // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio.hpp>
namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

class http_worker {
public:
    http_worker(http_worker const &) = delete;

    http_worker &operator=(http_worker const &) = delete;

    http_worker(boost::asio::io_context &ioc, tcp::acceptor &acceptor, int num) :
        acceptor(acceptor),
        ioc(ioc),
        threadNumber(num),
        tcpSocket(acceptor.get_executor().context()),
        requestDeadline(acceptor.get_executor().context(), (std::chrono::steady_clock::time_point::max) ()) {
    }

    void start() {
        accept();
        check_deadline();
        ioc.run();
    }

private:
    int threadNumber = 0;

    // The acceptor used to listen for incoming connections.
    tcp::acceptor &acceptor;

    // The io context
    boost::asio::io_context &ioc;

    // The socket for the currently connected client.
    tcp::socket tcpSocket;

    // The buffer for performing reads
    boost::beast::flat_static_buffer<8192> buffer;

    // The timer putting a time limit on requests.
    boost::asio::basic_waitable_timer<std::chrono::steady_clock> requestDeadline;

    // The string-based response message.
    boost::optional<http::response<http::string_body>> response;

    // The string-based response serializer.
    boost::optional<http::response_serializer<http::string_body>> stringSerializer;

    http::request<http::string_body> req_;

    void accept() {
        // Clean up any previous connection.
        boost::beast::error_code ec;
        tcpSocket.close(ec);
        buffer.reset();
        acceptor.async_accept(
            tcpSocket,
            [this](boost::beast::error_code ec) {
                if (ec) {
                    accept();
                } else {
                    // Request must be fully processed within 60 seconds.
                    requestDeadline.expires_after(std::chrono::seconds(60));

                    read_request();
                }
            });
    }

    void read_request() {
        http::async_read(
            tcpSocket,
            buffer,
            req_,
            [this](boost::beast::error_code ec, std::size_t) {
                if (ec)
                    accept();
                else
                    process_request(req_);
            });
    }

    void process_request(http::request<http::string_body> const &req) {
        if (req.method() == http::verb::unknown) {
            sendResponse(
                http::status::bad_request,
                "Invalid request-method '" + req.method_string().to_string() + "'\r\n");
            return;
        }
        sendResponse(http::status::ok, "File not found on thread: " + std::to_string(threadNumber) + "\r\n");
    }

    void sendResponse(http::status status, std::string const &error) {
        response.emplace(std::piecewise_construct, std::make_tuple());

        response->result(status);
        response->keep_alive(true);
        response->content_length(error.size());
        response->set(http::field::server, "Beast");
        response->set(http::field::content_type, "text/plain");
        response->body() = error;
        response->prepare_payload();

        stringSerializer.emplace(*response);

        http::async_write(
            tcpSocket,
            *stringSerializer,
            [this](boost::beast::error_code ec, std::size_t) {
                tcpSocket.shutdown(tcp::socket::shutdown_send, ec);
                stringSerializer = boost::none;
                response = boost::none;;
                accept();
            });
    }

    void check_deadline() {
        // The deadline may have moved, so check it has really passed.
        if (requestDeadline.expiry() <= std::chrono::steady_clock::now()) {
            // Close socket to cancel any outstanding operation.
            boost::beast::error_code ec;
            tcpSocket.close();

            // Sleep indefinitely until we're given a new deadline.
            requestDeadline.expires_at(
                std::chrono::steady_clock::time_point::max());
        }

        requestDeadline.async_wait(
            [this](boost::beast::error_code) {
                check_deadline();
            });
    }
};

int main(int argc, char *argv[]) {
    try {
        // Check command line arguments.
        if (argc != 4) {
            std::cerr << "Usage: goat_zero <address> <port> <threads>" << std::endl;
            std::cerr << "goat_zero 0.0.0.0 80 8" << std::endl;
            return EXIT_FAILURE;
        }

        auto const address = boost::asio::ip::make_address(argv[1]);
        auto port = static_cast<unsigned short>(std::strtoul(argv[2], nullptr, 10));
        auto threads_num = static_cast<unsigned short>(std::strtoul(argv[3], nullptr, 10) + 1);

        boost::asio::io_context ioc{threads_num};
        tcp::acceptor acceptor{ioc, {address, port}};

        // Create a pool of threads to run all of the http workers.
        std::vector<boost::shared_ptr<boost::thread> > threads;
        for (auto i = 0; i < threads_num; ++i) {

            boost::shared_ptr<http_worker> worker = boost::make_shared<http_worker>(ioc, acceptor, i);
            boost::shared_ptr<boost::thread> thread(
                new boost::thread(
                    boost::bind(&http_worker::start, worker)
                )
            );
            threads.push_back(thread);
        }
        ioc.run();

        // Wait for all threads in the pool to exit.
        for (auto const thread:threads) thread->join();
    }
    catch (const std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}
