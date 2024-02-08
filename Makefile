main: src/main.c src/client_mpd.c src/command.c src/input.c
	$(CC) src/main.c src/client_mpd.c src/command.c src/input.c -lmpdclient -lncurses -Wall -g -o ./bin/maple
