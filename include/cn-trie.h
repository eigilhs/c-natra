struct trie {
	char character;
	void *data;
	struct trie *children;
	struct trie *siblings;
};

struct trie *trie_new(void);
void trie_free(struct trie *trie, int free_data);
void *trie_find(struct trie *trie, const char *key);
void trie_insert(struct trie *trie, const char *key, void *value);
