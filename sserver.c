/* server.c */

#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


int player_count = 0;
pthread_mutex_t mutexcount;

#pragma pack(1)

typedef struct _intial_game_value {
    int CANVAS_WIDTH;
    int CANVAS_HEIGHT;
    int UNIT_SIZE;
    int SPEED;
    int SNAKE_ID;
} intial_game_value;

typedef struct _food_pos {
    int x;
    int y;
} food_pos;

typedef struct _control {
    int x;
} control;

typedef struct _send_data {
    uint8_t snake_id;
    uint8_t data_type; //0,1,2,3 for control signal,
    // 4 for food collision
    // and 5 for wall collision
} send_data;

typedef struct _send_datas {
    uint8_t data_0, data_1;
} send_datas;

#pragma pack()

typedef struct _game_state {
    uint16_t food_x, food_y;
    //snake 1
    uint32_t counter0, counter1;
    uint16_t score0, score1;
    uint8_t is_alive0, is_alive1;
    uint8_t is_col;
    //snake 2
} game_state;

void write_client_msg(int cli_sockfd, char * msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
    printf("Message sent (%d bytes).\n", strlen(msg));
}

void write_clients_msg(int * cli_sockfd, char * msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}

int createSocket(int port)
{
    int sock, err;
    struct sockaddr_in server;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("ERROR: Socket creation failed\n");
        exit(1);
    }
    printf("Socket created\n");

    bzero((char *) &server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if (bind(sock, (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        printf("ERROR: Bind failed\n");
        exit(1);
    }
    printf("Bind done\n");

    listen(sock , 3);

    return sock;
}

void closeSocket(int sock)
{
    close(sock);
    return;
}

int setup_listener(int portno)
{
    int sockfd;
    struct sockaddr_in serv_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening listener socket.");

    memset(&serv_addr, 0, sizeof(serv_addr));
     serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
        error("ERROR binding listener socket.");

    #ifdef DEBUG
    printf("[DEBUG] Listener set.\n");
    #endif

    return sockfd;
}

void get_clients(int lis_sockfd, int * cli_sockfd)
{
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    #ifdef DEBUG
    printf("[DEBUG] Listening for clients...\n");
    #endif

    int num_conn = 0;
    while(num_conn < 2)
    {
	    listen(lis_sockfd, 253 - player_count);

        memset(&cli_addr, 0, sizeof(cli_addr));

        clilen = sizeof(cli_addr);

        cli_sockfd[num_conn] = accept(lis_sockfd, (struct sockaddr *) &cli_addr, &clilen);

        if (cli_sockfd[num_conn] < 0)
            error("ERROR accepting a connection from a client.");

        #ifdef DEBUG
        printf("[DEBUG] Accepted connection from client %d\n", num_conn);
        #endif

        write(cli_sockfd[num_conn], &num_conn, sizeof(int));

        #ifdef DEBUG
        printf("[DEBUG] Sent client %d it's ID.\n", num_conn);
        #endif

        pthread_mutex_lock(&mutexcount);
        player_count++;
        printf("Number of players is now %d.\n", player_count);
        pthread_mutex_unlock(&mutexcount);

        // if (num_conn == 0) {
        //     write_client_msg(cli_sockfd[0],"HLD");

        //     #ifdef DEBUG
        //     printf("[DEBUG] Told client 0 to hold.\n");
        //     #endif
        // }

        num_conn++;
    }
}

void sendMsg(int sock, void* msg, uint32_t msgsize)
{
    if (write(sock, msg, msgsize) < 0)
    {
        printf("Can't send message.\n");
        closeSocket(sock);
        exit(1);
    }
    printf("Message sent (%d bytes).\n", msgsize);
    return;
}

int GetRandom(int max_value, int min_value, int step)
{
    int random_value = (rand() % ((++max_value - min_value) / step)) * step + min_value;
    return random_value;
}

void update_state(game_state* state, send_data* data){
        if(data->data_type == 4){
            state->food_x = GetRandom(600 - 10, 2*10, 10);
            state->food_y = GetRandom(600 - 10, 2*10, 10);
            if(data->snake_id == 0){
                state->score0 += 10;
                state->counter0 += 1;
                state->is_col = 1;
            }
            else{
                state->score1 += 10;
                state->counter1 += 1;
                state->is_col = 2;
            }
        }
        else if(data->data_type == 5){
            if(data->snake_id == 0){
                state->is_alive0 = 0;    
            }
            else state->is_alive1 = 0;
        }
}

void* start_game(void *thread_data){
    int *csock = (int*)thread_data;
    int nread, nread2;
    int BUFFSIZE = 512;
    char buff[BUFFSIZE], buff2[BUFFSIZE];
    while (1)
    {
        //csock = accept(ssock, (struct sockaddr *)&client, &clilen);
        game_state* state = (game_state*)malloc(sizeof(game_state));
        state->is_alive0 = 1; state->is_alive1 = 1;
        // if (csock < 0)
        // {
        //     printf("Error: accept() failed\n");
        //     continue;
        // }

        // printf("Accepted connection from %s\n", inet_ntoa(client.sin_addr));
        // bzero(buff, BUFFSIZE);

        write_clients_msg(csock, "STR");
        
        //nread=read(csock, buff, BUFFSIZE);
        //printf("\nReceived %d bytes\n", nread);
        intial_game_value *p = (intial_game_value*)malloc(sizeof(intial_game_value));
        p->CANVAS_WIDTH = 600;
        p->CANVAS_HEIGHT = 600;
        p->UNIT_SIZE = 10;
        p->SPEED = 15;
        p->SNAKE_ID = 0;
            //printf("Received contents: id=%d, counter=%d, temp=%f\n",
            //        p->id, p->counter, p->temp);
        printf("Sending it back.. ");
        sendMsg(csock[0], p, sizeof(intial_game_value));
        p->SNAKE_ID = 1;
        sendMsg(csock[1], p, sizeof(intial_game_value));
        free(p);
        bzero(buff, BUFFSIZE);

        nread=read(csock, buff, BUFFSIZE);
        food_pos *f = (food_pos*)malloc(sizeof(food_pos));
        f->x = GetRandom(600 - 10, 2*10, 10);
        f->y = GetRandom(600 - 10, 2*10, 10);
        printf("Sending it back.. ");
        sendMsg(csock[0], f, sizeof(food_pos));
        sendMsg(csock[1], f, sizeof(food_pos));
        free(f);
        bzero(buff, BUFFSIZE);

        while ((nread=read(csock[0], buff, BUFFSIZE)) > 0 && (nread2=read(csock[1], buff2, BUFFSIZE)) > 0)
        {
        //read first byte of the message to determine what kind of package
        // 1. control signal
        // 2. colision detection
        send_datas *data = (send_datas*)malloc(sizeof(send_datas));
        
        data->data_0 = ((send_data*)buff)->data_type;
        data->data_1 = ((send_data*)buff2)->data_type;
        printf("%d-%d",data->data_0, data->data_1);
        if((data->data_0 >= 0 && data->data_0 <= 3) || data->data_0 == 6 ){
            printf("Sending it back.. ");
            sendMsg(csock[0], data, sizeof(send_datas));
            sendMsg(csock[1], data, sizeof(send_datas));
            free(data);
            bzero(buff, BUFFSIZE);
            bzero(buff2, BUFFSIZE);
        }
        else{
            update_state(state, (send_data*)buff);
            update_state(state, (send_data*)buff2);
            printf("Sending it back.. ");
            sendMsg(csock[0], state, sizeof(game_state));
            sendMsg(csock[1], state, sizeof(game_state));
            state->is_col = 0;
            bzero(buff, BUFFSIZE);
            bzero(buff2, BUFFSIZE);
        }
        }
        printf("Closing connection to client\n");
        printf("----------------------------\n");
        free(state);
        closeSocket(csock[0]);
        closeSocket(csock[1]);
        pthread_mutex_lock(&mutexcount);
        player_count--;
        printf("Number of players is now %d.", player_count);
        player_count--;
        printf("Number of players is now %d.", player_count);
        pthread_mutex_unlock(&mutexcount);

        free(csock);

        pthread_exit(NULL);

    }
}


int main()
{
    int PORT = 2300;
    int BUFFSIZE = 512;
    char buff[BUFFSIZE];
    int ssock, csock;
    int nread;
    struct sockaddr_in client;
    int clilen = sizeof(client);

    //ssock = createSocket(PORT);
    ssock = setup_listener(PORT);
    printf("Server listening on port %d\n", PORT);
   
    // while (1)
    // {
    //     csock = accept(ssock, (struct sockaddr *)&client, &clilen);
    //     game_state* state = (game_state*)malloc(sizeof(game_state));
    //     if (csock < 0)
    //     {
    //         printf("Error: accept() failed\n");
    //         continue;
    //     }

    //     printf("Accepted connection from %s\n", inet_ntoa(client.sin_addr));
    //     bzero(buff, BUFFSIZE);

    //     //intial game state
    //     nread=read(csock, buff, BUFFSIZE);
    //     printf("\nReceived %d bytes\n", nread);
    //     intial_game_value *p = (intial_game_value*)malloc(sizeof(intial_game_value));
    //     p->CANVAS_WIDTH = 600;
    //     p->CANVAS_HEIGHT = 600;
    //     p->UNIT_SIZE = 10;
    //     p->SPEED = 15;
    //         //printf("Received contents: id=%d, counter=%d, temp=%f\n",
    //         //        p->id, p->counter, p->temp);
    //     printf("Sending it back.. ");
    //     sendMsg(csock, p, sizeof(intial_game_value));
    //     free(p);
    //     bzero(buff, BUFFSIZE);

    //     nread=read(csock, buff, BUFFSIZE);
    //     food_pos *f = (food_pos*)malloc(sizeof(food_pos));
    //     f->x = GetRandom(600 - 10, 2*10, 10);
    //     f->y = GetRandom(600 - 10, 2*10, 10);
    //     printf("Sending it back.. ");
    //     sendMsg(csock, f, sizeof(food_pos));
    //     free(f);
    //     bzero(buff, BUFFSIZE);

      
    //     // nread=read(csock, buff, BUFFSIZE);
    //     // printf("Sending it back.. ");
    //     // control* c = (control*) buff;
    //     // sendMsg(csock, c, sizeof(control));
    //     // bzero(buff, BUFFSIZE);

    //     while ((nread=read(csock, buff, BUFFSIZE)) > 0)
    //     {
    //     //read first byte of the message to determine what kind of package
    //     // 1. control signal
    //     // 2. colision detection
    //     send_data *data = (send_data*)buff;
    //     if(data->data_type >= 0 && data->data_type <= 3 ){
    //         printf("Sending it back.. ");
    //         sendMsg(csock, data, sizeof(send_data));
    //         bzero(buff, BUFFSIZE);
    //     }
    //     else{
    //         update_state(state, data);
    //          printf("Sending it back.. ");
    //         sendMsg(csock, state, sizeof(game_state));
    //         bzero(buff, BUFFSIZE);
    //     }
    //     }
    //     printf("Closing connection to client\n");
    //     printf("----------------------------\n");
    //     closeSocket(csock);
    // }


    pthread_mutex_init(&mutexcount, NULL);

    while (1) {
        if (player_count <= 252) {
            int *cli_sockfd = (int*)malloc(2*sizeof(int));
            memset(cli_sockfd, 0, 2*sizeof(int));

            get_clients(ssock, cli_sockfd);
            //strcpy(buff, "STR");
            //#sendMsg(cli_sockfd[0], buff, sizeof(int));
            //sendMsg(cli_sockfd[1], buff, sizeof(int));
            #ifdef DEBUG
            printf("[DEBUG] Starting new game thread...\n");
            #endif

            pthread_t thread;
            int result = pthread_create(&thread, NULL, start_game, (void *)cli_sockfd);
            if (result){
                printf("Thread creation failed with return code %d\n", result);
                exit(-1);
            }

            printf("[DEBUG] New game thread started.\n");
            }
    }

    closeSocket(ssock);
    printf("bye");
    pthread_mutex_destroy(&mutexcount);
    pthread_exit(NULL);
    return 0;
}