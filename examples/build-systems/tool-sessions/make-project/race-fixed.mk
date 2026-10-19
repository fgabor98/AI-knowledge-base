app: app.o
	cc app.o -o app

config.h:
	sleep 1
	printf '#define BOARD_ID 7\n' > $@

app.o: app.c config.h
	cc -c app.c -o app.o
