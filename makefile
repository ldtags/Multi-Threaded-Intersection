helgrind:
	gcc helgrind.c -lpthread -o helgrind

clean:
	rm -rf helgrind