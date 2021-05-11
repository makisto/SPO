#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

struct socket_data
{
    int id;
    int socket;
    pthread_t thread;
};

void *recv_work(struct socket_data *data)
{
    char buf[81];
    int msg_length = 1;

    int id = data->id;
    int socket = data->socket;

    printf("SERVER: THREAD ID - %d\n", id);
    printf("SERVER: socket - %d\n", socket);

    while(msg_length != 0)
    {
        if((msg_length = recv(socket, buf, 81, 0)) < 0)
        {
            perror("Server recv failed");
            pthread_exit(NULL); 
        } 
        printf("SERVER: THREAD ID - %d\n", id);
        printf("SERVER: message length - %d\n", msg_length);
        printf("SERVER: message - %d\n\n", atoi(buf)); 
    }

    printf("SERVER: THREAD ID - %d ENDS WORK\n", id);
    close(socket);
    pthread_exit(NULL);
}

int main(int argc, char **argv)
{
    int child = 0;
    int length = 0;
    int sock_main = 0;
    struct socket_data sock_client[5];

    for(int i = 0; i < 5; i++)
    {
        sock_client[i].id = i;
        sock_client[i].socket = 0;
        sock_client[i].thread = 0;
    }

    pthread_attr_t ta;
    struct sockaddr_in server_addr;

    if ((sock_main = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Server socket failed");
        exit(1);
    }

    bzero((char*)&server_addr, sizeof(server_addr));

    server_addr.sin_port = 0;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

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

    int counter = 0;
    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    for(;;)
    {
        if((sock_client[counter].socket = accept(sock_main, (struct sockaddr *)&server_addr, &length)) < 0)
        {
            perror("Server accept failed");
            exit(1);
        }
        printf("CLIENT: port - %d\n", ntohs(server_addr.sin_port));
        if (pthread_create(&sock_client[counter].thread, &ta, (void*(*)(void*))recv_work, &sock_client[counter]) < 0)
        {
            perror("Server thread failed");
            exit(1);
        }
        pthread_detach(sock_client[counter].thread);
        counter++;
        if(counter == 5)
        {
            counter = 0;
        }
    }
    
    close(sock_main);
    return 0;
}