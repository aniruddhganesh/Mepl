main: src/main.c src/client_mpd.c src/command.c src/input.c src/ui.c src/print.c
	$(CC) src/main.c src/client_mpd.c src/command.c src/input.c src/ui.c src/print.c -lmpdclient -lncurses  -g -o ./bin/maple
