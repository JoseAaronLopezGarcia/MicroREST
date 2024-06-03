//
// Created by aaron on 26/3/24.
//

#include <arpa/inet.h>
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <regex.h>
#include <cstdbool>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <iostream>

#include "server.h"


#define MAX_METHOD_SIZE 32
#define MAX_PATH_SIZE 255
#define BUFFER_SIZE (64*1024)
#define SCAN_FORMAT "%31s %254s HTTP/%f"

static volatile bool server_running = false;
static pthread_t thread_id;
static int server_port;
static std::string token = "";
static std::unordered_map<std::string, RequestHandler> handlers;

static bool isEmpty(std::string str){
    for (int i=0; i<str.size(); i++){
        char c = str[i];
        if (c != ' ' && c != '\n' && c != '\r' && c != '\t'){
            return false;
        }
    }
    return true;
}

static inline char* response_code(int code){
    switch (code){
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method not Allowed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
    }
    return "Unknown Error";
}

static void reply_reponse(int client_fd, RequestResponse res){
    int strsize = res.body.size() + MAX_PATH_SIZE;
    char* response = new char[strsize];
    char* format = "HTTP/1.1 %d %s\r\n"
                 "Content-Type: application/json\r\n"
                 "\r\n"
                 "%s";

    snprintf(response, strsize, format, res.code, response_code(res.code), res.body.c_str());
    send(client_fd, response, strlen(response), 0);

    delete[] response;
}

static void reply_error(int client_fd, int code, char* body){
    char* response = new char[BUFFER_SIZE];
    char* format = "HTTP/1.1 %d %s\r\n"
                 "Content-Type: text/plain\r\n"
                 "\r\n"
                 "%s";

    snprintf(response, BUFFER_SIZE, format, code, response_code(code), body);
    send(client_fd, response, strlen(response), 0);

    delete[] response;
}

static bool checkAuthorization(std::vector<std::string>& header){
    if (token.size() == 0) return true; // no auth
    bool res = false;
    char* t = new char[64];
    memset(t, 0, 64);
    for (int i=0; i<header.size(); i++){
        if (strncmp(header[i].c_str(), "Authorization:", 14) == 0){
            sscanf(header[i].c_str(), "Authorization: Token %s", t);
            res = (token == t);
        }
    }
    delete[] t;
    return res;
}

static void* main_server_thread(void* arg){
    int server_fd;
    struct sockaddr_in server_addr;

    // create server socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("can't open socket (%d)\n", server_fd);
        return NULL;
    }

    // config socket
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(server_port);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt));

    // bind socket to port
    if (bind(server_fd,
            (struct sockaddr *)&server_addr, 
            sizeof(server_addr)) < 0) {
        printf("can't bind socket (%d)\n", errno);
        goto server_thread_end;
    }

    // listen for connections
    if (listen(server_fd, 10) < 0) {
        printf("can't listen socket (%d)\n", errno);
        goto server_thread_end;
    }

    while (server_running) {
        // client info
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int client_fd;

        // accept client connection
        if ((client_fd = accept(server_fd, 
                                (struct sockaddr *)&client_addr, 
                                &client_addr_len)) < 0) {
            printf("can't accept client (%d)\n", client_fd);
            continue;
        }

        // obtain request
        printf("Obtaining request\n");
        std::string request;
        char* buffer = new char[BUFFER_SIZE];
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received] = 0;
        request = std::string(buffer);
        delete[] buffer;

        printf("parsing request:\n%s\n", request.c_str());

        // parse request
        std::vector<std::string> header;
        std::string body;
        bool sep_reached = false;
        std::istringstream f(request);
        std::string line;
        while (std::getline(f, line)){
            if (isEmpty(line)){
                sep_reached = true;
                continue;
            }
            if (sep_reached){
                body += (line + "\n");
            }
            else {
                header.push_back(line);
            }
        }

        printf("Body:\n%s\n", body.c_str());

        // parse first header
        std::string request_head = header[0];
        printf("request_head: %s\n", request_head.c_str());
        char* method = new char[MAX_METHOD_SIZE];
        char* path = new char[MAX_PATH_SIZE];
        float http_ver = 0.f;
        sscanf(request_head.c_str(), SCAN_FORMAT, method, path, &http_ver);

        printf("method: %s\n", method);
        printf("path: %s\n", path);
        printf("HTTP Ver: %f\n", http_ver);

        if (http_ver != 1.1f){
            reply_error(client_fd, 501, "Only HTTP 1.1 supported");
            close(client_fd);
            continue;
        }
        
        if (handlers.find(path) == handlers.end()){
            reply_error(client_fd, 404, "No handler defined for path");
            close(client_fd);
            continue;
        }

        if (!checkAuthorization(header)){
            reply_error(client_fd, 401, "Invalid Token");
            close(client_fd);
            continue;
        }

        RequestHandler handler = handlers[path];
        RequestResponse response = handler.callback(handler.ctx, method, body);
        reply_reponse(client_fd, response);

        close(client_fd);
        delete[] method;
        delete[] path;
    }

    server_thread_end:

    close(server_fd);
    return NULL;
}

void set_authorization_token(const char* t){
    token = std::string(t);
}

void start_server_thread(int port){
    // create a new thread to handle client request
    if (!server_running) {
        server_running = true;
        server_port = port;
        pthread_create(&thread_id, NULL, main_server_thread, NULL);
        pthread_detach(thread_id);
    }
}

void stop_server_thread(){
    if (server_running) {
        server_running = false;
        pthread_kill(thread_id, SIGKILL);
    }
}

void register_handler(RequestHandler handler){
    handlers[handler.path] = handler;
}

void unregister_handler(std::string path){
    if (handlers.find(path) != handlers.end())
        handlers.erase(path);
}