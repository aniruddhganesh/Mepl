main: main.c client_mpd.c command.c input.c
	$(CC) main.c client_mpd.c command.c input.c -lmpdclient -lncurses -g -o ./bin/maple
