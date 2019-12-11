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
#include <algorithm>
#include <random>
#include <iostream>

#define _DEBUG

#ifdef _DEBUG
    #define DEBUG(...) (fprintf(stderr, __VA_ARGS__))
#else  
    #define DEBUG(...) (0)
#endif

void help()
{
    DEBUG("OX_chess server\n\n");
    DEBUG(BOLDWHITE"NAME\n"RESET);
    DEBUG("\tOX_server - a OX_chess server for mult-players\n\n");
    DEBUG(BOLDWHITE"SYNOPSIS\n"RESET);
    DEBUG(BOLDWHITE"\t./OX_server"RESET" [options]\n");
    DEBUG(BOLDWHITE"\nOPTIONS\n"RESET);
    DEBUG("\t-a "UNDERLINE"ip address\n"RESET);
    DEBUG("\t\t The ip address you want to bind.\n");
    DEBUG("\t-p "UNDERLINE"port number\n"RESET);
    DEBUG("\t\t The port number you want to bind\n");
}

//member
struct node
{
    char name[10];
    int partner = -1;
    int gameroom = -1;
    char sym;
    int turn = 0;
    int state = 0; //0=quit  1=join 2=paired
};

int nGamerooms=0;

int board[25][3][3];
struct node table[50];
client_t players[50];

int swap(int& a, int &b)
{
    int tmp = a;
    a = b;
    b = tmp;
}

int is_success(int plate[3][3], int player)
{
    // row
    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++){
            if(plate[i][j]!=player) break;
            if(j==2) return player;
        }
    }
    // column
    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++){
            if(plate[j][i]!=player) break;
            if(j==2) return player;
        }
    }
    // cross
    for(int i=0; i<3; i++)
    {
        if(plate[i][i]!=player) break;
        if(i==2) return player;
    }
    for(int i=0; i<3; i++)
    {
        if(plate[i][2-i]!=player) break;
        if(i==2) return player;
    }
    // tie
    for(int i=0; i<3; i++)
    {
        for(int j=0; j<3; j++)
        {
            if(plate[i][j]<0) break;
            if(j==2&&i==2) return 50;
        }
    }

    return -1;
}

void boardcpy(char *buf, int gameroom_id, int player1, int player2)
{
    char buf_tmp[4096];
    strcpy(buf,"\n    1  2  3\n");
    strcat(buf,"--------------\n");
    for(int i=0; i<3; i++){
        sprintf(buf_tmp, "%02d |", i+1);
        strcat(buf, buf_tmp);
        for(int j=0; j<3; j++){
            if(board[gameroom_id][i][j] == player1) sprintf(buf_tmp, "|%c|", table[player1].sym);
            else if(board[gameroom_id][i][j] == player2) sprintf(buf_tmp, "|%c|", table[player2].sym);
            else sprintf(buf_tmp,"| |");

            strcat(buf, buf_tmp);
        }
        strcat(buf, "|\n");
    }
    strcat(buf,"--------------\n");
}

unsigned seed = 12345;
std::default_random_engine generator(seed);
std::normal_distribution<> rnorm(0,1);

