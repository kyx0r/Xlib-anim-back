
LIBS = -lpthread -lX11 -lxcb -lXau -lXext -lXdmcp -lpthread -ldl -lm

all:
	gcc -O3 -flto -g main.c threadpool.c $(LIBS) -o main.o
