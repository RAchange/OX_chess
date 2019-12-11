#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <tuple>
#include <utility>
#include <unistd.h>
#include <cstring>

typedef struct sockaddr *sockaddrp;

#define MAX_BUF 4096

// static char * buf[MAX_BUF];

typedef struct address {
    std::string adr;
    int port;
} address;

typedef struct sock_data{
    unsigned char *raw_data;
    int length=0;
} sock_data;

typedef struct cli client_t;

class Socket
{
    int fd=-1, portNum, clientLen, nBytes;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    std::string hostname;
    int newSocket;
public:
    Socket();
    Socket(int);
    Socket(int, int);
    ~Socket();

    void bind(std::tuple<std::string, int>);
    void listen(int);
    client_t accept();
    void connect(std::tuple<std::string, int>);
    sock_data recv(int);
    size_t send(const unsigned char *, int size);
    void shutdown(int);
    void close();

    std::string gethostname();
};

std::ostream& operator<<(std::ostream&, const address&);

struct cli{
    Socket client_socket;
    address client_address;
};