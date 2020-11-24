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
#include <sys/stat.h>

#define PORT 8000
#define bufflen 12000

char buffer[bufflen] = {0};
char buff[bufflen]={0};
int server_fd, new_socket, valread;

void pipebroke()
{
    printf("\nBroken pipe: write to pipe with no readers\n");
}

void exithandler()
{
    printf("\nExiting....\n");
    exit(EXIT_FAILURE);
}

int sendfilecontent()
{
    memset(&buffer,'\0',bufflen);
    if(recv(new_socket,buffer,bufflen,0)<0)
    {
        printf("Failed to recieve the signal from client\n");
        perror("recv failed");
        return -1;
    }
    if(strcmp(buffer,"GETSIZE")==0)
    {
        struct stat st;
        if(stat(buff,&st)<0)
        {
            printf("Error while trying to find the structure of the passed file\n");
            perror(("stat"));
            return -1;
        }
        long long totsize=st.st_size;
        // printf("%lld\n",totsize);
        memset(&buffer,'\0',bufflen);
        sprintf(buffer,"%lld",totsize);
        if(send(new_socket,buffer,bufflen,0)<0)
        {
            printf("Failed to send the total size of the file: %s\n",buff);
            perror("send failed");
            return -1;
        }
        memset(&buffer,'\0',bufflen);                 
        if(recv(new_socket,buffer,bufflen,0)<0)
        {
            perror("send failed");
            return -1;
        }
        if(strcmp("FILENO",buffer)==0)
        {
            printf("%s : Was not able to open in client side\n",buff);
            return -1;
        }
        else if(strcmp("FILEYES",buffer)==0)
        {
            
        }
        else
        {
            printf("OOPS!! Didnt expect this message to be passed!\n");
            return -1;
        }
        
        FILE* fd=fopen(buff,"r");
        if(fd==NULL)
        {
            perror("fopen failed");
            return -1;
        }
        long long currsize=0;
        printf("%s : Transmitting contents to client\n",buff);
        while(currsize<totsize)
        {
            memset(&buffer,'\0',bufflen);
            int val1 = fread(buffer,1,bufflen,fd);
            // printf("%d\n",val1);
            if(send(new_socket,buffer,val1,0)<0)
            {
                printf("\n%s : Failed to send the contents of the file\n",buff);
                break;
            }
            currsize+=val1;
            printf("\rSending Percentage: %Lf%%",((long double)100*currsize/totsize));
            if(currsize>=totsize)printf("\n");              // If all the bytes are written then I just kept end line.
        }
        memset(&buffer,'\0',bufflen);
        if(recv(new_socket,buffer,bufflen,0)<0)
        {
            printf("%s : Failed to recieve the acknowledgement\n",buff);
            perror("recv error");
        }
        else
        {
            if(strcmp("ACK",buffer)==0)
            {
                printf("%s : Recieved Acknowledgement from the client that transmission is sucessfull\n",buff);
            }
            else if(strcmp("FAIL",buffer)==0)
            {
                printf("%s : Recieved Acknowledgement from the client that transmission is unsucessfull\n",buff);
            }
            else
            {
                printf("Unexpected message recieved maybe because the CLIENT was shut down\n");
            }
        }

        // printf("%lld\n",currsize);
    }
    else
    {
        printf("GETSIZE signal not recived\n");
        return -1;
    }
    
}

int checkfilestatus()
{
    if(access(buff,F_OK)==-1)
    {
        printf("%s : No such file exists\n",buff);
        return -1;
    }
    if(access(buff,R_OK)==-1)
    {
        printf("%s : Write permission is not there\n",buff);
        return -1;
    }
    return 0;
}

int main(int argc, char const *argv[])
{
    signal(SIGPIPE,pipebroke);
    signal(SIGINT,exithandler);
    struct sockaddr_in address;  
    int opt = 1;
    int addrlen = sizeof(address);
    char *hello = "Hello from server";

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)  // creates socket, SOCK_STREAM is for TCP. SOCK_DGRAM for UDP
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // This is to lose the pesky "Address already in use" error message
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
                                                  &opt, sizeof(opt))) // SOL_SOCKET is the socket layer itself
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;  // Address family. For IPv6, it's AF_INET6. 29 others exist like AF_UNIX etc. 
    address.sin_addr.s_addr = INADDR_ANY;  // Accept connections from any IP address - listens from all interfaces.
    address.sin_port = htons( PORT );    // Server port to open. Htons converts to Big Endian - Left to Right. RTL is Little Endian

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address,
                                 sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Port bind is done. You want to wait for incoming connections and handle them in some way.
    // The process is two step: first you listen(), then you accept()
    if (listen(server_fd, 3) < 0) // 3 is the maximum size of queue - connections you haven't accepted
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    // returns a brand new socket file descriptor to use for this single accepted connection. Once done, use send and recv
    while(1)
    {
        printf("\n\n*****SERVER WAITING FOR A CLIENT******\n\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t*)&addrlen))<0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        printf("-----New client have requested for services----\n");
        int cc=0;
        while(1)
        {
            printf("\n\n");
            memset(&buffer,'\0',bufflen);
            if(valread=recv(new_socket,buffer,bufflen,0)<0)
            {
                printf("Failed to read the file name sent\n");
                perror("recv error");
                break;
            }
            // printf("%s\n",buffer);
            if(strlen(buffer)==0)
            {
                break;
            }
            if(strcmp(buffer,"NOMORETRINADH")==0)
            {
            
                printf("All the requests of this client are served!!\n");
                break;
            }
            cc++;
            strcpy(buff,buffer);
            if(checkfilestatus()==-1)
            {
                memset(&buffer,'\0',bufflen);
                strcpy(buffer,"Failure");
                if(send(new_socket,buffer,bufflen,0)<0)
                {
                    printf("%s : Failed to send the failure signal\n",buff);
                    continue;
                }
                // printf("sxfgnb\n");
                continue;
            }
            else
            {
                memset(&buffer,'\0',bufflen);
                strcpy(buffer,"Success");
                if(send(new_socket,buffer,bufflen,0)<0)
                {
                    printf("%s : Failed to send the Success signal\n",buff);
                    break;
                }
            }
            sendfilecontent();
        }
        if(cc==0)
        {
            printf("No files are there to be serviced for this client\n");
        }
        printf("\n\n-------Client work is done------");
    }
    // valread = read(new_socket , buffer, 1024);  // read infromation received into the buffer
    // printf("%s\n",buffer);
    // send(new_socket , hello , strlen(hello) , 0 );  // use sendto() and recvfrom() for DGRAM
    // printf("Hello message sent\n");
    return 0;
}
