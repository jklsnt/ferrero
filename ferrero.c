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
#define DICTIONARY_LENGTH 121760
#define NOUN_PATH "/home/quantumish/Wordlist-Nouns-All.txt"
#define NOUN_LENGTH 90960
#define VERB_PATH "/home/quantumish/Wordlist-Verbs-All.txt"
#define VERB_LENGTH 30800

#define BUFFER_SIZE 16*1024

// djb2 hash
// http://www.cse.yorku.ca/~oz/hash.html
size_t hash(char* p, size_t sz) {
    unsigned long hash = 5381;
    int c;
    for (int i = 0; i < sz; i++) {
	c = *p++;
	hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

int main() {    
    char* exists = calloc(DICTIONARY_LENGTH/4, 1);
    char* type = calloc(DICTIONARY_LENGTH/4, 1);
    char* valid = calloc(DICTIONARY_LENGTH/4, 1);
    int fd = open(VERB_PATH, O_RDONLY | O_NONBLOCK);
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    char* buf = malloc(BUFFER_SIZE+1); 
    size_t bytes_read = 0;
    while((bytes_read = read(fd, buf, BUFFER_SIZE))) {
	if (bytes_read == 0) break;
	bool first = true;
	for(char *p = buf;;) {
	    char* bound = memchr(p, '\n', (buf + bytes_read) - p);	    
	    int len = bound - p;	   
	    if (first) {
		first = false;
	        p += len+1;
		continue;
	    }
	    if (len <= 0 || bound == 0x0 || p > buf+BUFFER_SIZE) break;
	    size_t true_index = hash(p, len) % (DICTIONARY_LENGTH*2);	    
	    type[true_index/8] |= 1 << (8-(true_index%8));
	    exists[true_index/8] |= 1 << (8-(true_index%8));
	    p += len+1;
	}
    }
    size_t true_index = hash("run", 3) % (DICTIONARY_LENGTH*2);	    
    assert((type[true_index/8] & (1 << (8-(true_index%8)))) == (1 << (8-(true_index%8))));
    fd = open(NOUN_PATH, O_RDONLY | O_NONBLOCK);
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    while((bytes_read = read(fd, buf, BUFFER_SIZE))) {
	if (!bytes_read) break;
	bool first = true;
	for(char *p = buf;;) {
	    char* bound = memchr(p, '\n', (buf + bytes_read) - p);	    
	    int len = bound - p;	   
	    if (first) {
		first = false;
		p += len+1;
		continue;
	    }
	    if (len <= 0 || bound == 0x0) break;
	    size_t true_index = hash(p, len) % (DICTIONARY_LENGTH*2);	    
	    exists[true_index/8] |= 1 << (8-(true_index%8));
	    p += len+1;
	}
    }    
    fd = open(DICTIONARY_PATH, O_RDONLY | O_NONBLOCK);
    posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);
    while((bytes_read = read(fd, buf, BUFFER_SIZE))) {
	if (!bytes_read) break;
	bool first = true;
	for(char *p = buf;;) {
	    char* bound = memchr(p, '\n', (buf + bytes_read) - p);	    
	    int len = bound - p;	   
	    if (first) {
		first = false;
		p += len+1;
		continue;
	    }
	    if (len <= 0 || bound == 0x0) break;
	    for (size_t i = 1; i < len; i++) {	   
		size_t pre_index = hash(p, i) % (DICTIONARY_LENGTH*2);
		size_t post_index = hash(p+i, len-i) % (DICTIONARY_LENGTH*2);		
		if ((exists[pre_index/8] & (1 << (8-(pre_index%8)))) == (1 << (8-(pre_index%8))) && 
		    (exists[post_index/8] & (1 << (8-(post_index%8)))) == (1 << (8-(post_index%8))) && 
		    (type[pre_index/8] & (1 << (8-(pre_index%8)))) != (type[post_index/8] & (1 << (8-(post_index%8))))) {
		    size_t true_index = hash(p, len) % (DICTIONARY_LENGTH*2);
		    for (int j = 0; j < len; j++) {
			printf("%c", *(p+j));		       
		    }
		    printf(" -- ");
		    for (int j = 0; j < i; j++) {
			printf("%c", *(p+j));		       
		    }
		    printf("/");
		    for (int j = 0; j < len-i; j++) {
			printf("%c", *(p+i+j));		       
		    }
		    printf("\n");
		    valid[true_index/8] |= 1 << (8-(true_index%8));
		}
	    }	    
	    p += len+1;
	}
    }    
    true_index = hash("woodchuck", 9) % (DICTIONARY_LENGTH*2);	    
    assert((exists[true_index/8] & (1 << (8-(true_index%8)))) == (1 << (8-(true_index%8))));
    assert((type[true_index/8] & (1 << (8-(true_index%8)))) == 0);
    assert((valid[true_index/8] & (1 << (8-(true_index%8)))) == (1 << (8-(true_index%8))));
}


