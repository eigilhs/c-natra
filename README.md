# C-natra

A Sinatra-style microframework for C. Very inspired by
[Bogart](https://github.com/tyler/Bogart), but probably even more
unsuitable for serious use (or any use at all).

```c
#include <c-natra.h>

get("/") {
	html("<h1>Hello, World!</h1>");
	return HTTP_OK;
}

serve(8000)
```

More examples [here](examples/example.c).

Depends on [libevent](https://github.com/libevent/libevent) 2.x.
