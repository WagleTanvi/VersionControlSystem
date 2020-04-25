all: WTF WTFServer

WTF: ./client/wtfclient.c ./client/header-files/record.o ./client/header-files/helper.o
	gcc -g ./client/wtfclient.c ./client/header-files/record.o ./client/header-files/helper.o -o ./client/WTF -lssl -lcrypto

WTFServer: ./server/wtfserver.c ./server/header-files/record.o ./server/header-files/helper.o
	gcc -g -pthread ./server/wtfserver.c ./server/header-files/record.o ./server/header-files/helper.o -o ./server/WTFServer

./client/header-files/record.o: ./client/header-files/record.c
	gcc -c ./client/header-files/record.c -o ./client/header-files/record.o

./client/header-files/helper.o: ./client/header-files/helper.c
	gcc -c ./client/header-files/helper.c -o ./client/header-files/helper.o

./server/header-files/record.o: ./server/header-files/record.c
	gcc -c ./server/header-files/record.c -o ./server/header-files/record.o

./server/header-files/helper.o: ./server/header-files/helper.c
	gcc -c ./server/header-files/helper.c -o ./server/header-files/helper.o

clean:
	rm ./client/WTF ./client/header-files/record.o ./client/header-files/helper.o ./server/header-files/record.o ./server/header-files/helper.o ./server/WTFServer

gdbserver:
	gdb ./WTFServer

gdbclient:
	gdb ./WTF

runserver:
	./WTFServer 8006
