DIR_OBJ = ./obj
DIR_SRC = ./src

SRC = $(wildcard ${DIR_SRC}/*.c ${DIR_SRC}/cJSON/*.c ${DIR_SRC}/Log/*.c ${DIR_SRC}/Tcp/*.c)
OBJ = $(patsubst %.c, ${DIR_OBJ}/%.o, $(notdir $(SRC)))
INC = $(patsubst %, -I%, $(shell find src -name '[a-zA-Z0-9]*'.h))

BIN_TARGET = Daemon

CC = gcc
CFLAGS = -g -Wall $(INC)

${BIN_TARGET}: ${OBJ}
	$(CC) $(OBJ) -o $@ -lpthread -lrt -lm

#main.c
${DIR_OBJ}/%.o: ${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm

${DIR_OBJ}/%.o: ${DIR_SRC}/cJSON/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm

${DIR_OBJ}/%.o: ${DIR_SRC}/Log/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm

${DIR_OBJ}/%.o: ${DIR_SRC}/Tcp/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm


.PHONY:clean
clean:
	find ${DIR_OBJ} -name *.o -exec rm -rf {} \;
