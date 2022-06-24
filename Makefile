setup:
	gcc libferrero.c -O3 -c -o libferrero.o
	ar rcs libferrero.a libferrero.o
	cp libferrero.a /usr/local/lib
