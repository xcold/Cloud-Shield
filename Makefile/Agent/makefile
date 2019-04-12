DIR_OBJ = ./obj
DIR_SRC = ./src

SRC = $(wildcard ${DIR_SRC}/*.c ${DIR_SRC}/AES/*.c ${DIR_SRC}/AgentManage/*.c ${DIR_SRC}/cJSON/*.c ${DIR_SRC}/Http/*.c ${DIR_SRC}/Log/*.c ${DIR_SRC}/Plugin/*.c ${DIR_SRC}/SolveMsg/*.c ${DIR_SRC}/Tcp/*.c ${DIR_SRC}/ThreadPool/*.c ${DIR_SRC}/Security/*.c)
OBJ = $(patsubst %.c, ${DIR_OBJ}/%.o, $(notdir $(SRC)))
INC = $(patsubst %, -I%, $(shell find src -name '[a-zA-Z0-9]*'.h))

BIN_TARGET = DyAgent

CC = gcc
CFLAGS = -g -Wall $(INC)

${BIN_TARGET}: ${OBJ}
	$(CC) $(OBJ) -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/AES/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

#main.c
${DIR_OBJ}/%.o: ${DIR_SRC}/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/AgentManage/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/cJSON/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/Http/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/Log/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/Plugin/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/SolveMsg/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/Tcp/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/ThreadPool/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt

${DIR_OBJ}/%.o: ${DIR_SRC}/Security/%.c
	$(CC) $(CFLAGS) -c $< -o $@ -lpthread -lrt -lm -lcrypt


.PHONY:clean
clean:
	find ${DIR_OBJ} -name *.o -exec rm -rf {} \;