void *broadcast(void *indexp)
{
    // char name[30] = "";
    char buf_snd1[4096] = "";
    char buf_snd2[4096] = "";
    char buf_snd3[4096] = "";
    char buf_rcv1[4096] = "";
    int index = *(int*)indexp;

    sock_data ret = players[index].client_socket.recv(20);
    if(ret.length<0) return NULL;
    strncpy(table[index].name, (const char *)ret.raw_data, ret.length);
    table[index].state = 1;

    sprintf(buf_snd1, "ID:%d "BLUE"User %s"RESET" is online\n",index, table[index].name);
    for(int i=0; i<50; i++){
        if(table[i].state==1) players[i].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd1), strlen(buf_snd1));
    }

    while(true)
    {
        sprintf(buf_snd1, "");
        sprintf(buf_snd2, "");
        sprintf(buf_snd3, "");
        sprintf(buf_rcv1, "");
        
        sock_data req = players[index].client_socket.recv(10);
        if(!strcmp("quit", reinterpret_cast<const char *>(req.raw_data)))
        {
            sprintf(buf_snd1, "%s leave this chat room", table[index].name);
            for (int i = 0; i < 50; i++)
            {
                if(i == index) continue;
                if(table[i].state == 1) players[i].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd1), strlen(buf_snd1));
            }
            if(table[index].state==2){
                sprintf(buf_snd2,"Your competitioner is quit.");
                players[index].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
                table[table[index].partner].state = 1;
            }
            table[index].state = 0;
            pthread_exit(0);
        }else if(!strcmp("list", reinterpret_cast<const char *>(req.raw_data)))
        {
            sprintf(buf_snd2, "list:\n");
            for(int i=0; i<50; i++)
            {
                if(table[i].state == 1)
                {
                    sprintf(buf_snd3, "ID:%d %s is waiting.\n", i, table[i].name);
                    strcat(buf_snd2, buf_snd3);
                }else if(table[i].state == 2){
                    sprintf(buf_snd3, "ID:%d %s is paired.\n", i, table[i].name);
                    strcat(buf_snd2, buf_snd3);
                }
            }
            players[index].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
        }else if(!strcmp("pair", reinterpret_cast<const char *>(req.raw_data))){
            sprintf(buf_snd2, "Who do you want to fight with?");
            for(int i=0; i<50; i++)
            {
                if(table[i].state == 1)
                {
                    sprintf(buf_snd3, "ID:%d %s is waiting.\n", i, table[i].name);
                    strcat(buf_snd2, buf_snd3);
                }
            }
            players[index].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
            sock_data res =  players[index].client_socket.recv(30);
            int partner;
            sscanf(reinterpret_cast<const char*>(res.raw_data), "%d", &partner);
            while(index==partner || !(partner>=0 && partner<50) || table[partner].state!=1){
                if(table[partner].state == 0) sprintf(buf_snd2, "ID:%d is offline.\nPlease select the other players: ", partner);
                else if(table[partner].state == 2) sprintf(buf_snd2, "ID:%d is playing.\nPlease select the other players: ", partner);
                else sprintf(buf_snd2, "Invalid choice! Please select the other players: ");
                players[index].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
                res =  players[index].client_socket.recv(30);
                sscanf(reinterpret_cast<const char*>(res.raw_data), "%d", &partner);
            }

            sprintf(buf_snd2, "ID:%d player %s is inviting you (yes/no)", index, table[index].name);
            players[partner].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
            res =  players[partner].client_socket.recv(30);

            while(true){
                if(! strcmp("yes", reinterpret_cast<const char*>(res.raw_data))){
                    table[index].state = table[partner].state = 2;
                    char buf_snd[4096]="";
                    char buf_snd1[4096]="";
                    char buf_snd2[4096]="";
                    char buf_tmp[4096]="";

                    int randnum = rnorm(generator); 
                    int turn,next;
                    if(randnum) {
                        table[index].turn = 1;
                        table[partner].turn = 0;
                        table[index].sym = 'O';
                        table[partner].sym = 'X';
                        turn = index ; next = partner;
                    }else {
                        table[partner].turn = 1;
                        table[index].turn = 0;
                        table[partner].sym = 'O';
                        table[index].sym = 'X';
                        turn = partner; next = index;
                    }
                    
                    strcpy(buf_tmp, "you are "BOLDWHITE"O"RESET" player");
                    players[turn].client_socket.send(reinterpret_cast<const unsigned char *>(buf_tmp), strlen(buf_tmp));
                    strcpy(buf_tmp, "you are "BOLDWHITE"X"RESET" player");
                    players[next].client_socket.send(reinterpret_cast<const unsigned char *>(buf_tmp), strlen(buf_tmp));
                    table[index].gameroom = table[partner].gameroom = nGamerooms++;
                    table[partner].partner = index; table[index].partner = partner;
                    table[index].state = table[partner].state = 2;
                    boardcpy(buf_snd, table[index].gameroom, turn, next);
                    strcpy(buf_snd1, buf_snd);
                    strcpy(buf_snd2, buf_snd);
                    strcat(buf_snd1, "Your turn! Please assign a movement [format: move(<row>,<col>)]: ");
                    players[turn].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd1), strlen(buf_snd1));
                    strcat(buf_snd2, "Not your turn! Please wait for a minute...");
                    players[next].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
                    break;
                }else if(! strcmp("no", reinterpret_cast<const char *>(res.raw_data))){
                    sprintf(buf_snd2, "ID:%d player %s refuse your invition.", partner, table[partner].name);
                    players[index].client_socket.send(reinterpret_cast<const unsigned char *>(buf_snd2), strlen(buf_snd2));
                    break;
                }else{
                    sprintf(buf_snd2, "Invalid answer! ID:%d player %s is inviting you (yes/no): ");
                    players[partner].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
                }
            }
        }else if(strstr(reinterpret_cast<const char*>(req.raw_data),"move")!=NULL){
            bool submmit_success=false;
            if(table[index].state != 2){
                sprintf(buf_snd1, "Your are not in the game.");
            }else if(!table[index].turn){
                sprintf(buf_snd1, "Not your turn!");
            }else{
                char *delim = ",()move";
                char *buf_row, *buf_col;
                bool ok=false;
                int row, col;
                try{
                    char *movement = reinterpret_cast<char *>(req.raw_data);
                    buf_row = strtok(movement, delim);
                    row = atoi(buf_row);
                    buf_col = strtok(NULL, delim);
                    col = atoi(buf_col);
                    ok = true;
                }catch(int e){

                }
                if(!ok){
                    sprintf(buf_snd1, "Format Error! Please assign a movement again[format: move(<row>,<col>) ]:");
                }else if(row<1 || row>3 || col<1 || col>3){
                    sprintf(buf_snd1, "Invalid Error! Please assign a movement again[format: move(<row>,<col>) ]:");
                }else if(board[table[index].gameroom][row-1][col-1]>=0){
                    sprintf(buf_snd1, "The position (%d,%d) has been occupied! Please assign a movement again[format: move(<row>,<col>) ]:", row, col);
                }else{
                    board[table[index].gameroom][row-1][col-1] = index;
                    boardcpy(buf_snd1, table[index].gameroom, index, table[index].partner);
                    submmit_success = true;
                }   
                players[index].client_socket.send(reinterpret_cast<const unsigned char *>(buf_snd1), strlen(buf_snd1));
                if(submmit_success)
                {
                    players[table[index].partner].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd1), strlen(buf_snd1));
                    int winner1 = is_success(board[table[index].gameroom], index);
                    if(winner1 == index){
                        sprintf(buf_snd1, "loss");
                        sprintf(buf_snd2, "win");
                        table[index].state = table[table[index].partner].state = 1;
                    }else if(winner1 == 50){
                        sprintf(buf_snd1, "tie");
                        sprintf(buf_snd2, "tie");
                        table[index].state = table[table[index].partner].state = 1;
                    }else{
                        strcat(buf_snd1, "Your turn! Please assign a movement [format: move(<row>,<col>)]: ");
                        strcat(buf_snd2, "Not your turn! Please wait for a minute...");
                        table[index].turn = 0; table[table[index].partner].turn = 1;
                    }
                    players[index].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd2), strlen(buf_snd2));
                    players[table[index].partner].client_socket.send(reinterpret_cast<const unsigned char*>(buf_snd1), strlen(buf_snd1));
                }
            }
        }
    }
}

int main(int argc, char **argv)
{
    char c;
    char PORT[6];
    int port = 0;
    std::string ip = "127.0.0.1";

    memset(board, 0xFF, sizeof board);

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
				help();
				exit(1);
			default:
				exit(1);
		}
    }

    if(!port){
        help();
        exit(1);
    }

    Socket server_socket = Socket(AF_INET,SOCK_STREAM); 
    DEBUG(GREEN"create successfully\n"RESET);

    server_socket.bind({ip, port});
    server_socket.listen(50);

    int nOnlines = 0, index;
    while(nOnlines<50){
        client_t tmp_sock = server_socket.accept();
        memcpy(players+nOnlines, &(tmp_sock),sizeof(client_t));
        index = nOnlines++;
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, broadcast, &index);
        if (ret<0)
        {
            perror(RED"pthread_create faild"YELLOW);
            return -1;
        }
    }
}
