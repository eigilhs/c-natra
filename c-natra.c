#include <stdio.h>
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

void request_handler(struct evhttp_request *req, void *context)
{
	const char *u, *p;
	uint16_t i, code;
	char key[64], *value;
	struct request request;
	struct response response;
	struct handler **cur;

	enum evhttp_cmd_type method = evhttp_request_get_command(req);
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
	log_req(req, code);
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

int start(uint16_t port)
{
	struct event_base *base = event_base_new();
	struct evhttp *http = evhttp_new(base);
	evhttp_set_allowed_methods(http, EVHTTP_REQ_GET | EVHTTP_REQ_POST
				   | EVHTTP_REQ_PUT | EVHTTP_REQ_DELETE);
	evhttp_bind_socket(http, "0.0.0.0", port);
	evhttp_set_gencb(http, request_handler, NULL);
	/* XXX: New in libevent 2.1 */
	/* evhttp_set_default_content_type(http, "text/html; charset=utf-8"); */

	printf("\nServing on port %u ...\n\n", port);

	event_base_dispatch(base);

	evhttp_free(http);
	event_base_free(base);

	return 0;
}
