# C compiler
CC = gcc
CFLAGS = -g -lm -lpthread

all:
	$(CC) -c $(CFLAGS) TLS_const.c;
	$(CC) -c $(CFLAGS) TLS_msg.c;
	$(CC) -c $(CFLAGS) TLS_mmcmp.c;
	$(CC) -c $(CFLAGS) TLS_iarray.c;
	$(CC) -c $(CFLAGS) TLS_mmap.c;
	$(CC) -c $(CFLAGS) TLS_mconvert.c;
	$(CC) -c $(CFLAGS) TLS_timer.c;
	$(CC) -c $(CFLAGS) TLS_mutex.c;
	$(CC) -c $(CFLAGS) TLS.c;
	$(CC) -c $(CFLAGS) TLS_pred.c;
	ar -r TLS.a TLS_const.o TLS_mutex.o TLS_mmcmp.o TLS.o TLS_iarray.o TLS_mmap.o TLS_mconvert.o TLS_timer.o TLS_msg.o TLS_pred.o;
	gcc -shared -o libTLS.so TLS_const.o TLS_mutex.o TLS_mmcmp.o TLS.o TLS_iarray.o TLS_mmap.o TLS_mconvert.o TLS_timer.o TLS_msg.o TLS_pred.o;
	cp libTLS.so /usr/lib/. -f;

TLS_const:
	$(CC) -c $(CFLAGS) TLS_const.c		
TLS_mutex:
	$(CC) -c $(CFLAGS) TLS_mutex.c
TLS_msg:
	$(CC) -c $(CFLAGS) TLS_msg.c
TLS_mmcmp:
	$(CC) -c $(CFLAGS) TLS_mmcmp.c
TLS_iarray:
	$(CC) -c $(CFLAGS) TLS_iarray.c
TLS_mmap:
	$(CC) -c $(CFLAGS) TLS_mmap.c
TLS_mconvert:	
	$(CC) -c $(CFLAGS) TLS_mconvert.c
TLS_timer:
	$(CC) -c $(CFLAGS) TLS_timer.c
TLS_pred:
	$(CC) -c $(CFLAGS) TLS_pred.c
TLS:
	$(CC) -c $(CFLAGS) TLS.c

clean:
	rm *.o *.a *~ *.so -f
	rm /usr/lib/libTLS.so -f
