CFLAGS=-g -Wall
LDFLAGS=-lcrypto -pthread

all: break_md5 

break_md5: break_md5.c
	gcc break_md5.c -o break_md5 $(CFLAGS) $(LDFLAGS) 

clean: 
	rm -f *.o break_md5
