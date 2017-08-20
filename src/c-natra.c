#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "c-natra.h"


void log_req(struct evhttp_request *req, uint16_t code)
{
	char ts[64], *addr, *m[] = {"GET", "POST", "HEAD", "PUT", "DELETE",
				    "OPTIONS", "TRACE", "CONNECT", "PATCH"};
	const char *uri = evhttp_request_get_uri(req);
	int method = __builtin_ctz(evhttp_request_get_command(req));
	uint16_t p;
	time_t t = time(NULL);
	strftime(ts, sizeof(ts), "%c", localtime(&t));
	struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
	evhttp_connection_get_peer(evhttp_request_get_connection(req), &addr, &p);
	/* XXX: New in libevent 2.1 */
	/* addr = evhttp_connection_get_addr(evhttp_request_get_connection(req)); */
	fprintf(stderr, "%s %15s %s %d %s %s\n", ts, addr, m[method],
		code, uri, evhttp_find_header(headers, "User-Agent"));
}

static h2o_pathconf_t *register_handler(h2o_hostconf_t *hostconf,
		const char *path, int (*on_req)(h2o_handler_t *, h2o_req_t *))
{
	h2o_pathconf_t *pathconf = h2o_config_register_path(hostconf, path, 0);
	h2o_handler_t *handler = h2o_create_handler(pathconf, sizeof(*handler));
	handler->on_req = on_req;
	return pathconf;
}

static int request_handler(h2o_handler_t *self, h2o_req_t *req)
{
	const char *u, *p;
	uint16_t i, code;
	char key[64], *value;
	struct request request;
	struct response response;
	struct handler **cur;

	req->method.base, req->method.len;
	req->pathconf->path
	/* enum evhttp_cmd_type method = evhttp_request_get_command(req); */
	const struct evhttp_uri *uri = evhttp_request_get_evhttp_uri(req);
	struct trie *pattern_trie = trie_new();

	request.ev_req = req;
	request.pattern_trie = pattern_trie;
	request.uri = uri;

	for (cur = __start_handlers; cur < __stop_handlers; ++cur) {

		if ((*cur)->method != method)
			continue;

		u = evhttp_uri_get_path(uri);
		p = (*cur)->pattern;

		while (*p++ && *u++) {

			if (*p == ':') {
				value = malloc(64);

				for (++p, i = 0; i < 64 && *p && *p != '/';)
					key[i++] = *p++;
				key[i] = 0;
				for (i = 0; i < 64 && *u && *u != '/';)
					value[i++] = *u++;
				value[i] = 0;

				trie_insert(pattern_trie, key, value);
			}

			if (*p != *u)
				break;

			if (!*p && !*u) {
				response.buffer = evbuffer_new();
				code = (*cur)->handler(&request, &response);
				evhttp_send_reply(req, code, NULL,
						  response.buffer);
				evbuffer_free(response.buffer);
				goto out;
			}
		}
	}

	evhttp_send_error(req, code = HTTP_NOTFOUND, NULL);
out:
	/* log_req(req, code); */
	trie_free(pattern_trie, 1);
}

void _fill_body(struct response *resp, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	evbuffer_add_vprintf(resp->buffer, format, args);
	va_end(args);
}

void _set_header(struct evhttp_request *req, const char *key, const char *value)
{
	struct evkeyvalq *headers = evhttp_request_get_output_headers(req);
	evhttp_remove_header(headers, key);
	evhttp_add_header(headers, key, value);
}

void _render_template(struct response *resp, const char *template,
		      struct trie *map)
{
	const char *c = template;
	char *value, key[64];
	size_t i;
	for (; *c; ++c) {
		if (c[0] != '{' || c[1] != '{')
			continue;
		evbuffer_add_reference(resp->buffer, template, c - template,
				       NULL, NULL);
		c += 2;
		for (i = 0; *c && (c[0] != '}' || c[1] != '}'); ++c, ++i)
			key[i] = *c;
		key[i] = 0;
		value = trie_find(map, key);
		for (i = 0; value[i]; ++i)
			;
		evbuffer_add_reference(resp->buffer, value, i, NULL, NULL);
		template = c += 2;
	}
	evbuffer_add_reference(resp->buffer, template, c - template,
			       NULL, NULL);
	trie_free(map, 0);
}

