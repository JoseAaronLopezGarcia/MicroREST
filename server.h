//
// Created by aaron on 26/3/24.
//

#pragma once

#include <string>

#ifdef __ANDROID__
#include <android/log.h>
#define printf(...) __android_log_print(ANDROID_LOG_DEBUG, "Native Server", __VA_ARGS__);
#endif

struct RequestResponse{
    int code;
    std::string body;
};

struct RequestHandler{
    std::string path;
    void* ctx;
    RequestResponse (*callback)(void* ctx, std::string method, std::string body);
};

void set_authorization_token(const char* t);
void start_server_thread(int port);
void stop_server_thread();
void register_handler(RequestHandler handler);
void unregister_handler(std::string path);

void enable_hello_test();