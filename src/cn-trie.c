#include <stdlib.h>
#include "cn-trie.h"

struct trie *trie_new(void)
{
	return calloc(sizeof(struct trie), 1);
}

void trie_free(struct trie *trie, int free_data)
{
	if (trie->children)
		trie_free(trie->children, free_data);
	if (trie->siblings)
		trie_free(trie->siblings, free_data);
	if (free_data && trie->data)
		free(trie->data);
	free(trie);
}

void *trie_find(struct trie *trie, const char *key)
{
	for (; *key; ++key) {
		trie = trie->children;
		for (; trie; trie = trie->siblings)
			if (trie->character == *key)
				break;
		if (!trie)
			return NULL;
	}
	return trie->data;
}

void trie_insert(struct trie *trie, const char *key, void *value)
{
	struct trie *child;
	for (child = trie->children; child; child = child->siblings)
		if (child->character == *key)
			break;

	if (!child) {
		child = calloc(sizeof(struct trie), 1);
		child->character = *key;
		child->siblings = trie->children;
		trie->children = child;
	}

	if (!key[1])
		child->data = value;
	else
		trie_insert(child, key + 1, value);
}
