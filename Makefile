all: WTF WTFServer

WTF: wtfclient.c
	gcc -g wtfclient.c -o WTF

WTFServer: wtfserver.c
	gcc -g -pthread wtfserver.c -o WTFServer
	
clean:
	rm WTF WTFServer