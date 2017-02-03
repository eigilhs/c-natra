#include "c-natra.h"


get("/") {
	html("<h1>Hello, World!</h1>");
	return HTTP_OK;
}

put("/user/:name") {
	html("<p>User '%s' created.</p>", params("name"));
	return HTTP_CREATED;
}

get("/api") {
	json("{\"stuff\": %d}", 42);
	return HTTP_OK;
}

get("/template") {
	view("index.html", map("title", "Template test",
			       "body", "<h1>Success!</h1>"));
	return HTTP_OK;
}

get("/search") {
	html("<h1>Searching for %s ...</h1>", query("q"));
	return HTTP_OK;
}

post("/gimme") {
	html("<p>First eight chars posted: %s</p>", fetch(8));
	return HTTP_OK;
}

serve(8000)
