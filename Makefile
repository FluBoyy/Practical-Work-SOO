CC=gcc
CFLAGS=-o
LIBS=-lncurses -pthread

all: jogoUI motor bot

jogoUI: jogoUI.c
	$(CC) $(CFLAGS) jogoUI jogoUI.c $(LIBS)

motor: motor.c
	$(CC) $(CFLAGS) motor motor.c $(LIBS)

bot: bot.c
	$(CC) $(CFLAGS) bot bot.c

clean:
	$(RM) -f jogoUI motor bot
