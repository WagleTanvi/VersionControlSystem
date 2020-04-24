all: WTF WTFServer

WTF: ./client/wtfclient.c ./client/record.o
	gcc -g ./client/wtfclient.c ./client/record.o -o ./client/WTF -lssl -lcrypto

WTFServer: ./server/wtfserver.c ./server/record.o
	gcc -g -pthread ./server/wtfserver.c ./server/record.o -o ./server/WTFServer

./client/record.o: ./client/record.c
	gcc -c ./client/record.c -o ./client/record.o

./server/record.o: ./server/record.c
	gcc -c ./server/record.c -o ./server/record.o

clean:
	rm ./client/WTF ./client/record.o ./server/record.o ./server/WTFServer

gdbserver:
	gdb ./WTFServer

gdbclient:
	gdb ./WTF

runserver:
	./WTFServer 8006
