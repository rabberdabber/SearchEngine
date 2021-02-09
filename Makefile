SERVER_SRC = server.c
SERVER_AUX1 = helper.c
SERVER_AUX2 = thread_pool.c
SERVER_AUX3 = tc_malloc.c
SERVER_HDR1 = helper.h
SERVER_HDR2 = common.h
SERVER_HDR3 = thread_pool.h
SERVER_HDR4 = tc_malloc.h

CLIENT1_SRC = client1.c

CLIENT2_SRC = client2.c

##############################

CC=gcc
CFLAGS= -g -Wall -pthread

build: server client1 client2

server: $(SERVER_SRC) $(SERVER_HDR1) $(SERVER_HDR2) $(SERVER_HDR3) $(SERVER_HDR4) 
	$(CC) $(CFLAGS) -o $@ $(SERVER_SRC) $(SERVER_AUX1) $(SERVER_AUX2) $(SERVER_AUX3) 

client1: $(CLIENT1_SRC) $(SERVER_HDR1) $(SERVER_HDR2) $(SERVER_HDR4)
	$(CC) $(CFLAGS) -o $@ $(CLIENT1_SRC) $(SERVER_AUX1) $(SERVER_AUX3) 

client2: $(CLIENT2_SRC) $(SERVER_HDR1) $(SERVER_HDR2) $(SERVER_HDR4)
	$(CC) $(CFLAGS) -o $@ $(CLIENT2_SRC) $(SERVER_AUX1) $(SERVER_AUX3) 


clean:
	rm server client1 client2
