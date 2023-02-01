#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int player_count = 0;
pthread_mutex_t mutexcount;

void error(const char *msg)
{
    perror(msg);
    pthread_exit(NULL);
}
void write_client_msg(int cli_sockfd, char * msg)
{
    int n = write(cli_sockfd, msg, strlen(msg));
    if (n < 0)
        error("ERROR writing msg to client socket");
    printf("Message sent (%d bytes).\n", strlen(msg));
}

void write_client_int(int cli_sockfd, int msg)
{
    int n = write(cli_sockfd, &msg, sizeof(int));
    if (n < 0)
        error("ERROR writing int to client socket");
    printf("Message sent <interger> (%d bytes).\n", sizeof(int));
}
void write_client_arrint(int cli_sockfd, int msg[])
{
    int n = write(cli_sockfd, msg, 50*sizeof (int));
    if (n < 0)
        error("ERROR writing int to client socket");
     printf("Message sent <interger array> (%d bytes).\n", sizeof(int)*50);
}


void write_clients_msg(int * cli_sockfd, char * msg)
{
    write_client_msg(cli_sockfd[0], msg);
    write_client_msg(cli_sockfd[1], msg);
}

void write_clients_int(int * cli_sockfd, int msg)
{
    write_client_int(cli_sockfd[0], msg);
    write_client_int(cli_sockfd[1], msg);
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
int recv_int(int cli_sockfd)
{
    int msg = 0;
    int n = read(cli_sockfd, &msg, sizeof(int));

    if (n < 0 || n != sizeof(int))  return -1;

    printf("[DEBUG] Received int: %d\n", msg);

    return msg;
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

        if (num_conn == 0) {
            write_client_msg(cli_sockfd[0],"HLD");

            #ifdef DEBUG
            printf("[DEBUG] Told client 0 to hold.\n");
            #endif
        }

        num_conn++;
    }
}

int get_player_move(int cli_sockfd)
{
    #ifdef DEBUG
    printf("[DEBUG] Getting player move...\n");
    #endif

    write_client_msg(cli_sockfd, "TRN");

    return recv_int(cli_sockfd);
}

int check_move( int move, int check[])
{
    if (check[move]!=1 && move >=0 &&move <=25) {

        #ifdef DEBUG
        printf("[DEBUG] Player %d's move was valid.\n", player_id);
        #endif

        return 1;
   }
   else {
   #ifdef DEBUG
       printf("[DEBUG] Player %d's move was invalid.\n", player_id);
       #endif

       return 0;
   }
}

void update_board(int board1[],int board2[], int move, int check[])
{
    int i;
    for(i=0; i<25; i++)
    {

             if(board1[i]==move) {board1[i]=-1; check[move]=1;}
             if(board2[i]==move) board2[i]=-1;



    }

    #ifdef DEBUG
    printf("[DEBUG] Board updated.\n");
    #endif
}

void draw_board(int board1[],int board2[])
{
    int i=0;
    printf("Player 0: \n");
    for(;i<25;i++)
    {
     if(i!=0&&i%5==0)
     {printf("\n------------------------\n");

      printf("| %-2d |",board1[i]);
      }
     else printf("| %-2d |",board1[i]);


    }
    printf("\nPlayer 1: \n");
    for(i=0;i<25;i++)
    {
      if(i!=0&&i%5==0)
      {
      printf("\n------------------------\n");

       printf("| %-2d |",board2[i]);
       }
     else printf("| %-2d |",board2[i]);

    }
    printf("\n");

}

void send_update(int * cli_sockfd, int broad1[],int broad2 [],int player_id)
{
    #ifdef DEBUG
    printf("[DEBUG] Sending update...\n");
    #endif

    write_clients_msg(cli_sockfd, "UPD");

    write_clients_int(cli_sockfd, player_id);

    write_client_arrint(cli_sockfd[0], broad1);

    write_client_arrint(cli_sockfd[1], broad2);

    #ifdef DEBUG
    printf("[DEBUG] Update sent.\n");
    #endif
}

