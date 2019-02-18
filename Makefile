server: server.c 
	$(CC) server.c -o server -Wall -Wextra -pedantic

clean:
	@rm server
