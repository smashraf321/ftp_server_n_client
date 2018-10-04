All: ftpClient ftpServer

ftpClient: ftpClient.c
	gcc -Wall -g -o ftpClient ftpClient.c

ftpServer: ftpServer.c
	gcc -Wall -g -o ftpServer ftpServer.c

clean:
	rm -f ftpServer ftpClient
