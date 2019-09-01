
LIBS = -lpthread -lX11 -lxcb -lXau -lXext -lXdmcp -lpthread -ldl -lm

all:
	gcc main.c threadpool.c $(LIBS) -o main.o
