# app.o includes the generated config.h, but only app lists it - and first,
# so a serial build happens to generate it before compiling app.o.
app: config.h app.o
	cc app.o -o app

config.h:
	sleep 1
	printf '#define BOARD_ID 7\n' > $@

app.o: app.c
	cc -c app.c -o app.o
