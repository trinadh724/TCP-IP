// Client side C/C++ program to demonstrate Socket programming
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <signal.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

#define PORT 8000
#define bufflen 12000
int sock, valread;
char buffer[bufflen] = {0};

void pipebroke()
{
    printf("\nBroken pipe: write to pipe with no readers\n");
    exit(EXIT_FAILURE);
}

void exithandler()
{
    printf("\nExiting execution abruptly....\n");
    exit(EXIT_FAILURE);
}

int copythecontent(const char* arg)
{
    // printf("%s\n",arg);
       
    memset(&buffer,'\0',bufflen);                   // setting all the charqacters in string to "\0"
    strcpy(buffer,arg);
    if(send(sock,buffer,bufflen,0)<0)               // Sending the name of the file to server.
    {
        printf("\n%s : sending the file name to the server side failed\n",arg);
        perror("send failed");
        return -1;
    }
        memset(&buffer,'\0',bufflen);

    if(recv(sock,buffer,bufflen,0)<0)
    {
        printf("\n%s: Knowing the status of the file on server side failed\n",arg);
        perror("recv failed");
        return -1;
    }
    if(strcmp(buffer,"Success")==0)                 // Success will sent by the server if all 
    {
                                                    // the conditions which are required for file copying in server end are done. Else it will send Failure.
        printf("\n%s is ready to be copied\n",arg);
    }
    else if(strcmp(buffer,"Failure")==0)
    {
        printf("\n%s : Failure status is sent to client by server\n",arg);
        return -1;
    }
    else
    {
        printf("\nUnexpected messaege recived maybe becasue the SERVER was shut down\n");
        return -1;
    }
    
    memset(&buffer,'\0',bufflen);
    strcpy(buffer,"GETSIZE");                        // Once we are sure if the file can be transfered this GETSIZE will be called if the client want to ask the server the size of the file requested for.
    if(send(sock,buffer,bufflen,0)<0)
    {
        printf("\n%s : Attmept to ask the size of the file\n",arg);
        perror("send failed");
        return -1;
    }
    memset(&buffer,'\0',bufflen);
    if(recv(sock,buffer,bufflen,0)<0)
    {
        printf("\n%s : Failed to recieve the file size\n",arg);
        perror("recv failed");
        return -1;
    }
    long long totsize,currsize=0;                     // totalsize denotes the file size. currentize denotes the number of bytes the copying of the file is done.
    sscanf(buffer,"%lld",&totsize);                   // Converting string into long long integer.
    FILE* fd = fopen(arg,"w");                        // Opening a file with write permissions if there 
    if(fd==NULL)                                      // is no file then we will create one with the same
    {
                                                      // name.
        printf("\n%s : failed to open the required file\n",arg);
        perror("fopen failed");
        
        memset(&buffer,'\0',bufflen);                 
        strcpy(buffer,"FILENO");
        if(send(sock,buffer,bufflen,0)<0)
        {
            printf("\n%s : Failed to send server that this file is failed to open\n",arg);
            perror("send failed");
        }
        return -1;
    }
    else
    {
        memset(&buffer,'\0',bufflen);                 
        strcpy(buffer,"FILEYES");
        if(send(sock,buffer,bufflen,0)<0)
        {
            printf("\n%s : Failed to send server that this file opened\n",arg);
            perror("send failed");
        }
    }
    
    printf("\n%s : Downloading file......\n",arg);
    // printf("%lld\n",totsize);
    while(currsize<totsize)                           // Checking if all the contents are copied.
    {
        memset(&buffer,'\0',bufflen);                  
        int val = recv(sock,buffer,bufflen,0);        // recieving a bufflen characters from server.
        if(val<0)                                     // checking if it recieved something.
        {
            printf("\n%s : unable to get any string from server\n",arg);
            // perror("recv failed");
            break;
        }
        if(val==0)
        {
            printf("\n%s : Didnt receive any bytes from server side in current round of recieving\n",arg);
            // printf("Exiting abruptly (One reason is server is shutdown)\n");
            break;
        }
        // printf("%d\n",val);
        int val1 = fwrite(buffer,sizeof(char),val,fd);
        // printf("%d\n",val1);
        if(val1!=val)                                  // Checking if all the contents recieved is 
        {                                              // written to the file.
            printf("\n%s : fwrite didnt write the complete contents to file\n",arg);
            break;
        }
        currsize+=val;                                  // Increased the total number fo bytes written.
        printf("\rDownloading Percentage: %Lf%%",((long double)100*currsize/totsize));
        if(currsize>=totsize)printf("\n");              // If all the bytes are written then I just kept end line.
    }
    if(currsize==totsize)
    {
        printf("\n%s : Downloaded sucessfully\n",arg);
        memset(&buffer,'\0',bufflen);
        strcpy(buffer,"ACK");                  
        if(send(sock,buffer,bufflen,0)<0)
        {
            printf("\n%s : unable to send ACK ACKNOWLEDGEMENT to server\n",arg);
            perror("send failed");
        }
    }
    else
    {
        printf("\n%s : Download stopped abruptly\n",arg);
        memset(&buffer,'\0',bufflen);
        strcpy(buffer,"FAIL");                  
        if(send(sock,buffer,bufflen,0)<0)
        {
            printf("\n%s : unable to send FAIL ACKNOWLEDGEMENT to server\n",arg);
            perror("send failed");
        }
    }
    
    // printf("%lld\n",currsize);
    fclose(fd);
    return 0;
}

void getme(int argc, char const *argv[])
{
    // if(argc>1)
    // printf("Starting to copy files...\n");
    if(argc<=1)
    {
        printf("\n\nOOPS... No files are given!!\n\n");
        return ;
    }
    
    for(int i = 1 ;i < argc ; i++)                      // Iterating through indices of the file
    {
        // printf("%s\n",argv[i]);
        if(access(argv[i],F_OK)==-1)                    // Checking if there is a file with same name.
        {
            copythecontent(argv[i]);                        
        }
        else
        {
            printf("\n\n%s : already exists\n",argv[i]);
            printf(">>Press 1 to overwrite else will be skipped\n");
            int num;
            printf("Your choice>>  ");
            scanf("%d",&num);                           // This is the response of the user.
            if(num!=1)
            {
                printf("\nSkipping the file: %s .......\n",argv[i]);
            }
            else
            {
                copythecontent(argv[i]);
            }
            
        }    
    }
    memset(&buffer,'\0',bufflen);                       // Once iterating through all the file is done.
    strcpy(buffer,"NOMORETRINADH");                     // I will send a signal to server that all the  files are done. SO that server will know that this clients requirements are done.
    if(send(sock,buffer,bufflen,0)<0)                   // checking if the send failed.
    {
        printf("\nSignal that there are no more files left is failed to send\n");
        perror("send failed");
    }
    else
    {
        printf("\nSignal that there are no more files left to get copied is sent to the server\n");
    }
}

void handler()
{
    printf("\nexiting.....for checking purposes.\n");
    exit(EXIT_FAILURE);
}
int main(int argc, char const *argv[])
{
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    // signal(SIGTSTP,handler);
    struct sockaddr_in address;
    // int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char *hello = "Hello from client";
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }
    // printf("%d\n",sock);
    memset(&serv_addr, '0', sizeof(serv_addr)); // to make sure the struct is empty. Essentially sets sin_zero as 0
                                                // which is meant to be, and rest is defined below

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Converts an IP address in numbers-and-dots notation into either a 
    // struct in_addr or a struct in6_addr depending on whether you specify AF_INET or AF_INET6.
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)  // connect to the server address
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    // printf("%d\n",sock);
    getme(argc,argv);
    return 0;
}
