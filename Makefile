server: server.c 
	@$(CC) server.c -o server -Wall -Wextra -pedantic

client: client.c
	@$(CC) client.c -o client -Wall -Wextra -pedantic

clean:
	@rm server
	@rm client
