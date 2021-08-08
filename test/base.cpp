#include <exception>
#include "server.hpp"

// base functionality

int main(int argc, char const *argv[]) {
    jzj::BaseServer server;
    server.accept([](int clientSock){
        std::string req(10000, '\0');
        read(clientSock, &req[0], req.size());
        std::cout << req << std::endl;
        std::string res = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html\r\n\r\n"
        "<html><body><h1> Test </h1></body></html>";
        write(clientSock, &res[0], res.size());
        close(clientSock);
    }).run(3000);
    return 0;
}