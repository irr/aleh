/* ALEH sample code.
 * 2012, Ivan Ribeiro Rocha <ivan.ribeiro at gmail dot com>
 *
 * BOLA - Buena Onda License Agreement (v1.1)
 * 
 * This work is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this work.
 *   
 * To all effects and purposes, this work is to be considered Public Domain.
 *   
 * However, if you want to be "buena onda", you should:
 * 
 * 1. Not take credit for it, and give proper recognition to the authors.
 * 2. Share your modifications, so everybody benefits from them.
 * 3. Do something nice for the authors.
 * 4. Help someone who needs it: sign up for some volunteer work or help your neighbour paint the house.
 * 5. Don't waste. Anything, but specially energy that comes from natural non-renewable resources. Extra points if you discover or invent something to replace them.
 * 6. Be tolerant. Everything that's good in nature comes from cooperation.
 */
 
#include "aleh.hpp"

void func(struct evhttp_request *req, redisContext *c, aleh_cb& cb) {
    redisReply *r = NULL;

    try {
        r = static_cast<redisReply *>(redisCommand(c, "INCR %s", req->uri));

        if ((NULL != r) && (r->type == REDIS_REPLY_INTEGER)) {  
            cb.status = HTTP_OK;
            cb.body = aleh::string_format("{ \"request\":\"%s\", \"host\":\"%s:%d\", \"value\":%d }", 
                                         req->uri, req->remote_host, req->remote_port, r->integer);
        } 
        else
            cb.body = aleh::string_format("{ \"error\": \"REDIS_REPLY_ERROR\" }");
    }
    catch(...) {
        cb.status = HTTP_INTERNAL;
        cb.body = aleh::string_format("{ \"error\": \"REDIS_INTERNAL_ERROR\" }");
    }

    if (NULL != r)
        freeReplyObject(r);
}

int main()
{
    struct aleh_config cfg = { { "127.0.0.1", 8081, 5, "application/json; charset=UTF-8", 1024, 100 },
                               { CType::REDIS_CONN_UNIX, 5 }, 
                               { "localhost", 6379 }, 
                               { "/tmp/redis.sock" }, 
                               { &func } };

    return aleh::event_loop(&cfg);
}
