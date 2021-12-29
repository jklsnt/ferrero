#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/mman.h>

#define NOUN_PATH "./Wordlist-Nouns-All.txt"
#define NOUN_LENGTH 90960
#define VERB_PATH "./verbs_inf.txt"
#define VERB_LENGTH 6057
#define DICTIONARY_LENGTH (NOUN_LENGTH+VERB_LENGTH)

#define BUFFER_SIZE (64*1024) // Align to page size while still being big enough.

#define HASHMAP_LEN (DICTIONARY_LENGTH*100) // Chosen through ~~science~~ trial and error.

// djb2 hash
// http://www.cse.yorku.ca/~oz/hash.html
size_t hash(char* p, size_t sz) {
    unsigned long hash = 5381;    
    for (int i = 0; i < sz; i++) hash += (hash << 5) + *p++;
    return hash;
}

void generate(char* path, char* buf, char* incomplete_word, char* exists, char* type) {
    size_t bytes_read = 0;
    int fd = open(path, O_RDONLY | O_NONBLOCK); // Non-blocking read
    // Tell the kernel we're immediately going to read sequentially with no reuse.
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_NOREUSE | POSIX_FADV_WILLNEED);
    while((bytes_read = read(fd, buf, BUFFER_SIZE))) {
	if (bytes_read == 0) break;
	bool first = true;
	char* p = buf;
	while (true) {
	    // Find the end of the line.
	    char* bound = memchr(p, '\n', (buf + bytes_read) - p);
	    int len = bound - p;
	    if (bound == 0x0) break;
	    size_t true_index;
	    // Handle incomplete word at start of buffer by appending to saved array.
	    if (first) {
		first = false;
		strncat(incomplete_word, p, len+1);
		size_t word_len = strlen(incomplete_word);
		true_index = hash(incomplete_word, word_len) % HASHMAP_LEN;
		memset(incomplete_word, 0, 45);
	    }
	    else true_index = (uint64_t)hash(p, len) % (uint64_t)HASHMAP_LEN;
	    // Don't bother writing 0s to represent nouns since the buffer is calloc'd
	    if (type != 0x0) type[true_index/8] |= 1 << (8-(true_index%8));
	    // Write a 1 to show it exists
	    exists[true_index/8] |= 1 << (8-(true_index%8));
	    p += len+1;
	}
	// Handle incomplete word at end of buffer by saving it to an array.
	if (p != buf + bytes_read) memcpy(incomplete_word, p, buf+bytes_read-p);
    }
}

int main() {
    // Allocate hashmaps. Everything we care about (word existence, type, validity) is
    // representable by a bit, so pack them into ints for a much lower memory cost.
    // Then, since they're hashmaps, tell the kernel we're going to access them randomly.
    char* exists = calloc(HASHMAP_LEN/8, 1);
    madvise(exists, HASHMAP_LEN/8, MADV_RANDOM | MADV_WILLNEED);
    char* type = calloc(HASHMAP_LEN/8, 1);
    madvise(type, HASHMAP_LEN/8, MADV_RANDOM | MADV_WILLNEED);
    char* valid = calloc(HASHMAP_LEN/8, 1);
    madvise(valid, HASHMAP_LEN/8, MADV_RANDOM | MADV_WILLNEED);
    // Allocate buffers necessary for buffered read of file.
    char* buf = malloc(BUFFER_SIZE+1);    
    char incomplete_word[45] = {0};    
    generate(VERB_PATH, buf, incomplete_word, exists, type); 
    generate(NOUN_PATH, buf, incomplete_word, exists, 0x0);
    // Iterate over nouns wordlist one more time to check validity. Mostly same I/O logic.
    // FIXME Incomplete words not handled here.
    int fd = open(NOUN_PATH, O_RDONLY | O_NONBLOCK);
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL | POSIX_FADV_WILLNEED | POSIX_FADV_NOREUSE);
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
	    // Check all possible splits
	    for (size_t i = 1; i < len; i++) {
		size_t pre_index = hash(p, i) % HASHMAP_LEN;
		size_t post_index = hash(p+i, len-i) % HASHMAP_LEN;
		// Check that both subwords exist and are different types
		if ((exists[pre_index/8] & (1 << (8-(pre_index%8)))) == (1 << (8-(pre_index%8))) &&
		    (exists[post_index/8] & (1 << (8-(post_index%8)))) == (1 << (8-(post_index%8))) &&
		    (type[pre_index/8] & (1 << (8-(pre_index%8)))) != (type[post_index/8] & (1 << (8-(post_index%8))))) {
		    // Print combination out and update validity  hashmap.
		    size_t true_index = hash(p, len) % HASHMAP_LEN;
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
