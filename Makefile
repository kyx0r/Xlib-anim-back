
LIBS = -lpthread -lX11 -lm

all:
	gcc -O3 -flto -g main.c threadpool.c $(LIBS) -o bg
