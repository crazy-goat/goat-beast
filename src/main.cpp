//------------------------------------------------------------------------------
//
// Example: HTTP server
//
//------------------------------------------------------------------------------

#include "HttpWorker.h"

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

            auto worker = boost::make_shared<HttpWorker>(ioc, acceptor, i);
            boost::shared_ptr<boost::thread> thread(
                new boost::thread(
                    boost::bind(&HttpWorker::start, worker)
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