void send_player_count(int cli_sockfd)
{
    write_client_msg(cli_sockfd, "CNT");
    write_client_int(cli_sockfd, player_count);

    #ifdef DEBUG
    printf("[DEBUG] Player Count Sent.\n");
    #endif
}

int check_board(int board1[],int board2[], int player_turn)
{
    #ifdef DEBUG
    printf("[DEBUG] Checking for a winner...\n");
    #endif
    int p1 =0,p2=0;
    if(player_turn==0)
    {
        int i;

            if(board1[0]==-1&&board1[1]==-1&&board1[2]==-1&&board1[3]==-1&&board1[4]==-1) p1++;
            if(board1[5]==-1&&board1[6]==-1&&board1[7]==-1&&board1[8]==-1&&board1[9]==-1) p1++;
            if(board1[10]==-1&&board1[11]==-1&&board1[12]==-1&&board1[13]==-1&&board1[14]==-1) p1++;
            if(board1[15]==-1&&board1[16]==-1&&board1[17]==-1&&board1[18]==-1&&board1[19]==-1) p1++;
            if(board1[20]==-1&&board1[21]==-1&&board1[22]==-1&&board1[23]==-1&&board1[24]==-1) p1++;
            if(board1[0]==-1&&board1[5]==-1&&board1[15]==-1&&board1[20]==-1&&board1[10]==-1) p1++;
            if(board1[1]==-1&&board1[6]==-1&&board1[11]==-1&&board1[21]==-1&&board1[16]==-1) p1++;
            if(board1[2]==-1&&board1[7]==-1&&board1[12]==-1&&board1[17]==-1&&board1[22]==-1) p1++;
            if(board1[3]==-1&&board1[8]==-1&&board1[13]==-1&&board1[18]==-1&&board1[23]==-1) p1++;
            if(board1[4]==-1&&board1[9]==-1&&board1[14]==-1&&board1[19]==-1&&board1[24]==-1) p1++;
            if(board1[0]==-1&&board1[6]==-1&&board1[12]==-1&&board1[18]==-1&&board1[24]==-1) p1++;
            if(board1[4]==-1&&board1[8]==-1&&board1[12]==-1&&board1[16]==-1&&board1[20]==-1) p1++;

        if(p1>=5) return 1;

             if(board2[0]==-1&&board2[1]==-1&&board2[2]==-1&&board2[3]==-1&&board2[4]==-1) p2++;
            if(board2[5]==-1&&board2[6]==-1&&board2[7]==-1&&board2[8]==-1&&board2[9]==-1) p2++;
            if(board2[10]==-1&&board2[11]==-1&&board2[12]==-1&&board2[13]==-1&&board2[14]==-1) p2++;
            if(board2[15]==-1&&board2[16]==-1&&board2[17]==-1&&board2[18]==-1&&board2[19]==-1) p2++;
            if(board2[20]==-1&&board2[21]==-1&&board2[22]==-1&&board2[23]==-1&&board2[24]==-1) p2++;
            if(board2[0]==-1&&board2[5]==-1&&board2[15]==-1&&board2[20]==-1&&board2[10]==-1) p2++;
            if(board2[1]==-1&&board2[6]==-1&&board2[11]==-1&&board2[21]==-1&&board2[16]==-1) p2++;
            if(board2[2]==-1&&board2[7]==-1&&board2[12]==-1&&board2[17]==-1&&board2[22]==-1) p2++;
            if(board2[3]==-1&&board2[8]==-1&&board2[13]==-1&&board2[18]==-1&&board2[23]==-1) p2++;
            if(board2[4]==-1&&board2[9]==-1&&board2[14]==-1&&board2[19]==-1&&board2[24]==-1) p2++;
            if(board2[0]==-1&&board2[6]==-1&&board2[12]==-1&&board2[18]==-1&&board2[24]==-1) p2++;
            if(board2[4]==-1&&board2[8]==-1&&board2[12]==-1&&board2[16]==-1&&board2[20]==-1) p2++;


        if(p2>=5) return 2;
    }
    else
    {
        int i;

             if(board2[0]==-1&&board2[1]==-1&&board2[2]==-1&&board2[3]==-1&&board2[4]==-1) p2++;
            if(board2[5]==-1&&board2[6]==-1&&board2[7]==-1&&board2[8]==-1&&board2[9]==-1) p2++;
            if(board2[10]==-1&&board2[11]==-1&&board2[12]==-1&&board2[13]==-1&&board2[14]==-1) p2++;
            if(board2[15]==-1&&board2[16]==-1&&board2[17]==-1&&board2[18]==-1&&board2[19]==-1) p2++;
            if(board2[20]==-1&&board2[21]==-1&&board2[22]==-1&&board2[23]==-1&&board2[24]==-1) p2++;
            if(board2[0]==-1&&board2[5]==-1&&board2[15]==-1&&board2[20]==-1&&board2[10]==-1) p2++;
            if(board2[1]==-1&&board2[6]==-1&&board2[11]==-1&&board2[21]==-1&&board2[16]==-1) p2++;
            if(board2[2]==-1&&board2[7]==-1&&board2[12]==-1&&board2[17]==-1&&board2[22]==-1) p2++;
            if(board2[3]==-1&&board2[8]==-1&&board2[13]==-1&&board2[18]==-1&&board2[23]==-1) p2++;
            if(board2[4]==-1&&board2[9]==-1&&board2[14]==-1&&board2[19]==-1&&board2[24]==-1) p2++;
            if(board2[0]==-1&&board2[6]==-1&&board2[12]==-1&&board2[18]==-1&&board2[24]==-1) p2++;
            if(board2[4]==-1&&board2[8]==-1&&board2[12]==-1&&board2[16]==-1&&board2[20]==-1) p2++;


        if(p2>=5) return 2;

           if(board1[0]==-1&&board1[1]==-1&&board1[2]==-1&&board1[3]==-1&&board1[4]==-1) p1++;
            if(board1[5]==-1&&board1[6]==-1&&board1[7]==-1&&board1[8]==-1&&board1[9]==-1) p1++;
            if(board1[10]==-1&&board1[11]==-1&&board1[12]==-1&&board1[13]==-1&&board1[14]==-1) p1++;
            if(board1[15]==-1&&board1[16]==-1&&board1[17]==-1&&board1[18]==-1&&board1[19]==-1) p1++;
            if(board1[20]==-1&&board1[21]==-1&&board1[22]==-1&&board1[23]==-1&&board1[24]==-1) p1++;
            if(board1[0]==-1&&board1[5]==-1&&board1[15]==-1&&board1[20]==-1&&board1[10]==-1) p1++;
            if(board1[1]==-1&&board1[6]==-1&&board1[11]==-1&&board1[21]==-1&&board1[16]==-1) p1++;
            if(board1[2]==-1&&board1[7]==-1&&board1[12]==-1&&board1[17]==-1&&board1[22]==-1) p1++;
            if(board1[3]==-1&&board1[8]==-1&&board1[13]==-1&&board1[18]==-1&&board1[23]==-1) p1++;
            if(board1[4]==-1&&board1[9]==-1&&board1[14]==-1&&board1[19]==-1&&board1[24]==-1) p1++;
            if(board1[0]==-1&&board1[6]==-1&&board1[12]==-1&&board1[18]==-1&&board1[24]==-1) p1++;
            if(board1[4]==-1&&board1[8]==-1&&board1[12]==-1&&board1[16]==-1&&board1[20]==-1) p1++;

        if(p1>=5) return 1;

    }


    #ifdef DEBUG
    printf("[DEBUG] No winner, yet.\n");
    #endif

    return 0;
}
void random_mang(int board[])
{
int mang_dd[50];
     int i =0;
     for(i=0;i<25;i++) mang_dd[i]=0;
     for(i=0;i<25;i++)
     {
        do
        {
          int r= rand()%(24+1-0);
          if(mang_dd[r]!=1) { printf("%d  ",r); board[i]=r; mang_dd[r]=1; break;}
        } while (1);
     }
}

