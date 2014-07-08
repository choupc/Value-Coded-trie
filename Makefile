all: vc_trie
bps: vc_trie.c rdtsc.h
	gcc vc_trie.c -g -o vc_trie

clean:
	rm vc_trie
