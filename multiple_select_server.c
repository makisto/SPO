#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if_arp.h>

pthread_mutex_t st_mutex; 

int max2(int x, int y)
{
    if (x > y)
    {
        return x;
    }
    return y;
}

//, struct sockaddr_in *server_addr

void *tcp_recv_work(int *socket)
{
    char buf[81];
    int msg_length = 1;

    while(msg_length != 0)
    {
        bzero((char*)&buf, sizeof(buf));
        if((msg_length = recv(*socket, buf, 81, 0)) < 0)
        {
            perror("Server TCP recv failed");
            exit(1);
        }
        printf("TCP-CONNECT\n");
        /*printf("CLIENT: IP - %s\n", inet_ntoa(server_addr->sin_addr)); 
        printf("CLIENT: port - %d\n", ntohs(server_addr->sin_port)); */
        printf("SERVER: socket - %d\n", *socket);
        printf("SERVER: message length - %d\n", msg_length);
        printf("SERVER: message - %d\n\n", atoi(buf)); 
    }

    //return msg_length;
}

int udp_recv_work(int socket, struct sockaddr_in *client_addr)
{
    char buf[81];
    int msg_length = 1;
    int length = sizeof(*client_addr);

    bzero((char*)&buf, sizeof(buf));
    if((msg_length = recvfrom(socket, buf, 81, 0, (struct sockaddr *)*&client_addr, &length)) < 0)
    {
        perror("Server UDP recv failed");
        exit(1);
    }
    printf("UDP-CONNECT\n");
    printf("CLIENT: IP - %s\n", inet_ntoa(client_addr->sin_addr)); 
    printf("CLIENT: port - %d\n", ntohs(client_addr->sin_port)); 
    printf("SERVER: socket - %d\n", socket);
    printf("SERVER: message length - %d\n", msg_length);
    printf("SERVER: message - %d\n\n", atoi(buf));   

    return msg_length;
}

int set_tcp_main(struct sockaddr_in *server_addr)
{
    int tcp_sock = 0;

    if ((tcp_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("Server socket failed");
        exit(1);
    }

    if(bind(tcp_sock, (struct sockaddr *)*&server_addr, sizeof(*server_addr)))
    {
        perror("Server bind failed");
        exit(1);
    }

    return tcp_sock;
}

int set_udp_main()
{
    int udp_sock = 0;

    if ((udp_sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Server socket failed");
        exit(1);
    }

    /*if(bind(udp_sock, (struct sockaddr *)*&server_addr, sizeof(*server_addr)))
    {
        perror("Server bind failed");
        exit(1);
    }*/

    return udp_sock;
}

int main(int argc, char **argv)
{
    fd_set afds;
    fd_set rfds;

    int nfds;
    int length = 0;
    int counter = 0;
    int udp_sock_main = 0;
    int tcp_sock_main = 0;

    pthread_t th;
    pthread_attr_t ta;

    pthread_attr_init(&ta);
    pthread_attr_setdetachstate(&ta, PTHREAD_CREATE_DETACHED);

    struct sockaddr_in udp_client_addr;

    struct sockaddr_in tcp_server_addr;
    struct sockaddr_in udp_server_addr;

    bzero((char*)&tcp_server_addr, sizeof(tcp_server_addr));
    bzero((char*)&udp_server_addr, sizeof(udp_server_addr));

    tcp_server_addr.sin_port = 0;
    tcp_server_addr.sin_family = AF_INET;

    tcp_sock_main = set_tcp_main(&tcp_server_addr);
    udp_sock_main = set_udp_main();

    length = sizeof(tcp_server_addr);
    if(getsockname(tcp_sock_main, (struct sockaddr *)&tcp_server_addr, &length))
    {
        perror("Server getsockname failed");
        exit(1);
    }
    printf("TCP-SERVER: port - %d\n", ntohs(tcp_server_addr.sin_port));
    listen(tcp_sock_main, 5);

    udp_server_addr.sin_family = AF_INET;
    udp_server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    udp_server_addr.sin_port = tcp_server_addr.sin_port;

    if(bind(udp_sock_main, (struct sockaddr *)&udp_server_addr, sizeof(udp_server_addr)))
    {
        perror("Server bind failed");
        exit(1);
    }

    length = sizeof(udp_server_addr);
    if(getsockname(udp_sock_main, (struct sockaddr *)&udp_server_addr, &length))
    {
        perror("Server getsockname failed");
        exit(1);
    }
    printf("UDP-SERVER: port - %d\n", ntohs(udp_server_addr.sin_port));
    
    nfds = max2(tcp_sock_main, udp_sock_main) + 1;
    FD_ZERO(&rfds);

    int *tcp_sock_client = (int*)malloc(nfds * sizeof(int));

    for(;;)
    {
        FD_SET(tcp_sock_main, &rfds);
        FD_SET(udp_sock_main, &rfds);
        if(select(nfds, &rfds, (fd_set *)0, (fd_set *)0, (struct timeval *)0) < 0)
        {
            perror("SELECT FAIL");
            exit(-1);
        }
        if(FD_ISSET(tcp_sock_main, &rfds))
        {
            length = sizeof(tcp_server_addr);
            if((tcp_sock_client[counter] = accept(tcp_sock_main, (struct sockaddr *)&tcp_server_addr, &length)) < 0)
            {
                perror("Server accept failed");
                exit(1);
            }
            if(pthread_create(&th, &ta, (void*(*)(void*))tcp_recv_work, &tcp_sock_client[counter]) < 0)
            {
                perror("Thread failed");
                exit(-1); 
            }
            counter++;
            if(counter >= nfds)
            {
                counter = 0;
            }
            /*switch(fork())
            {
                case 0:
                   tcp_recv_work(tcp_sock_client, &tcp_server_addr);
                   break;
                case -1:
                    perror("Fork failed");
                    exit(-1); 
            }*/
        }

        if(FD_ISSET(udp_sock_main, &rfds))
        {
            udp_recv_work(udp_sock_main, &udp_client_addr);  
        }
    }

    exit(0);
}