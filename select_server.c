#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int recv_work(int socket)
{
    char buf[81];
    int msg_length = 1;

    bzero((char*)&buf, sizeof(buf));
    if((msg_length = recv(socket, buf, sizeof(buf), 0)) < 0)
    {
        perror("Server recv failed");
        exit(1);
    }
    printf("SERVER: socket - %d\n", socket);
    printf("SERVER: message length - %d\n", msg_length);
    printf("SERVER: message - %d\n\n", atoi(buf));  
   
    return msg_length;
}

int main(int argc, char **argv)
{
    fd_set rfds;
    fd_set afds;

    int nfds;
    int child = 0;
    int length = 0;
    int counter = 0;
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

    nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(sock_main, &afds);

    for(;;)
    {
        FD_ZERO(&rfds);
        memcpy(&rfds, &afds, sizeof(rfds));
        if(select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
        {
            perror("SELECT FAIL");
            exit(-1);
        }
        if(FD_ISSET(sock_main, &rfds))
        {
            length = sizeof(server_addr);
            if((sock_client = accept(sock_main, (struct sockaddr *)&server_addr, &length)) < 0)
            {
                perror("Server accept failed");
                exit(1);
            }
            printf("CLIENT: port - %d\n", ntohs(server_addr.sin_port));
            FD_SET(sock_client, &afds);
        }

        for(int fd = 0; fd < nfds; fd++)
        {
            if(fd != sock_main && FD_ISSET(fd, &rfds))
            {
                if(recv_work(fd) == 0)
                {
                    close(fd);
                    FD_CLR(fd, &afds);
                }
            }
        }
    }

    exit(0);
}