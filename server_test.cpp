//
// Created by aaron on 26/3/24.
//
#include "server.h"

static RequestResponse hello_world_func(void* ctx, std::string method, std::string body){
    return {
            200,
            "{ \"msg\" : \"Hello World\" }"
    };
}

void enable_hello_test() {
    RequestHandler handler = {
            "/hello",
            NULL,
            &hello_world_func
    };
    register_handler(handler);
}