void *run_game(void *thread_data)
{
    int *cli_sockfd = (int*)thread_data;
    int board1[50] ;
    int board2[50] ;
    int check[50];

    printf("Game on!\n");

    write_clients_msg(cli_sockfd, "SRT");

    #ifdef DEBUG
    printf("[DEBUG] Sent start message.\n");
    #endif

     random_mang(board1);
     random_mang(board2);

    draw_board(board1,board2);

    int prev_player_turn = 1;
    int player_turn = 0;
    int game_over = 0;
    int turn_count = 0;
    write_client_arrint(cli_sockfd[0], board1);

    write_client_arrint(cli_sockfd[1], board2);
    while(!game_over) {

        if (prev_player_turn != player_turn)
            write_client_msg(cli_sockfd[(player_turn + 1) % 2], "WAT");

        int valid = 0;
        int move = 0;
        while(!valid) {

          move = get_player_move(cli_sockfd[player_turn]);
            if (move == -1) break;
            printf("Player %d played position %d\n", player_turn, move);

            valid = check_move(move, check);
            if (!valid) {
                printf("Move was invalid. Let's try this again...\n");
                write_client_msg(cli_sockfd[player_turn], "INV");
            }
        }

	    if (move == -1) {
            printf("Player disconnected.\n");
            break;
        }
        else if (move == 25) {
            prev_player_turn = player_turn;
            send_player_count(cli_sockfd[player_turn]);
        }
        else {
                     update_board(board1,board2, move, check);
            send_update( cli_sockfd,board1,board2, player_turn );


            draw_board(board1,board2);

                        game_over = check_board(board1,board2,player_turn);

            if (game_over == 1 || game_over == 2) {
            if(game_over==1)
            {
                write_client_msg(cli_sockfd[0], "WIN");
                write_client_msg(cli_sockfd[1], "LSE");
                printf("Player %d won.\n", player_turn);


            }
            else
            {
               write_client_msg(cli_sockfd[0], "LSE");
                write_client_msg(cli_sockfd[1], "WIN");
                printf("Player %d won.\n", player_turn);
            }
            }

            else if (turn_count == 25) {
               printf("Draw.\n");
                write_clients_msg(cli_sockfd, "DRW");
                game_over = 1;
            }

            prev_player_turn = player_turn;
            player_turn = (player_turn + 1) % 2;
            turn_count++;
        }
    }

    printf("Game over.\n");

	close(cli_sockfd[0]);
    close(cli_sockfd[1]);

    pthread_mutex_lock(&mutexcount);
    player_count--;
    printf("Number of players is now %d.", player_count);
    player_count--;
    printf("Number of players is now %d.", player_count);
    pthread_mutex_unlock(&mutexcount);

    free(cli_sockfd);

    pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    int lis_sockfd = setup_listener(atoi(argv[1]));
    printf("Server listening on port %d\n", atoi(argv[1]));
    pthread_mutex_init(&mutexcount, NULL);

    while (1) {
        if (player_count <= 252) {
            int *cli_sockfd = (int*)malloc(2*sizeof(int));
            memset(cli_sockfd, 0, 2*sizeof(int));

            get_clients(lis_sockfd, cli_sockfd);

            #ifdef DEBUG
            printf("[DEBUG] Starting new game thread...\n");
            #endif

            pthread_t thread;
            int result = pthread_create(&thread, NULL, run_game, (void *)cli_sockfd);
            if (result){
                printf("Thread creation failed with return code %d\n", result);
                exit(-1);
            }

            printf("[DEBUG] New game thread started.\n");
            }
    }

    close(lis_sockfd);

    pthread_mutex_destroy(&mutexcount);
pthread_exit(NULL);
}