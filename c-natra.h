#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <event2/keyvalq_struct.h>
#include <alloca.h>
#include "trie.h"


#define _HANDLER2(PATTERN, METHOD, ID)					\
	int _handler_##ID(struct request *, struct response *);		\
	struct handler _s_handler_##ID = {&_handler_##ID, METHOD, PATTERN}; \
	struct handler *_sp_handler_##ID				\
	__attribute__((__section__("handlers"))) = &_s_handler_##ID;	\
	int _handler_##ID(struct request *req, struct response *resp)
#define _HANDLER(PATTERN, METHOD, ID) _HANDLER2(PATTERN, METHOD, ID)

#define connect(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_CONNECT, __COUNTER__)
#define delete(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_DELETE, __COUNTER__)
#define get(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_GET, __COUNTER__)
#define options(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_OPTIONS, __COUNTER__)
#define patch(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_PATCH, __COUNTER__)
#define post(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_POST, __COUNTER__)
#define put(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_PUT, __COUNTER__)
#define trace(PATTERN) _HANDLER(PATTERN, EVHTTP_REQ_TRACE, __COUNTER__)

#define map(...) _map(NULL, __VA_ARGS__, NULL)
#define _VIEW2(FNAME, MAP, ID) __asm__ __volatile__(			\
		"	.data			\n"			\
		"template_"#ID":		\n"			\
		"	.incbin	\""FNAME"\"	\n"			\
		"	.byte	0		\n"			\
		"	.align	4		\n"			\
		"	.text			\n");			\
	extern void *template_##ID;					\
	_render_template(resp, (char *)&template_##ID, MAP)
#define _VIEW(FNAME, MAP, ID) _VIEW2(FNAME, MAP, ID)
#define view(FNAME, MAP) _VIEW(FNAME, MAP, __COUNTER__)
#define json(PATTERN, ...) body(PATTERN, ##__VA_ARGS__);		\
	set_header("Content-Type", "application/json; charset=utf-8")
#define html(PATTERN, ...) body(PATTERN, ##__VA_ARGS__);		\
	set_header("Content-Type", "text/html; charset=utf-8")
#define body(PATTERN, ...) _fill_body(resp, PATTERN, ##__VA_ARGS__)
#define set_header(HEADER, VALUE) _set_header(req->ev_req, HEADER, VALUE)
#define header(HEADER) evhttp_find_header(				\
		evhttp_request_get_input_headers(req->ev_req), HEADER)
#define fetch(BYTES)							\
	({ struct evbuffer *b = evhttp_request_get_input_buffer(req->ev_req); \
		size_t len = evbuffer_get_length(b);			\
		len = len > BYTES ? BYTES : len;			\
		char *buf = alloca(len+1);				\
		ev_ssize_t n = evbuffer_copyout(b, buf, len);		\
		buf[len] = 0;						\
		buf; })
#define params(KEY) trie_find(req->pattern_trie, KEY)
#define query(KEY) ({ const char *qs = evhttp_uri_get_query(req->uri);	\
			static struct evkeyvalq params;			\
			evhttp_parse_query_str(qs, &params);		\
			evhttp_find_header(&params, KEY); })

#define serve(PORT) int main(void) { return start(PORT); }

struct response {
	struct evbuffer *buffer;
};

struct request {
	struct evhttp_request *ev_req;
	struct trie *pattern_trie;
	const struct evhttp_uri *uri;
};

struct handler {
	int (*handler)(struct request *, struct response *);
	enum evhttp_cmd_type method;
	const char *pattern;
};

int start(uint16_t port);
void _render_template(struct response *resp, const char *template, struct trie *map);
struct trie *_map(void *, ...);
void _set_header(struct evhttp_request *req, const char *key, const char *value);
void _fill_body(struct response *resp, const char *format, ...);

extern struct handler *__start_handlers[], *__stop_handlers[];

/* Some codes missing from libevent */
#define HTTP_CREATED		201
#define HTTP_RESETCONTENT	205
#define HTTP_PARTIALCONTENT	206
#define HTTP_MULTIPLECHOICES	300
#define HTTP_SEEOTHER		303
#define HTTP_USEPROXY		305
#define HTTP_TEMPORARYREDIRECT	307
#define HTTP_UNAUTHORIZED	401
#define HTTP_FORBIDDEN		403
#define HTTP_NOTACCEPTABLE	406
#define HTTP_GONE		410
