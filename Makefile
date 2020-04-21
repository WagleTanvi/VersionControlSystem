all: WTF WTFServer

WTF: ./client/wtfclient.c
	gcc -g ./client/wtfclient.c -o ./client/WTF -lssl -lcrypto

WTFServer: ./server/wtfserver.c
	gcc -g -pthread ./server/wtfserver.c -o ./server/WTFServer
	
clean:
	rm ./client/WTF ./server/WTFServer

gdbserver:
	gdb ./server/WTFServer

gdbclient:
	gdb ./client/WTF

runserver:
	./server/WTFServer 8006

create:
	./client/WTF create project

checkout:
	./client/WTF checkout projectTwo

destroy:
	./client/WTF destroy projectTwo
