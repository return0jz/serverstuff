#pragma once
#include "server.hpp"
#include "sockhead.hpp"
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <functional>

namespace jzj {
class BaseServer {
public:
    BaseServer();
    ~BaseServer();
    /* Add a callback with the socket representing the endpoint of the client as a parameter. */
    /* Called every time a connection is made. */
    jzj::BaseServer &accept(std::function<void(int)> callback); 
    /* Run the server on a port*/
    void run(unsigned int port);

    jzj::BaseServer &backlog(int backlog);
    jzj::BaseServer &reuseAddr();
    jzj::BaseServer &useIPv4();
    jzj::BaseServer &useIPv6();
private:
    std::function<void(int)> m_acceptCallback;

    int m_backlog;
    bool m_reuseaddr;
    int m_addrFamily;
private:
    ::addrinfo *getServerAddrInfo(unsigned int port);
    int sockFromAddrInfo(addrinfo *serverAddr);
    void startListening(int listenfd);
};
}

jzj::BaseServer::BaseServer() {
    this->m_backlog = SOMAXCONN;
    this->m_reuseaddr = false;
    this->m_addrFamily = AF_UNSPEC;
}

jzj::BaseServer::~BaseServer() {}

void jzj::BaseServer::run(unsigned int port) {
    addrinfo *serverAddr = this->getServerAddrInfo(port);
    int listenfd = this->sockFromAddrInfo(serverAddr);
    if (listenfd<0) {
        exit(EXIT_FAILURE);
    }
    freeaddrinfo(serverAddr);
    
    this->startListening(listenfd);
}

jzj::BaseServer &jzj::BaseServer::accept(std::function<void(int)> callback) {
    this->m_acceptCallback = callback;
    return *this;
}

jzj::BaseServer &jzj::BaseServer::backlog(int backlog) {
    this->m_backlog = backlog;
    return *this;
}

jzj::BaseServer &jzj::BaseServer::useIPv4() {
    this->m_addrFamily = AF_INET;
    return *this;
}

jzj::BaseServer &jzj::BaseServer::useIPv6() {
    this->m_addrFamily = AF_INET6;
    return *this;
}

jzj::BaseServer &jzj::BaseServer::reuseAddr() {
    this->m_reuseaddr = true;
    return *this;
}

addrinfo *jzj::BaseServer::getServerAddrInfo(unsigned int port) {
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    // hints.ai_protocol
    addrinfo *serverAddr;
    int e = getaddrinfo(NULL, std::to_string(port).c_str(), &hints, &serverAddr);
    if (e!=0) {
        std::cerr << "jzjServer: Failed to get server address info: " << gai_strerror(e) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    return serverAddr;
}

int jzj::BaseServer::sockFromAddrInfo(addrinfo *serverAddr) {
    int sockfd=-1;
    for (addrinfo *p=serverAddr; p!=NULL;p=p->ai_next) {
        std::cout  
            << "jzjServer: Trying to create socket with the following:\n"
            << "\tAddress Family: " << p->ai_family << "\n"
            << "\tSocket Type: " << p->ai_socktype << "\n"
            << "\tProtocol: " << p->ai_protocol << std::endl;
        
        sockfd = socket(p->ai_family, p->ai_socktype,p->ai_protocol);
        if (sockfd>=0) {
            if (this->m_reuseaddr) {
                int optv = 1;
                if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optv, sizeof(optv))<0) {
                    std::cerr << "jzjServer: Failed to set socket option: " << std::strerror(errno) << std::endl;
                } 
            }
            std::string mesAddr(INET6_ADDRSTRLEN,0);
            if (p->ai_family == AF_INET) {
                inet_ntop(p->ai_family, &(( (sockaddr_in*) p->ai_addr)->sin_addr), &mesAddr[0],INET6_ADDRSTRLEN);
            } else {
                inet_ntop(p->ai_family, &(( (sockaddr_in6*) p->ai_addr)->sin6_addr), &mesAddr[0],INET6_ADDRSTRLEN);
            }
            std::cout 
                << "jzjServer: Trying to bind socket with the following:\n"
                << "\tAddress String: " << mesAddr << "\n";
                if (p->ai_family == AF_INET) {
                    std::cout << "\tPort Number: " << ntohs(((sockaddr_in*) p->ai_addr)->sin_port) << "\n";
                } else {
                    std::cout << "\tPort Number: " << ntohs(((sockaddr_in6*) p->ai_addr)->sin6_port) << "\n";
                }
            std::cout << std::endl; 
            if (bind(sockfd, p->ai_addr, p->ai_addrlen) >= 0) { 
                std::cout << "jzjServer: Successfully created socket." << std::endl;
                break;
            } else {
                close(sockfd);
                std::cerr << "jzjServer: Failed to bind socket: " << std::strerror(errno) << "\n";
                if (p) std::cout << "jzjServer: Trying again to create a new socket..." << std::endl;
                continue;
            }
        } else {
            std::cerr << "jzjServer: Failed to create socket: " << std::strerror(errno) << "\n";
            if (p) std::cout << "jzjServer: Trying again to create a new socket..." << std::endl;
        }
    }
    return sockfd;
}

void jzj::BaseServer::startListening(int listenfd) {
    if (listen(listenfd, this->m_backlog)<0) {
        std::cerr << "Failure to enable socket to listen for requests: " << std::strerror(errno) << std::endl;
        std::exit(EXIT_FAILURE);
    }
    while (true) {
        sockaddr_storage clientAddr;
        socklen_t clientAddrLen=sizeof(clientAddr);
        int clientSock = ::accept(listenfd, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock<0) {
            std::cerr << " jzjServer: Unable to accept connection from client:" << std::strerror(errno) << std::endl;
        } else {
            if (this->m_acceptCallback) {
                this->m_acceptCallback(clientSock);
            }
        }
    }
}