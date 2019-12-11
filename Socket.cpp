#include "Socket.h"
#include "Color.h"
#define _RELEASE
#ifdef _DEBUG
#define DEBUG(...) (fprintf(stderr,__VA_ARGS__))
#else
#define DEBUG(...) (0)
#endif

unsigned long hexdump(const unsigned char * data, unsigned long size, int step=16, int sep=4, bool decimal=false)
{
    unsigned long nResults=0;
    for(unsigned long i=0; i<size; i+=step)
    {
        if(decimal) {
            nResults+=DEBUG("%08d |", i);
        }else{
            nResults+=DEBUG("%08X |", i);
        }
        for(unsigned long j=0; j<step; j++)
        {
            if(i+j<size){
                nResults+=DEBUG(" %02X", data[i+j]);
            }else{
                nResults+=DEBUG("   ");
            }
            if(!((i+j+1)%sep)) nResults+=DEBUG(" ");
        }
        nResults+=DEBUG(" | ");
        for(unsigned long j=0; j<step; j++){
            unsigned char c = data[i+j];
            unsigned char u = (0x20<=c && c<=0x7F) ? data[i+j]:'.';
            nResults+=DEBUG("%c", u);
        }
        nResults+=DEBUG("\n");
    }
}

Socket::Socket(){

}

Socket::Socket(int family, int type)
{
    serverAddr.sin_family = family;
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero); 
    fd = ::socket(family, type, 0);
}

Socket::Socket(int socket_fd):fd(socket_fd){
    addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(fd, (struct sockaddr *)&serverAddr, &addr_size);
    portNum = serverAddr.sin_port;
    hostname = std::string(inet_ntoa(serverAddr.sin_addr));
}

Socket::~Socket(){
    serverAddr.sin_family = AF_INET;
    memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero); 
    fd = ::socket(serverAddr.sin_family, SOCK_STREAM, 0);
}

void Socket::bind(std::tuple<std::string, int> obj)
{
    hostname = std::get<0>(obj);
    portNum = std::get<1>(obj);
    serverAddr.sin_port = htons(portNum);
    serverAddr.sin_addr.s_addr = inet_addr(hostname.c_str());
    if(::bind(fd, (struct sockaddr *) &serverAddr, sizeof(serverAddr))==-1){
        perror(RED"bind () "RESET);
        exit(1);
    }
}

void Socket::listen(int nQueue)
{
    if(::listen(fd,nQueue)==0)
        printf(BLUE"Listening on"GREEN" %d\n"RESET, portNum);
    else
        printf(RED"Error\n"RESET);
    addr_size = sizeof serverStorage;
}

client_t Socket::accept()
{
    newSocket = ::accept(fd, (struct sockaddr *) &serverStorage, &addr_size);
    Socket clientSocket = Socket(newSocket);
    clientSocket.serverAddr.sin_family = serverStorage.ss_family;
    struct sockaddr_in addr;
    socklen_t addr_size = sizeof(struct sockaddr_in);
    int res = getpeername(newSocket, (struct sockaddr *)&addr, &addr_size);
    std::string *clientip = new std::string(inet_ntoa(addr.sin_addr));
    int port = ntohs(addr.sin_port);
    client_t re{clientSocket, {*clientip, port}};

    return re;
}

void Socket::connect(std::tuple<std::string, int> adr)
{
    struct sockaddr_in info;
    char hostname[100];
    bzero(&info,sizeof(info));
    info.sin_family = PF_INET;
    strcpy(hostname, std::get<0>(adr).c_str());
    info.sin_addr.s_addr = inet_addr(hostname);
    info.sin_port = htons(std::get<1>(adr));
    if(::connect(fd,(struct sockaddr *)&info,sizeof(info))==-1)
        printf(RED"Connection error"RESET);
    else
        printf(BLUE"Connect to "GREEN"%s:%d\n"RESET, hostname, std::get<1>(adr));
       
}

sock_data Socket::recv(int nbytes)
{
    unsigned char *buf = new unsigned char[nbytes+1];
    size_t real_siz = ::recv(fd, buf, nbytes, 0);
    
    #ifdef _DEBUG
    DEBUG("Recv:\n");
    hexdump(buf, real_siz);
    #endif
    #ifdef _RELEASE
    printf("%s", reinterpret_cast<const char*>(buf));
    #endif

    sock_data re = {buf, real_siz};
    return re; 
}

size_t Socket::send(const unsigned char * data, int size)
{
    size_t real_siz = ::send(fd, data, size, 0);
    DEBUG("Send:\n");
    #ifdef _DEBUG
    hexdump(data, real_siz);
    #endif
    #ifdef _RELEASE
    printf("%s", reinterpret_cast<const char*>(data));
    #endif
    return real_siz;
}

void Socket::shutdown(int flag)
{
    ::shutdown(fd, flag);
}

void Socket::close()
{
    ::close(fd);
}

std::string Socket::gethostname()
{
    char *name = new char[150];
    if(::gethostname(name, 150)){
        perror(YELLOW"gethostname() "RESET);
    }
    return std::string(name);
}

std::ostream& operator<<(std::ostream& os, const address& addr)
{
    std::string ret= "(";
    ret+=addr.adr;
    ret+=(":");
    ret+=std::string(std::to_string(addr.port));
    ret+=")";
    os << ret;
    return os;
}
