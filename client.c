#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char **argv)
{
    int sock;
    int data;
    char buffer[256];
    struct sockaddr_in server_addr;
    struct hostent *hp, *gethostbyname();

    if (argc < 4)
    {
        perror("Client message failed: too little params");
        exit(1);
    }

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Client socket failed");
        exit(1);
    }

    bzero((char *)&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    hp = gethostbyname(argv[1]);
    bcopy(hp->h_addr, &server_addr.sin_addr, hp->h_length);

    server_addr.sin_port = htons(atoi(argv[2]));
    
    if(connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Client connect failed");
        exit(1);
    }
    printf("CLIENT: ready\n");

    data = atoi(argv[3]);

    for(int i = 0; i <= 10; i++)
    {
        sleep(data);
        sprintf(buffer, "%d", data);
        if(send(sock, buffer, strlen(buffer), 0) < 0)
        {
            perror("Client send failed");
            exit(1);
        }         
    }  

    printf("EASY\n");
    close(sock);
    exit(0);
}