all:
	gcc -fPIC -shared -o proxyhack.so proxyhack.c -lc -ldl
