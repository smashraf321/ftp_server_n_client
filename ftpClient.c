#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>


#define DEFAULT_PORT 8080
#define NEW_PORT1 12055
#define NEW_PORT2 12060
#define LIST_CMD "LIST"
#define RET_CMD "RET"
#define OK_ACK "OK"
#define DEBUG 0

//declarations
int readServer(int dataSock, struct sockaddr_in * data_addr,FILE *fileOut, char * cmd, char * fileName);

int startCmdSocket(struct sockaddr_in * serv_addr);
int startDataSocket(struct sockaddr_in * data_addr, char * cmd);

int parseCommandLine(int argc, char const *argv[], struct sockaddr_in * addr);

int main(int argc, char const *argv[])
{

    char buff[4096];
    int bufLen = 0;
    int cmdSock,dataSock;
    int quit = 0;
    struct sockaddr_in server_addr, data_addr;

    //read in server info from cmd line
    if( parseCommandLine(argc, argv, &server_addr) < 0)
    {
        perror("parseCommandLine");
        return -1;
    }





    //create dataSock


    char line[64] = {0};
    //user interaction loop
    while(!quit){

        //create serverSock
        if((cmdSock = startCmdSocket(&server_addr)) <= 0)
        {
            perror("creating cmdSock");
            return -1;
        }

        //enter command
        printf("\nenter command:\nLIST, RET <FILE>\n");

        //read line
        fgets(line, sizeof(line),stdin);

        //parse line
        char *token = strtok(line," \n");

       // printf("printing command\n");//debug

        int index = 0;
        int isList = 0;
        int isRet = 0;

        char cmd[5];
        char cmdStr[30];
        char fileName [20] = " ";

        //while loop to process user command
        while(token!=NULL){
            //printf("command: %s\n",token);

            if(index == 0){
                if(strcmp(token,LIST_CMD) == 0){
                    isList = 1;
                    strcpy(cmd,LIST_CMD);
                    printf("Running List\n");
                }
                else if(strcmp(token,RET_CMD) == 0){
                    isRet = 1;
                    strcpy(cmd,RET_CMD);
                    printf("Running ret\n");
                }
                else{
                    printf("Invalid command quitting\n");
                    token = NULL;//set to escape loop
                    quit = 1;
                    break;
                }
            }else if(index == 1 && isRet){
                //create file for ret
                strcpy(fileName,token);

            }else{
                printf("extra cmds ignored: %s\n",token);
            }

            token = strtok(NULL," \n");
            index++;

        }



        //create command packet
        if(isList == 1)
        {
            sprintf(cmdStr, "%s %d\r\n",cmd,NEW_PORT1);
        }
        else if(isRet == 1)
        {
            sprintf(cmdStr, "%s %s %d\r\n",cmd,fileName,NEW_PORT2);
        }
        else{
            continue;//restart the user interaction loop
        }

        //create data socket
        if((dataSock = startDataSocket(&data_addr,cmd))<0)
        {
            perror("creating dataSock");
            return -1;
        }


        printf("command to be sent: %s\n",cmdStr);

        //send command packet
        //write(cmdSock,cmdStr,sizeof(cmdStr));
        if(send(cmdSock,cmdStr,strlen(cmdStr),0) != strlen(cmdStr)) {
            perror("send cmd");
            return -1;
        }

        //wait for ok

        /*
        buff[0] = ' ';
        buff[1] = '\0';
        */

        //printf("\n buff now is: '%s' \n",buff);

        if( (bufLen = read(cmdSock,buff,sizeof(buff) ) ) < 0)
        {
            perror("read");
            return -1;
        }

        if(DEBUG)
            printf("\n WAITING FOR OK AND GOT: '%s' \n",buff);
        //check if server accepts command
        if(strcmp(buff,OK_ACK) != 0)
        {
            printf("command not ok: %s\n",buff);
            continue;
        }else{
            printf("command ok\n");
        }

        buff[0] = ' ';
        buff[1] = '\0';

        if(DEBUG)
            printf("\n buff now is: '%s' \n",buff);

        //call read server
        printf("\n--- start command ---\n");

        //use call server
        if(readServer(dataSock,&data_addr,stdout,cmd,fileName)<0){
            perror("readserver");
            exit(EXIT_FAILURE);
        }

        printf("\n--- end command ---\n");
        close(dataSock);

        close(cmdSock);

    }//end user interaction loop



    //close(cmdSock);
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

int startDataSocket(struct sockaddr_in * data_addr, char * cmd){

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
        //perror("setsockopt non nessary failure");
        //exit(EXIT_FAILURE);
    }


    //assign address localhost:DEFAULT_DATA_PORT
    data_addr->sin_family = AF_INET;

    if(strcmp(cmd,LIST_CMD)==0)
        data_addr->sin_port = htons(NEW_PORT1);
    if(strcmp(cmd,RET_CMD)==0)
        data_addr->sin_port = htons(NEW_PORT2);

    data_addr->sin_addr.s_addr = INADDR_ANY;

    //force attacment to default data port
    if (bind(sock, (struct sockaddr *)data_addr,sizeof(*data_addr))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 3) < 0)
    {
        perror("listen");
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
int readServer(int dataSock, struct sockaddr_in * data_addr, FILE *fileOut, char * cmd, char * fileName)
{
    char buff[4096];
    char *ackStr = "ACK";
    char *doneStr = "DONE";
    int data_addr_len = sizeof(*data_addr);
    int isList = 0, isRet = 0;

    //set up data socket
    int new_socket;

    if(DEBUG)
        printf("\n waiting for server to connect to me \n");

    if ( ( new_socket = accept(dataSock, (struct sockaddr *)data_addr, (socklen_t*)&data_addr_len) ) <0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    if(DEBUG)
        printf("\n server connected to me \n");
    //start reading data
    int done = 0;
    int valread;

    if(strcmp(cmd,LIST_CMD)==0){
        isList = 1;
    }

    //create file for RET comand
    if(strcmp(cmd,RET_CMD)==0)
    {
        if(DEBUG)
            printf("\n waiting for receiving and saving file '%s' \n",fileName);

        fileOut = fopen(fileName, "w");
        if(fileOut < 0)
        {
          printf("\n failed to open new file \n");
          return -1;
        }
        isRet = 1;
    }


    while(!done)
    {
        //read buffer
        if( (valread = read(new_socket,buff,sizeof(buff) ) ) < 0)
        {
            perror("read");
            return -1;
        }



        //printf("read from server: %.*s\n",valread,buff);
        //printf("read %d bytes\n",valread);
        //fflush(stdout);

        //send acknologement
        if(send(new_socket,ackStr,strlen(ackStr),0) != strlen(ackStr)) {
            perror("send");
            return -1;
        }

        //check if done .. valread == 0 ||
        if(valread == 0 || strcmp(buff, doneStr) == 0){
            done = 1;
            if(DEBUG)
                printf("%.*s<- not printed to file\nclient recieved done\n",valread,buff);
            continue;
        }

        //print to stdout
        if(isList || DEBUG)
            fprintf(stdout, "%.*s",valread,buff);
        //could be better solution to assign fileOut pointer
        if(isRet){
           fwrite(buff,sizeof(char),(size_t)valread,fileOut);
           //fprintf(fileOut, "%.*s",valread,buff);
        }
    }

    if(isRet)
        printf("saved file as %s\n",fileName);

    //close open file
    if(strcmp(cmd,RET_CMD)==0){
        fclose (fileOut);
    }
    close(new_socket);
    return 0;
}
