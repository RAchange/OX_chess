#include "Socket.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>
#include <unistd.h>
#include <cstdlib>
#include "Color.h"
#include <termcap.h>
#include <utility>
#include <iostream>

#define _DEBUG

#ifdef _DEBUG
    #define DEBUG(...) (fprintf(stderr, __VA_ARGS__))
#else  
    #define DEBUG(...) (0)
#endif

Socket client_socket;

void *recv_other(void *arg)
{
    char buf[4096] = "";
    while(true){
        sock_data ret = client_socket.recv(100);
        if(ret.length<0)
        {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    char c;
    char PORT[6], buf[4096];
    std::string ip="127.0.0.1";
    int port=0;
    while ((c = getopt (argc, argv, "p:a:h")) != -1){
		switch (c)
		{
			case 'a':
                ip = std::string(optarg);
				break;
			case 'p':
				strcpy(PORT,optarg);
                port = atoi(PORT);
				break;
			case 'h':
            	exit(1);
			default:
				exit(1);
		}
    }

    if(!port){
        exit(1);
    }

    client_socket = Socket(AF_INET, SOCK_STREAM);
    client_socket.connect({ip, port});
    
    std::string username;
    printf("Please input your name:");
    std::cin >> username;
    client_socket.send(reinterpret_cast<const unsigned char*>(username.c_str()), username.length());
    
    //create child process
    pthread_t tid;
    int ret = pthread_create(&tid, NULL, recv_other, NULL);
    
    if (0 > ret)
    {
        perror("pthread_create");
        return -1;
    }

    while(1)
    {
        printf("> ");
        scanf("%s", buf);
        client_socket.send(reinterpret_cast<const unsigned char *>(buf), strlen(buf));

        //quit
        if (strcmp("quit", buf)==0)
        {
            std::cout<<username;
            printf(",quit\n");
            return 0;
        }
    }
}