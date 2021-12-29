# ferrero

## about
An optimized C program that calculates all valid words that could be used in place of "woodchuck" in the sentence "how much wood could a woodchuck chuck if a woodchuck could chuck wood". 

Outputs 7435 valid words (with some error) from a combined wordlist of length of ~100000 words in around 13-14 milliseconds on my machine. Uses relatively low amounts of memory at about 3.53 MiB of heap and 720 bytes of stack. Certainly improvable at the cost of speed (the hashmaps are drastically expanded since the djb2 hash used is fast but shoddy).

## usage
```
gcc ferrero.c -o ferrero -O3
./ferrero # enjoy!
```
