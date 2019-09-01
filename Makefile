
LIBS = -lpthread -lX11 -lxcb -lXau -lXext -lXdmcp -lpthread -ldl -lXcursor -lXmu

all:
	gcc main.c threadpool.c $(LIBS) -o main.o
