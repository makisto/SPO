#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

void reaper(int sig)
{
    int status;
    while(wait3(&status, WNOHANG, (struct rusage *)0) >= 0);
}

int recv_work(int socket)
{
    char buf[81];
    int msg_length;
    while(msg_length != 0)
    {
        if((msg_length = recv(socket, buf, 81, 0)) < 0)
        {
            perror("Server recv failed");
            exit(1);
        }
        printf("SERVER: pid - %d\n", getpid());
        printf("SERVER: socket - %d\n", socket);
        printf("SERVER: message length - %d\n", msg_length);
        printf("SERVER: message - %d\n\n", atoi(buf));  
    }     
    printf("SERVER %d END WORKS\n", getpid());
}

int main(int argc, char **argv)
{
    signal(SIGCHLD, reaper);

    int child = 0;
    int length = 0;
    int sock_main = 0;
    int sock_client = 0;

    struct sockaddr_in server_addr;

    if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Server socket failed");
        exit(1);
    }

    bzero((char*)&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = 0;

    if(bind(sock_main, (struct sockaddr *)&server_addr, sizeof(server_addr)))
    {
        perror("Server bind failed");
        exit(1);
    }

    length = sizeof(server_addr);
    if(getsockname(sock_main, (struct sockaddr *)&server_addr, &length))
    {
        perror("Server getsockname failed");
        exit(1);
    }
    printf("SERVER: port - %d\n", ntohs(server_addr.sin_port));
    listen(sock_main, 5);

    for(;;)
    {
        if((sock_client = accept(sock_main, (struct sockaddr *)&server_addr, &length)) < 0)
        {
            perror("Server accept failed");
            exit(1);
        }
        printf("CLIENT: port - %d\n", ntohs(server_addr.sin_port));
        switch(fork())
        {
            case(0):
                close(sock_main);
                recv_work(sock_client);
                close(sock_client);
                exit(0);
            case(-1):
                perror("FORK MISTAKE");
                break;
            default:
                close(sock_client);
                break;
        }
    }
}