struct trie *_map(void *dummy, ...)
{
	va_list ap;
	char *key, *value;
	struct trie *map = trie_new();

	va_start(ap, dummy);
	while ((key = va_arg(ap, char *)) && (value = va_arg(ap, char *)))
		trie_insert(map, key, value);
	va_end(ap);

	return map;
}

static void sigint_cb(evutil_socket_t sig, short events, void *base)
{
	printf("\nGoodbye!\n");
	event_base_loopexit((struct event_base *)base, NULL);
}

static void on_accept(uv_stream_t *listener, int status)
{
	uv_tcp_t *conn;
	h2o_socket_t *sock;

	if (status != 0)
		return;

	conn = h2o_mem_alloc(sizeof(*conn));
	uv_tcp_init(listener->loop, conn);

	if (uv_accept(listener, (uv_stream_t *)conn) != 0) {
		uv_close((uv_handle_t *)conn, (uv_close_cb)free);
		return;
	}

	sock = h2o_uv_socket_create((uv_handle_t *)conn, (uv_close_cb)free);
	h2o_accept(&accept_ctx, sock);
}

static int create_listener(void)
{
	static uv_tcp_t listener;
	struct sockaddr_in addr;
	int r;

	uv_tcp_init(ctx.loop, &listener);
	uv_ip4_addr("127.0.0.1", 7890, &addr);
	if ((r = uv_tcp_bind(&listener, (struct sockaddr *)&addr, 0)) != 0) {
		fprintf(stderr, "uv_tcp_bind:%s\n", uv_strerror(r));
		goto Error;
	}
	if ((r = uv_listen((uv_stream_t *)&listener, 128, on_accept)) != 0) {
		fprintf(stderr, "uv_listen:%s\n", uv_strerror(r));
		goto Error;
	}

	return 0;
Error:
	uv_close((uv_handle_t *)&listener, NULL);
	return r;
}

int start(uint16_t port)
{
	h2o_hostconf_t *hostconf;

	signal(SIGPIPE, SIG_IGN);

	h2o_config_init(&config);
	hostconf = h2o_config_register_host(&config, h2o_iovec_init(H2O_STRLIT("default")), 65535);
	register_handler(hostconf, "/", request_handler);
	/* h2o_reproxy_register(register_handler(hostconf, "/reproxy-test", reproxy_test)); */
	/* h2o_file_register(h2o_config_register_path(hostconf, "/", 0), "examples/doc_root", NULL, NULL, 0); */

	uv_loop_t loop;
	uv_loop_init(&loop);
	h2o_context_init(&ctx, &loop, &config);

	/* if (USE_HTTPS && setup_ssl("examples/h2o/server.crt", "examples/h2o/server.key") != 0) */
	/* 	goto error; */

	/* disabled by default: uncomment the line below to enable access logging */
	/* h2o_access_log_register(&config.default_host, "/dev/stdout", NULL); */

	accept_ctx.ctx = &ctx;
	accept_ctx.hosts = config.hosts;

	if (create_listener() != 0) {
		fprintf(stderr, "failed to listen to 127.0.0.1:7890:%s\n", strerror(errno));
		goto error;
	}

	uv_run(ctx.loop, UV_RUN_DEFAULT);

error:
	return 1;

	/* struct event *signal_event; */
	/* struct event_base *base = event_base_new(); */
	/* struct evhttp *http = evhttp_new(base); */
	/* evhttp_set_allowed_methods(http, EVHTTP_REQ_GET | EVHTTP_REQ_POST */
	/* 			   | EVHTTP_REQ_PUT | EVHTTP_REQ_DELETE); */
	/* evhttp_bind_socket(http, "0.0.0.0", port); */
	/* evhttp_set_gencb(http, request_handler, NULL); */
	/* signal_event = evsignal_new(base, SIGINT, sigint_cb, (void *)base); */
	/* event_add(signal_event, NULL); */
	/* /\* XXX: New in libevent 2.1 *\/ */
	/* /\* evhttp_set_default_content_type(http, "text/html; charset=utf-8"); *\/ */

	/* printf("\nServing on port %u ...\n\n", port); */

	/* event_base_dispatch(base); */

	/* event_free(signal_event); */
	/* evhttp_free(http); */
	/* event_base_free(base); */

	/* return 0; */
}
