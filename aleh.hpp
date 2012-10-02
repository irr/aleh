/* ALEH engine.
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

#pragma once

#include <exception>

#include <iostream>
#include <cstring>
#include <vector>
#include <thread>

#include <event.h>
#include <evhttp.h>
#include <fcntl.h>
#include <unistd.h>

#include "hiredis.h"
#include "adapters/libevent.h"

using namespace std;

struct aleh_config;

struct aleh_cb {
    int status;
    string content_type;
    string body;
    struct aleh_config *cfg;

    aleh_cb() {}
    aleh_cb(int status, 
           string content_type, 
           string body, 
           struct aleh_config *cfg) : status(status), 
                                     content_type(content_type), 
                                     body(body),
                                     cfg(cfg) {}
    aleh_cb(const aleh_cb& cb) : status(cb.status), 
                               content_type(cb.content_type), 
                               body(cb.body),
                               cfg(cb.cfg) {}
};

enum class CType {
    REDIS_CONN_UNIX,
    REDIS_CONN_TCP
};

typedef void (*aleh_exec)(struct evhttp_request *, 
                         redisContext *, 
                         aleh_cb&);

struct aleh_config {
    struct {
        const char *host;
        const int port;
        const int timeout_secs;
        const char *content_type;
        const int backlog;
        const int threads;
    } http;

    struct {
        const enum CType type;
        const int timeout_secs;
    } redis_conn;
    
    struct {
        const char *host;
        const int port;
    } redis_tcp;

    struct {
        const char *path;
    } redis_unix;

    struct {
        const aleh_exec exec;
    } cb;
};

namespace aleh {
    void request_msg(struct evhttp_request *req, 
                     int code, 
                     const char *tag, 
                     struct evbuffer *buffer, 
                     const char* msg) {
        if (NULL != msg)
            evbuffer_add_printf(buffer, "%s", msg);   
        evhttp_send_reply(req, code, tag, buffer);
    }

    string string_format(const string &fmt, ...) {
        int n, size = 256;
        string str;
        va_list ap;
        for (;;) {
            str.resize(size);
            va_start(ap, fmt);
            n = vsnprintf((char *)str.c_str(), size, fmt.c_str(), ap);
            va_end(ap);
            if ((n > -1) && (n < size)) 
                return str;
            size = (n > -1) ? (n + 1) : (size * 2);
        }
    }

    void request_func(struct evhttp_request *req, void *arg) {   
        auto *cfg = static_cast<struct aleh_config *>(arg);

        auto *returnbuffer = evbuffer_new();
        
        redisContext *c = NULL;
        redisReply *r = NULL;

        try {    
            evhttp_add_header(req->output_headers, "Content-Type", cfg->http.content_type);

            if (cfg->redis_conn.type == CType::REDIS_CONN_TCP)
                c = redisConnect(cfg->redis_tcp.host, cfg->redis_tcp.port);
            else if (cfg->redis_conn.type == CType::REDIS_CONN_UNIX)
                c = redisConnectUnix(cfg->redis_unix.path);

            if (c->err) {
                request_msg(req, HTTP_SERVUNAVAIL, "ERROR", returnbuffer, 
                    string_format("{ \"error\": \"SERVICE UNAVAILABLE\" }").c_str());
            }
            else {
                const struct timeval tv = { cfg->redis_conn.timeout_secs, 0 };

                if (REDIS_OK == redisSetTimeout(c, tv)) {
                    aleh_cb cb(HTTP_INTERNAL, string(cfg->http.content_type), 
                        string_format("{ \"error\": \"INTERNAL ERROR\" }").c_str(), cfg);
          
                    auto exec = cfg->cb.exec;

                    exec(req, c, cb);

                    if (HTTP_OK == cb.status)
                        request_msg(req, HTTP_OK, "OK", returnbuffer, cb.body.c_str());
                    else
                        request_msg(req, cb.status, "ERROR", returnbuffer, cb.body.c_str());
                }
                else
                    request_msg(req, HTTP_INTERNAL, "ERROR", returnbuffer, 
                        string_format("{ \"error\": \"SET-TIMEOUT ERROR\" }").c_str());
            }
        }
        catch (exception& e) {
            request_msg(req, HTTP_INTERNAL, "ERROR", returnbuffer, 
                string_format("{ \"error\": \"%s\" }", e.what()).c_str());
        }
        catch (...) {
            request_msg(req, HTTP_INTERNAL, "ERROR", returnbuffer, 
                string_format("{ \"error\": \"CRITICAL ERROR\" }").c_str());
        }

        evbuffer_free(returnbuffer);

        if (NULL != r)
            freeReplyObject(r);   
            
        if (NULL != c)
            redisFree(c);
    }

    void request_handler(struct evhttp_request *req, void *arg) {
        request_func(req, arg);
    }

    int getCPUs() {
        return sysconf(_SC_NPROCESSORS_ONLN);
    }

    int setCPU(int cpu) {
        cpu_set_t set;
        CPU_ZERO(&set);
        CPU_SET(cpu, &set);
        if (sched_setaffinity(0, sizeof(cpu_set_t), &set) == 0)
            return cpu;
        return -1;
    }

    int getCPU() {
        const int cpus = getCPUs();
        cpu_set_t set;
        CPU_ZERO(&set);
        if (sched_getaffinity(0, sizeof(cpu_set_t), &set) == 0)
            for (int i = 0; i < cpus; i++)
              if (CPU_ISSET(i, &set) > 0) 
                return i;              
        return -1;
    }

    void dispatcher(void *arg, int cpus, int n) {
        setCPU(n % cpus);
        event_base_dispatch((struct event_base*)arg);
    }

    int event_loop(aleh_config *cfg) {
        const int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) 
            return errno;

        const int one = 1;
        int r = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(int));
        if (r < 0) 
            return errno;

        struct sockaddr_in addr;
        struct hostent *host;

        if ((host = gethostbyname(cfg->http.host)) == NULL) 
            return -1;

        memset(&addr, 0, sizeof(addr));
        memcpy(&addr.sin_addr, host->h_addr_list[0], host->h_length);
        addr.sin_family = AF_INET;
        addr.sin_port = htons(cfg->http.port);

        r = bind(fd, (struct sockaddr*)&addr, sizeof(addr));
        if (r < 0) 
            return errno;

        r = listen(fd, cfg->http.backlog);
        if (r < 0) 
            return errno;

        const int flags = fcntl(fd, F_GETFL, 0);
        if ((flags < 0)
              || (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0))
            return errno;

        vector<thread> threads;
        const int cpus = getCPUs();
        const int nt = cfg->http.threads;

        for (int n = 0; n < nt; n++) {
            struct event_base *base = event_init();
            if (NULL == base) 
                return -1;

            struct evhttp *httpd = evhttp_new(base);
            if (NULL == httpd) 
                return -1;

            r = evhttp_accept_socket(httpd, fd);
            if (r != 0) 
                return -1;

            evhttp_set_timeout (httpd, cfg->http.timeout_secs);     
            evhttp_set_gencb(httpd, request_handler, cfg);

            threads.push_back(thread(dispatcher, base, cpus, n));
        }

        cout << string_format("aleh (cpus:%d, threads:%d) started on %s:%d\n", 
                              cpus, nt, cfg->http.host, cfg->http.port);
        
        for(auto& thread : threads)
            thread.join();

        return 0;
    }
}

