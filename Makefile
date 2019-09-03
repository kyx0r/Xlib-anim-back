
LIBS = -lpthread -lX11 -lxcb -lXau -lXext -lXdmcp -lpthread -ldl -lm

all:
	gcc -O3 -flto -s main.c threadpool.c $(LIBS) -o main.o
