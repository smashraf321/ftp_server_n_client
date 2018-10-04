#include <stdio.h> 
#include <sys/socket.h> 
#include <stdlib.h> 
#include <netinet/in.h> 
#include <string.h> 
#include <unistd.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <netdb.h>
#define DEFAULT_PORT 8080
#define DEFAULT_DATA_PORT 9001
   
//declarations
int readServer(int dataSock, struct sockaddr_in * data_addr, int fileOut);

int startCmdSocket(struct sockaddr_in * serv_addr);
int startDataSocket(struct sockaddr_in * data_addr);

int parseCommandLine(int argc, char const *argv[], struct sockaddr_in * addr);

int main(int argc, char const *argv[]) 
{ 

    char buff[1024];
    int bufLen = 0;
    int cmdSock,dataSock;

    struct sockaddr_in server_addr, data_addr;
    //read in server info from cmd line
    if( parseCommandLine(argc, argv, &server_addr) < 0)
    {
        perror("parseCommandLine");
        return -1;
    }

    //create serverSock
    if((cmdSock = startCmdSocket(&server_addr)) <= 0)
    {
        perror("creating cmdSock");
        return -1;
    }

    //create dataSock 
    if((dataSock = startDataSocket(&data_addr))<0)
    {
        perror("creating dataSock");
        return -1;
    }
    
    //command loop
    while(1){

    }
    //start server call
    //readServer(cmdSock,buff,&bufLen);
    printf("buff\n");
    printf("buff: %s\n",buff);


    close(cmdSock);
    return 0;
} 



int parseCommandLine(int argc, char const *argv[], struct sockaddr_in *addr)
{
    int index = 1;
    while(index < argc)
    {
        char arg[50]; //gets individual argument
        strcpy(arg, argv[index]);
        char serverId[50];
        char portStr[6];
        int portNum;
        int portSection = 0;
        char * ipStr;
        for(int i = 0; arg[i] != '\0' && i<sizeof(serverId); i++)
        {
            if(arg[i] == ':')
            {
                portSection = i + 1;
                serverId[i] ='\0';
                i++;//skip colon
            }

            if(portSection == 0)
            {
                serverId[i] = arg[i];
            }
            else if((i-portSection) < sizeof(portStr) - 1)
            {
                portStr[i - portSection] = arg[i];
            }
            else
            {
                portStr[sizeof(portStr)-1] = '\0';
            }
        }

        if(portSection == 0)
        {
            strcpy(portStr, "N/A");
            portNum = DEFAULT_PORT;
        }
        else
        {
            char *endptr;
            portNum = strtoimax(portStr,&endptr,10);
        }
        printf("server%d\n",index);
        printf("serverId: %s\t",serverId);
        //printf("portStr: %s\t",portStr);
        printf("portNum: %d\n",portNum);

        //create sockaddr struct
        struct hostent * ent_addr;

        
        //try by hostname
        ent_addr = gethostbyname(serverId);
        

        
        printf("hostname: %s\t",ent_addr->h_name);
        ipStr = inet_ntoa( *( (struct in_addr *) ent_addr->h_addr_list[0]) );
        printf("ipString: %s\n",  ipStr );


        memset(addr, '0',sizeof(*addr));
        //create address
        addr->sin_family = AF_INET;
        addr->sin_port = htons(portNum);
        
        // Convert IPv4 and IPv6 addresses from text to binary form 
        if(inet_pton(AF_INET, ipStr, &addr->sin_addr)<=0)  
        { 
            printf("\nInvalid address/ Address not supported \n"); 
            return -1; 
        } 
        
        printf("\n");
        index++;//move to next argument
    }
    return 0;
}

int startDataSocket(struct sockaddr_in * data_addr){

    int sock;
    int opt = 1; 

    //start socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        exit(EXIT_FAILURE);  
    }

    // Set socket options 
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) 
    { 
        perror("setsockopt"); 
        exit(EXIT_FAILURE); 
    } 

    //assign address localhost:DEFAULT_DATA_PORT
    data_addr->sin_family = AF_INET;
    data_addr->sin_port = htons(DEFAULT_DATA_PORT);
    data_addr->sin_addr.s_addr = INADDR_ANY;

    //force attacment to default data port
    if (bind(sock, (struct sockaddr *)data_addr,sizeof(*data_addr))<0) 
    { 
        perror("bind failed"); 
        exit(EXIT_FAILURE); 
    } 
    return sock;
}
//returns created socket file descriptor -1 if error
int startCmdSocket(struct sockaddr_in * serv_addr)
{
    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) 
    { 
        printf("\n Socket creation error \n"); 
        return -1; 
    }
   
    if (connect(sock, (struct sockaddr *)serv_addr, sizeof(*serv_addr)) < 0) 
    { 
        printf("\nConnection Failed \n"); 
        return -1; 
    } 

    return sock;
}
int readServer(int dataSock, struct sockaddr_in * data_addr, int fileOut)
{
    char buff[4096];
    char *ack = "ACK";
    int data_addr_len = sizeof(*data_addr);

    //set up data socket
    int new_socket;
    
    if (listen(dataSock, 3) < 0) 
    { 
        perror("listen"); 
        exit(EXIT_FAILURE); 
    } 
    if ( ( new_socket = accept(dataSock, (struct sockaddr *)data_addr, (socklen_t*)&data_addr_len) ) <0) 
    { 
        perror("accept"); 
        exit(EXIT_FAILURE); 
    } 

    //start reading data
    int done = 0;
    int valread;
    while(!done){
        //read buffer
        if( (valread = read(new_socket,buff,sizeof(buff) ) ) < 0)
        {
            perror("read");
            done = 1;
            continue;
        }

        //check if done
        if(valread == 0 || strcmp(buff, ack)){
            done = 1;
            continue;
        }
        //send acknologement
        if(send(new_socket,ack,strlen(ack),0) != strlen(ack)) {
            perror("send");
            return -1;
        }
        
    }

    close(new_socket);
    return 0;
}







