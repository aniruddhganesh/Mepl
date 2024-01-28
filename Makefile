main: main.c client_mpd.c command.c
	$(CC) main.c client_mpd.c command.c -lmpdclient -lncurses -g -o ./bin/maple
