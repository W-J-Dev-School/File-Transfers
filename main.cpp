#include <iostream>
#include <fstream>
#include <thread>
#include "httplib.h"

void server_save_file_handler(
    const httplib::Request &req,
    httplib::Response &res,
    const httplib::ContentReader &content_reader)
{
    assert(!req.is_multipart_form_data() && "Multipart is not supported!");

    std::fstream file;
    file.open("out.dat", std::ios::binary | std::ios::out | std::ios::trunc);
    assert(file.is_open() && "Output file failed to open!");

    content_reader([&](const char *data, size_t data_length) {
        std::cout << "[SERVER]"
                  << "Received " << data_length << " bytes" << std::endl;

        file.write(data, data_length);
        return true;
    });
    file.close();

    std::cout << "[SERVER]"
              << "Receiving done" << std::endl;

    res.set_content("Great!", "text/plain");
}

void server_proc()
{
    std::cout << "Server started" << std::endl;

    httplib::Server http_server;
    http_server.set_payload_max_length(1024 * 1024 * 512); // 512 MB
    http_server.Post("/", server_save_file_handler);
    http_server.listen("localhost", 3000);
}

void client_proc()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // Let the server start first

    std::cout << "Client started" << std::endl;

    httplib::Client http_client("http://localhost:3000");

    // Send file

    std::fstream file;
    file.open("in.dat", std::ios::binary | std::ios::in | std::ios::ate);
    assert(file.is_open() && "Input file failed to open!");

    size_t file_size = file.tellg();
    file.seekg(0);

    httplib::Result response = http_client.Post(
        "/", [&](size_t offset, httplib::DataSink &sink) {
            char data[8192] = {0};

            file.seekg(offset);
            file.read(data, sizeof(data));

            assert(!file.bad() && "File is bad!");

            size_t data_length = file.gcount();

            std::cout << "[CLIENT]"
                      << "Sending " << data_length << " bytes" << std::endl;

            sink.write(data, data_length);

            if (file.eof())
            {
                std::cout << "[CLIENT]"
                          << "Sending done" << std::endl;
                sink.done();
            }

            return true;
        },
        "application/octet-stream");

    file.close();

    std::cout << "[CLIENT]"
              << "Response: " << response.value().body << std::endl;
}

int main()
{
    std::thread server(server_proc);
    std::thread client(client_proc);

    server.join();
    client.join();
}