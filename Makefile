all:
	gcc -fPIC -shared -o bindhack.so bindhack.c -lc -ldl
