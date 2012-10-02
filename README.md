aleh (A LibEvent Http-engine)
-----------

**aleh**  is a very small web engine based upon [libevent] and [redis] using **C++11**

Generating project files with [gyp], if you want to use **Makefile**:

```shell
gyp aleh.gyp --depth=. -f make
```

Clean generated files (Makefile):

```shell
rm -rf out Makefile aleh.Makefile aleh.target.mk
```

Building all and testing with [scons] and [valgrind]:

```shell
scons -c && scons && valgrind --leak-check=full ./aleh
```

Building all and testing with [gyp] and [valgrind]:

```shell
make && valgrind --leak-check=full out/Default/aleh
```

Running [redis]:

```shell
redis-server ./redis.conf
```

Benchmarking using [ab] with 10000 requests:

```shell
ab -n 10000 -c 10 http://localhost:8081/
```

Benchmarking using [httperf] with 10000 requests:

```shell
httperf --server localhost --port 8081 --uri /test --num-call 100 --num-conn 100
```

Querying URI-counter from basic sample code with 10000 requests:

```shell
$ redis-cli -s /tmp/redis.sock -s /tmp/redis.sock get /test
"10000"
```

Dependencies
-----------

* [libevent]: An event notification library
* [hiredis]: Minimalistic C client for Redis
* [scons]: A software construction tool (build tool, or make tool) implemented in Python
* [gyp]: Generates native Visual Studio, Xcode and SCons and/or make build files from a platform-independent input format

Copyright and License
-----------
[BOLA - Buena Onda License Agreement (v1.1)]

  This work is provided 'as-is', without any express or implied warranty. In no event will the authors be held liable for any damages arising from the use of this work.
  
  To all effects and purposes, this work is to be considered Public Domain.
  
  However, if you want to be "buena onda", you should:

  1. Not take credit for it, and give proper recognition to the authors.
  2. Share your modifications, so everybody benefits from them.
  3. Do something nice for the authors.
  4. Help someone who needs it: sign up for some volunteer work or help your neighbour paint the house.
  5. Don't waste. Anything, but specially energy that comes from natural non-renewable resources. Extra points if you discover or invent something to replace them.
  6. Be tolerant. Everything that's good in nature comes from cooperation.

[libevent]: http://libevent.org
[hiredis]: https://github.com/antirez/hiredis
[redis]: http://www.redis.io
[ab]: http://httpd.apache.org/docs/2.2/programs/ab.html
[httperf]: http://www.hpl.hp.com/research/linux/httperf/
[valgrind]: http://valgrind.org/
[scons]: http://www.scons.org/
[gyp]: http://code.google.com/p/gyp/
[BOLA - Buena Onda License Agreement (v1.1)]: http://blitiri.com.ar/p/bola/
