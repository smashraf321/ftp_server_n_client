#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define client_request_list "LIST"
#define client_request_ret "RET"
#define valid_request "OK"
#define invalid_request "Right now the server supports only 2 commands, LIST and RETR, please try again"
#define done "DONE"

int main(int argc, char* argv[])
{

 FILE* p; // file pointer for the stream to a process opened using popen
 int fd; // file descriptor for sending a file
 int control_sock; // socket identifier for the control socket to which the server is bound
 int list_sock, data_sock; // socket identifiers for sending list info and data to client
 int acc, client_len; // identifier for the accept connection from client and client_len to store the client structure when he pings the server
 int n_req; // for storing size of request message from client
 int n_sent; // for storing length of data read from requested file
 int i,j,k,lll,sp_cnt,opt; // support variables
 char ack[5]; // support variable
 //char ch; // support variable for alternate way of reading file
 struct sockaddr_in server, server_send, client, client_list, client_data; // socket structures for storing info on 2 server sockets and 3 sockets for clients.
 /*
 client is for when client pings the server,
 client_list is for when client wants to view the list of files in server,
 client_data is for when we're sending the requested file to client
 */
 /*
 DIR *mydir;
 struct dirent *myfile;
 struct stat mystat;
 char buf[512];
 */
 struct hostent* cmd_addr; // structure for storing socket structure informaqtion when IP address/name of server is entered from command line

 if(argc == 1)
 {
	 fprintf(stderr,"\n Please enter the IP address or host name of the server, i.e the PC you're working on right now \n");
	 return -1;
 }

 char temp_request[35] = " "; // for storing parsed request information from client like LIST, RET, etc.
 char port_no[8] = " "; // for storing port number sent by client
 char client_request[50] = " "; // for storing actual request message of client
 char dir_list[20] = " "; // buffer for sending list of files to client
 char data_buffer[4096] = " "; // buffer for sending requested file

 client_len = sizeof(client);

 // initializing the server socket info
 server.sin_family = AF_INET;
 server_send.sin_family = AF_INET;

 server.sin_port = htons(14085);
 server_send.sin_port = htons(14084);

 // Server will connect to client twice, for sending list of files and data. Therefore, we're defining the client_list and client_data structures
 client_list.sin_family = AF_INET;
 client_data.sin_family = AF_INET;

 cmd_addr = gethostbyname(argv[1]); //IP address of server taken from command line

 if(!(cmd_addr == NULL))
	 {
		 printf("\n host name/ip address is: %s \n",inet_ntoa(*((struct in_addr*) cmd_addr -> h_addr_list[0])));
		 server.sin_addr = *((struct in_addr*) (cmd_addr -> h_addr_list[0])); // IP address of server is stored in server socket structure
		 server_send.sin_addr = *((struct in_addr*) (cmd_addr -> h_addr_list[0])); // IP address of server is stored in server socket structure
	 }
	 else
	 {
     perror("\n Failed to retrieve IP Address \n");
     return -1;
	 }

 // control socket of server is now set up
 control_sock = socket(AF_INET, SOCK_STREAM, 0);
 if(control_sock < 0)
 {
	 perror("\n socket() failed \n");
	 return -1;
 }

 // for reusing socket without an issue
 setsockopt(control_sock,SOL_SOCKET,SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

 // server binds the required port and IP address to server socket
 if(bind(control_sock, (struct sockaddr*) &server, sizeof(server)) < 0)
 {
	 perror("\n bind() failed \n");
	 return -1;
 }

 // informs the OS the socket is for listening for connections
 if(listen(control_sock, 100) < 0 )
 {
	 perror("\n listen() failed \n");
	 return -1;
 }

 //*******************************************************************************************************************************************
 while(1)
 {
           printf("\n Waiting for client to ping me on control socket \n");
           // accepts the client's connection and stores client's information in client structure
           if((acc = accept(control_sock, (struct sockaddr*) &client, (socklen_t *) &client_len)) < 0)
           // get the client's IP address and port from this ^ client structure.
           {
             perror("\n accept failed \n");
             return -1;
           }

           printf("\n accepted the client's connection \n");

           printf("\n now reading the client's request \n");

           if((n_req = read(acc, client_request, 50)) < 0) // reads the client's request message
           {
        	 perror("\n read() failed \n");
        	 return -1;
           }

           printf("\n Client's request of size %d was: \n %s \n",n_req,client_request);

           for (i = 0; client_request[i] != ' ' ; i++) // storing client's request type in buffer, like LIST, RET...
            temp_request[i] = client_request[i];
           temp_request[i] = '\0';

           printf("\n Client requested for: %s \n",temp_request);

   //---------------------------------------------------------------------------------------------------------------------------------------
   // if client requests for LIST
           if(strcmp(temp_request,client_request_list) == 0)
           {
                               printf("\n Gonna send ok to client \n");

                               write(acc, valid_request, 3 ); // sends OK to client

                               printf("\n sent ok to client \n");

                               client_list.sin_addr.s_addr = client.sin_addr.s_addr; // get IP address info from client and store it in IP Address part of client_list

                            	 // now get port number from client_request and store it in port number part of client_list
                            	 sp_cnt = 0;
                            	 for (i = 0, j = 0; client_request[i] != '\r' ; i++)
                            	 {
                            		if (client_request[i] == ' ')
                            			sp_cnt++;

                            		if(sp_cnt == 1)
                            		{
                            			port_no[j] = client_request[i];
                            			j++;
                            		}
                            	 }
                                 port_no[j] = '\0';

                            	 // storing port info in port field of client_list
                            	 client_list.sin_port = htons(atoi(port_no));

                            	 printf("\n Client's wants to receive file list on port: '%d' \n",ntohs(client_list.sin_port));

                            	 // open a socket for sending file list info
                            	 if( (list_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                            	 {
                            		 perror("\n Failed to create socket for sending file list info\n");
                            		 return -1;
                            	 }

                            	 // connect to socket on client for sending file list info
                            	 if(connect(list_sock,(struct sockaddr*) &client_list, sizeof(client_list)) < 0)
                            	 {
                            		 perror("\n Failed to connect to socket for sending file list info\n");
                            		 return -1;
                            	 }

                               p = popen("ls","r"); // pipe the stream from ls command
                               while(fgets(dir_list, sizeof(dir_list), p) != NULL)
                               {
                          		   write(list_sock, dir_list, sizeof(dir_list)); // send the file list to client
                          		   read(list_sock, ack, 4); // wait for acknowledgment
                               }
                               /*
                               mydir = opendir(".");
                               while((myfile = readdir(mydir)) != NULL)
                               {
                                  sprintf(buf, "%s/%s", ".", myfile->d_name);
                                  stat(buf, &mystat);

                                  for(i = 0; myfile->d_name[i] != '\0'; i++)
                                  {
                                    dir_list[i] = myfile->d_name[i];
                                  }
                                  dir_list[i] = '\0';
                                  printf(" %s\n", dir_list);
                                  write(list_sock,dir_list,sizeof(dir_list));
                                  read(list_sock, ack, 4); // wait for acknowledgment

                               }*/
                               printf("\n Done sending ls info \n");
                               write(list_sock,done,sizeof(done));
                               pclose(p);
                               //closedir(mydir);
                            	 close(list_sock); // close the socket for sending file list

            }

   //----------------------------------------------------------------------------------------------------------------------------------------
   // if client requests for RET
         else if(strcmp(temp_request,client_request_ret) == 0)
             {

                               printf("\n Gonna send ok to client \n");

                               write(acc, valid_request, 3 ); // sends OK to client

                               printf("\n sent ok to client \n");

                               client_data.sin_addr.s_addr = client.sin_addr.s_addr; // get IP address info from client and store it in IP Address part of client_data

                            	 // now get file name and port number from client and store it in port number part of client_data
                            	 sp_cnt = 0;
                            	 for (i = 0, j = 0, k = 0; client_request[i] != '\r' ; i++)
                            	 {
                            		if (client_request[i] == ' ')
                            			sp_cnt++;

                            		if(sp_cnt == 1)
                            		{
                            			temp_request[j] = client_request[i+1];
                            			j++;
                            		}

                            		if(sp_cnt == 2)
                            		{
                            			port_no[k] = client_request[i];
                            			k++;
                            		}
                            	 }
                                 temp_request[j-1] = '\0';
                            	 port_no[k] = '\0';

                            	 // storing port info in port field of client_list
                            	 client_data.sin_port = htons(atoi(port_no));

                            	 printf("\n The file name Client requested for: '%s' \n",temp_request);
                            	 printf("\n Client's wants to receive file on port: '%d' \n",ntohs(client_data.sin_port));

                            	 // open a socket for sending file list info
                            	 if( (data_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
                            	 {
                            		 perror("\n Failed to create socket for sending file \n");
                            		 return -1;
                            	 }

                            	 // server is bound since it has to send file only on a specified port, 14084 in our case
                            	 if(bind(data_sock, (struct sockaddr*) &server_send, sizeof(server_send)) < 0)
                            	 {
                            		 perror("\n bind() failed \n");
                            		 return -1;
                            	 }

                            	 // connect to socket on client for sending requested file
                            	 if(connect(data_sock,(struct sockaddr*) &client_data, sizeof(client_data)) < 0)
                            	 {
                            		 perror("\n Failed to connect to socket for sending requested file \n");
                            		 return -1;
                            	 }
                            	/*
                                 p = fopen(temp_request,"r"); // read from requested file
                              */
                              /*
                                 while(fgets(data_buffer, sizeof(data_buffer), p) != NULL)
                            	 {
                            		 write(data_sock, data_buffer, 4096); // send the file list to client
                            	 }
                            	*/
                            	 if( (fd = open(temp_request, O_RDWR, S_IRUSR | S_IWUSR | S_IXUSR )) < 0)
                            	 {
                            		 perror("\n Failed to open requested file \n");
                            		 return -1;
                            	 }
                               lll = 1;
                            	 while( (n_sent = read(fd, data_buffer, sizeof(data_buffer))) > 0) // reading from requested file
                            	 {
                                 printf(" sending packet %d \n",lll );
                            		 write(data_sock, data_buffer, n_sent); // send the file list to client

                            		 read(data_sock, ack, 4); // get acknowledgement from client
                                 usleep(50000);
                                 lll++;
                            	 }

                            	 /* alternative way of reading from file
                            	 i = 0;
                            	 while(!feof(p))
                            	 {
                            		 ch = getc(p);

                            		 if(ch == EOF)
                            		 {
                            			 printf("\n error occured when reading from file\n");
                            			 break;
                            		 }

                            		 data_buffer[i] = ch;
                            		 i++;

                            		 if((i+1)%4096 == 0)
                            		 {
                            			 write(data_sock, data_buffer, 4096); // send the file list to client
                            			 i = 0;
                            		 }
                            	 }
                            	 write(data_sock, data_buffer, 4096); // send the file list to client
                            	 */

                            	 //fclose(p); // close the file to be sent

                               printf("\n Done sending ls info \n");
                               write(list_sock,done,sizeof(done));

                            	 close (fd); // close the file descriptor for the sent file

                            	 close(data_sock); // close the socket for sending file

               }

               else
               {
                               write(acc, invalid_request, 120 );
                               printf("\n Right now the server supports only 2 commands, LIST and RETR, please try again \n");
               }

               close(acc);

               temp_request[0] = ' ';
               temp_request[1] = '\0';

               client_request[0] = ' ';
               client_request[1] = '\0';
 }

 //close(acc);
 close(control_sock);

 return 0;
}
