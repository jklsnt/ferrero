/* Prompt:
ferrero the woodchuck is tired of being made fun of for not being able to chuck wood. instead of going to the gym to practice throwing things, he's been sitting at home eating this giant tub of popcorn that my mom's work sent us for no apparent reason. he realizes that popcorn, like woodchucks, must be made fun of too. presently, he begins to feel bad and becomes a social justice warrior on a mission to right this wrong in the world.

given a dictionary of N words with average length L letters, each labeled with noun, verb, both, or none, find all words that can be broken down into "[noun][verb]" or "[verb][noun]" so that they might be oppressed
*/

#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define DICTIONARY_PATH "/home/quantumish/Wordlist-All.txt"
#define NOUN_PATH "/home/quantumish/Wordlist-Nouns-All.txt"
#define NOUN_LENGTH 90960
#define VERB_PATH "/home/quantumish/verbs_inf.txt"
#define VERB_LENGTH 6057
#define DICTIONARY_LENGTH (NOUN_LENGTH+VERB_LENGTH)

#define BUFFER_SIZE (16*1024)

#define HASHMAP_LEN (DICTIONARY_LENGTH*100)

// djb2 hash
// http://www.cse.yorku.ca/~oz/hash.html
uint64_t hash(char* p, size_t sz) {
    unsigned long hash = 5381;
    int c;
    for (int i = 0; i < sz; i++) {
	c = *p++;
	hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

void generate(char* path, char* buf, char* incomplete_word, char* exists, char* type) {
    size_t bytes_read = 0;
    int fd = open(path, O_RDONLY | O_NONBLOCK);
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_NOREUSE | POSIX_FADV_WILLNEED);
    while((bytes_read = read(fd, buf, BUFFER_SIZE))) {
	if (bytes_read == 0) break;
	bool first = true;
	char* p = buf;
	while (true) {
	    char* bound = memchr(p, '\n', (buf + bytes_read) - p);
	    int len = bound - p;
	    if (bound == 0x0) break;
	    uint64_t true_index;
	    if (first) {
		first = false;
		strncat(incomplete_word, p, len+1);
		size_t word_len = strlen(incomplete_word);
		true_index = hash(incomplete_word, word_len) % HASHMAP_LEN;
		memset(incomplete_word, 0, 45);
	    }
	    else true_index = (uint64_t)hash(p, len) % (uint64_t)HASHMAP_LEN;
	    if (type != 0x0) type[true_index/8] |= 1 << (8-(true_index%8));
	    exists[true_index/8] |= 1 << (8-(true_index%8));
	    p += len+1;
	}
	if (p != buf + bytes_read) memcpy(incomplete_word, p, buf+bytes_read-p);
    }
}

int main() {
    char* exists = calloc(HASHMAP_LEN/8, 1);
    char* type = calloc(HASHMAP_LEN/8, 1);
    char* valid = calloc(HASHMAP_LEN/8, 1);
    int fd = open(VERB_PATH, O_RDONLY | O_NONBLOCK);
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    char* buf = malloc(BUFFER_SIZE+1);
    char incomplete_word[45] = {0};
    generate(VERB_PATH, buf, incomplete_word, exists, type);
    generate(NOUN_PATH, buf, incomplete_word, exists, 0x0);
    fd = open(NOUN_PATH, O_RDONLY | O_NONBLOCK);
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    size_t bytes_read = 0;
    while((bytes_read = read(fd, buf, BUFFER_SIZE))) {
	bool nasty_error = false;
	if (!bytes_read) break;
	bool first = true;
	char *p = buf;
	while (true) {
	    char* bound = memchr(p, '\n', (buf + bytes_read) - p);
	    int len = bound - p;
	    if (bound == 0x0) break;
	    else if (len <= 0) {
		nasty_error = true;
		break;
	    }
	    for (size_t i = 1; i < len; i++) {
		size_t pre_index = hash(p, i) % HASHMAP_LEN;
		size_t post_index = hash(p+i, len-i) % HASHMAP_LEN;
		if ((exists[pre_index/8] & (1 << (8-(pre_index%8)))) == (1 << (8-(pre_index%8))) &&
		    (exists[post_index/8] & (1 << (8-(post_index%8)))) == (1 << (8-(post_index%8))) &&
		    (type[pre_index/8] & (1 << (8-(pre_index%8)))) != (type[post_index/8] & (1 << (8-(post_index%8))))) {
		    uint64_t true_index = hash(p, len) % HASHMAP_LEN;
		    for (int j = 0; j < i; j++) {
			printf("%c", *(p+j));
		    }
		    printf(" ");
		    for (int j = 0; j < len-i; j++) {
			printf("%c", *(p+i+j));
		    }
		    printf("\n");
		    valid[true_index/8] |= 1 << (8-(true_index%8));
		}
	    }
	    p += len+1;
	}
	if (p != buf + bytes_read && !nasty_error) memcpy(incomplete_word, p, buf+bytes_read-p);
	nasty_error = false;
    }
}
