// Marcelina Oset 313940

#include <arpa/inet.h>
#include <cassert>
#include <cctype>
#include <dirent.h>
#include <errno.h>
#include <filesystem>
#include <iostream>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <fstream>

void check_port(std::string port) {
    for (unsigned int i = 0; i < port.size(); i++) {
        if (isdigit(port[i]) == 0) {
            fprintf(stderr, "Invalid argument! Port should be a number\n");
            exit(EXIT_FAILURE);
        }
    }
}

void check_dir(std::string directory) {
    DIR *dir = opendir(directory.c_str());
    if (dir) {
        closedir(dir);
    } else if (ENOENT == errno) {
        fprintf(stderr, "directory path error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    } else {
        fprintf(stderr, "directory path error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

int create_socket_and_bind_port(int port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "bind failure: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    return sockfd;
}

int check_get_request(std::string buffer) {
    std::stringstream ss(buffer);
    std::string get, page;
    ss >> get;
    if (get != "GET") {
        return 0;
    }

    if (!(ss >> page)) {
        return 0;
    }

    std::string s;
    bool host_found, connection_found;
    std::string host, connection;

    while (ss >> s) {
        if (s == "Host:") {
            host_found = true;
            if (!(ss >> host)) {
                return 0;
            }
        }
        if (s == "Connection:") {
            connection_found = true;
            if (!(ss >> connection)) {
                return 0;
            }
        }
        if (connection_found && host_found) {
            break;
        }
    }
    return 1;
}

void send_code(int code, int sockfd, std::filesystem::path path) {
    switch (code) {
    case 200: {
        std::ifstream file(path);
        std::stringstream ss;
        ss << file.rdbuf();
        auto content = ss.str();
        std::string type = "application/octet-stream";
        auto ext = std::string(path.extension());
        if (ext == ".txt") {
            type = "text/plain";
        } else if (ext == ".html") {
            type = "text/html";
        } else if (ext == ".css") {
            type = "text/css";
        } else if (ext == ".jpeg" or ext == ".jpg") {
            type = "image/jpeg";
        }  else if (ext == ".png") {
            type = "image/png";
        } else if (ext == ".pdf") {
            type = "application/pdf";
        } 


        std::string response = std::string("HTTP/1.1 200 OK\r\n"
                                           "Server: marcelina\r\n"
                                           "X-XSS-Protection: 0\r\n"
                                           "X-Frame-Options: SAMEORIGIN\r\n"
                                           "Content-Type: " + type + "\r\n"
                                           "Content-Length: " + std::to_string(content.size()) + "\r\n"
                                           "\r\n");

        if (send(sockfd, response.c_str(), response.size(), 0) < 0) {
            fprintf(stderr, "send error %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        };

        for (int i = 0; i < (int)content.size(); i += 1024) {
            auto temp = content.substr(i, 1024);


            if (send(sockfd, temp.c_str(), temp.size(), 0) < 0) {
                fprintf(stderr, "send error %s\n", strerror(errno));
                exit(EXIT_FAILURE);
            }
        }
    } break;

    case 301: {
        std::string response = std::string("HTTP/1.1 301 Moved Permenently\r\n"
                                           "Location: /index.html\r\n");
        if (send(sockfd, response.c_str(), response.size(), 0) < 0) {
            fprintf(stderr, "send error %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }; break;

    case 403: {
        std::string response =
            std::string("HTTP/1.1 403 Forbidden\r\n"
                        "Server: marcelina\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 58\r\n"
                        "\r\n"
                        "<html><body><h1>403, permission denied.</h1></body></html>");
        if (send(sockfd, response.c_str(), response.size(), 0) < 0) {
            fprintf(stderr, "send error %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }; break;

    case 404: {
        std::string response =
            std::string("HTTP/1.1 404 Not Found\r\n"
                        "Server: marcelina\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 55\r\n"
                        "\r\n"
                        "<html><body><h1>404, page not found.</h1></body></html>");
        if (send(sockfd, response.c_str(), response.size(), 0) < 0) {
            fprintf(stderr, "send error %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }; break;

    case 501: {
        std::string response =
            std::string("HTTP/1.1 501 Not Implemented\r\n"
                        "Server: magi\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: 60\r\n"
                        "\r\n"
                        "<html><body><h1>501 Not Implemented.</h1></body></html>");
        if (send(sockfd, response.c_str(), response.size(), 0) < 0) {
            fprintf(stderr, "send error %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }; break;

    default:
        fprintf(stderr, "code error");
        exit(EXIT_FAILURE);
    }
}

std::pair<int, std::filesystem::path> check_path(std::string page, std::string host,
                                                 std::string directory) {
    auto pos = host.find(":");
    auto s = host.substr(0, pos);
    auto path1 = std::filesystem::path(directory + "/" + s + page);

    int depth = 0;
    unsigned int i = 0;
    while (i < page.size()) {
        auto pos = page.find("/", i);
        if (pos == std::string::npos) {
            pos = page.size();
        }
        if (page.substr(i, pos - i) != "..") {
            depth++;
        } else {
            depth--;
        }
        if (depth < 0) {
            return {403, path1};
        }
        i = pos + 1;
    }

    if (!std::filesystem::exists(path1)) {
        return {404, path1};
    }

    if (std::filesystem::is_directory(path1)) {
        return {301, path1};
    }

    return {200, path1};
}

void server_listen(std::string directory, int sockfd) {
    struct sockaddr_in client_addr;

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int client_sockfd = accept(sockfd, NULL, NULL);
    int option = 1;
    setsockopt(client_sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));
    if (client_sockfd < 0) {
        fprintf(stderr, "accept error %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    char ip_address[20];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_address, sizeof(ip_address));

    fd_set descriptors;
    FD_ZERO(&descriptors);
    FD_SET(client_sockfd, &descriptors);

    while (1) {
        int ready = select(client_sockfd + 1, &descriptors, NULL, NULL, &tv);
        if (ready < 0) {
            fprintf(stderr, "select() error: %s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
        if (ready == 0) {
            break;
        }

        socklen_t client_len = sizeof(client_addr);
        u_int8_t buffer[IP_MAXPACKET];

        ssize_t packet_len = recvfrom(client_sockfd, buffer, IP_MAXPACKET, MSG_DONTWAIT,
                                      (struct sockaddr *)&client_addr, &client_len);

        std::string s = (char *)buffer;

        if (packet_len < 0) {
            fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
            // exit(EXIT_FAILURE);
            break;
        }

        if (packet_len == 0) {
            break;
        }

        int ok = check_get_request(std::string((char *)buffer));

        if (ok == 0) {
            send_code(501, client_sockfd, std::filesystem::path());
            break;
        }

        std::stringstream ss((char *)buffer);
        std::string tmp, page, host;
        ss >> tmp;
        ss >> page;
        for (int i = 0; i < 2; i++) {
            ss >> tmp;
        }
        ss >> host;

        auto [result, path] = check_path(page, host, directory);


        if (result > 1) {
            send_code(result, client_sockfd, path);
        }

        send_code(200, client_sockfd, path);
    }

    close(client_sockfd);
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr,
                "Invalid number of arguments, try ./webserver <port> <directory>");
        return EXIT_FAILURE;
    }

    check_port(argv[1]);
    int port = atoi(argv[1]);
    if (port > 65535) {
        fprintf(stderr, "error: given port is too large \n");
        return EXIT_FAILURE;
    }

    check_dir(argv[2]);
    char *directory = argv[2];

    int sockfd = create_socket_and_bind_port(port);
    int option = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(int));

    if (listen(sockfd, 64) < 0) {
        fprintf(stderr, "listen error: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    while (1) {
        server_listen(directory, sockfd);
    }

    close(sockfd);
    return 0;
}