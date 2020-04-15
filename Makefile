all: WTF WTFServer

WTF: ./client/wtfclient.c
	gcc -g ./client/wtfclient.c -o WTF

WTFServer: ./server/wtfserver.c
	gcc -g -pthread ./server/wtfserver.c -o WTFServer
	
clean:
	rm WTF WTFServer