/* ALEH sample code.
 *
 * Copyright (c) 2012, Ivan Ribeiro Rocha <ivan.ribeiro at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of ALEH nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
