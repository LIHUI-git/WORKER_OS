ll:
	gcc server.c -o ser.out -lsqlite3 -Wall
	gcc client.c -o cli.out -Wall
.PHONY:clean
	clean:cleanrm server client my.db

