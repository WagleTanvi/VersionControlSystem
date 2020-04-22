all: WTF WTFServer

WTF: ./client/wtfclient.c
	gcc -g ./client/wtfclient.c -o ./client/WTF -lssl -lcrypto

WTFServer: ./server/wtfserver.c
	gcc -g -pthread ./server/wtfserver.c -o ./server/WTFServer
	
clean:
	rm ./client/WTF ./server/WTFServer

gdbserver:
	gdb ./WTFServer

gdbclient:
	gdb ./WTF

runserver:
	./WTFServer 8006
