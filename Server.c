#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define MAX_CLIENT_PER_ROOM 10
#define MAX_CHAT_ROOMS 10
#define MSG_SIZE 140
#define NOT_FOUND -1

static int cli_id = 1000;  //Client ID will increment by one for each new client

typedef struct
{
	char name[256];
	int port_no;
	int num_member;
	int slave_socket[MAX_CLIENT_PER_ROOM];
}chatRoom;

chatRoom chatroom[MAX_CHAT_ROOMS];	
int chatroom_counter=0;

typedef struct {
	struct sockaddr_in addr;
	int cli_fd;			
	int cli_id;
	int commandMode; //0: command  mode 1: chat mode
} client_t;

client_t *cli_list[MAX_CLIENT_PER_ROOM];

int exists(char name[256])
{
	int i;
	for(i=0;i<chatroom_counter;i++)
	{
		if(strcmp(chatroom[i].name,name)==0)
			return i;
	}
	return NOT_FOUND;
}

int getChatRoomIndex(int socket_no)
{
	int i,j;
	for(i=0;i<chatroom_counter;i++)
	{
		for(j=0;j<chatroom[i].num_member;j++)
		{
			if(chatroom[i].slave_socket[j]==socket_no)
				return i;
		}
	}
}

char* toString(int number)
{
	int temp, i=0;
	char* numberToString;
	while(number!=0)
	{
		numberToString[i++]=(number%10)+48;
		number=number/10;		
	}
	return(numberToString);
}

/* Delete client from queue */
void queue_delete(int cli_fd, fd_set *readfds, int *num_clients){
	int i;
	for(i=0;i<MAX_CLIENT_PER_ROOM;i++){
		if(cli_list[i]){
			if(cli_list[i]->cli_fd == cli_fd){
				cli_list[i] = NULL;
				(*num_clients)--;
				close(cli_fd);
                FD_CLR(cli_fd, readfds); //clear the leaving client from the set
				return;
			}
		}
	}
}

/* Send message to all clients but the sender */
void send_message(char *s, int cli_fd){
	int i;
	for(i=0;i<MAX_CLIENT_PER_ROOM;i++){
		if(cli_list[i]){
			//printf("Client FD %d ", cli_list[i]->cli_fd);
			if(cli_list[i]->cli_fd != cli_fd){
				write(cli_list[i]->cli_fd, s, strlen(s));
			}
		}
	}
}

/* Send message to all clients */
void send_message_all(char *s){
	int i;
	for(i=0;i<MAX_CLIENT_PER_ROOM;i++){
		if(cli_list[i]){
			write(cli_list[i]->cli_fd, s, strlen(s));
		}
	}
}

/* Send message to sender */
void send_message_self(const char *s, int cli_fd){
	write(cli_fd, s, strlen(s));
}

void add_client_list(client_t *client){
	int i;
	for(i=0;i<MAX_CLIENT_PER_ROOM;i++){
		if(!cli_list[i]){
			cli_list[i] = client;
			return;
		}
	}
}

int main(int argc, char *argv[])
{
    int server_sockfd, client_sockfd, portno;
    socklen_t clilen;
    
    fd_set readfds, testfds;

    char msg[MSG_SIZE + 1];     
    char kb_msg[MSG_SIZE + 20]; 
	
    struct sockaddr_in serv_addr, cli_addr;
    
    char command[20];
    int num_clients = 0;
    int counter;
    char name[50];
    int result, i, fd;
    
    if(argc < 2){
        fprintf(stderr,"ERROR, no port provided\n");
        exit(1);
    }

    server_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sockfd < 0)
        perror("ERROR opening socket");

    memset((char *) &serv_addr, 0, sizeof(serv_addr));

    portno = atoi(argv[1]);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(portno);

	printf(" Socket address saved\n");
    if (bind(server_sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0)
              perror("ERROR on binding");

    listen(server_sockfd,5);
    
    FD_ZERO(&readfds);
    FD_SET(server_sockfd, &readfds);
    FD_SET(0, &readfds);  /* Add keyboard to file descriptor set */

    printf("Yay...Server is UP\n");
    while(1){
    	testfds = readfds;
        select(FD_SETSIZE, &testfds, NULL, NULL, NULL);

		for (fd = 0; fd < FD_SETSIZE; fd++) {
			if (FD_ISSET(fd, &testfds)) {
				if (fd == server_sockfd) {
					clilen = sizeof(cli_addr);
			    	client_sockfd = accept(server_sockfd, (struct sockaddr *) &cli_addr, &clilen );


					if (client_sockfd < 0)
        				perror("ERROR on accept");
						
					if (num_clients < MAX_CLIENT_PER_ROOM) {
                    	FD_SET(client_sockfd, &readfds);
                    	/*Client ID*/
                    	
                    	client_t *client = (client_t *)malloc(sizeof(client_t));
						client->addr = cli_addr;
						client->cli_fd = client_sockfd;
						client->cli_id = cli_id++;
						client->commandMode = 0;
						num_clients++;
						
						add_client_list(client);
                    	//fflush(stdout);
                    
                    	printf("Client %d Joined \n", client_sockfd);
                 	}
                 	else {
                    	sprintf(msg, "XSorry, too many clients.  Try again later.\n");
                    	write(client_sockfd, msg, strlen(msg));
                    	close(client_sockfd);
                 	}			
				}
				else if (fd == 0)  {  /* Process keyboard activity */                 
                	fgets(kb_msg, MSG_SIZE + 1, stdin);
                 	if (strcmp(kb_msg, "quit\n")==0) {
                    	sprintf(msg, "XServer is shutting down.\n");
                    	send_message_all(msg);
                    	for (i = 0; i < num_clients ; i++) {
                       		close(cli_list[i]->cli_fd);
                    	}
                    close(server_sockfd);
                    exit(0);
                 	}
                 	else {
                    	sprintf(msg, "%s", kb_msg);
                    	send_message_all(msg);
                 	}
              	}
              	else if(fd) {  
              		result = read(fd, msg, MSG_SIZE);
	                if(result==-1) perror("read()");
	                
	                else if(result>0){
                    	/*read 2 bytes client id*/
                    	sprintf(kb_msg,"Client %d: ",fd);
                    	msg[result]='\0';
                    
                    	/*concatinate the client id with the client's message*/
                    	strcat(kb_msg,msg);                                        
                    
                    	/*print to other clients*/
						/*dont write msg to same client*/
                       	send_message(kb_msg, fd);
                        //write(fd_array[i],kb_msg,strlen(kb_msg));
                    	
                    	/*print to server*/
                    	printf("%s",kb_msg);
                    
                    	/*Exit Client*/
                    	if(msg[0] == 'X'){
                       		queue_delete(fd,&readfds, &num_clients);
                    	}   
                 	}             
				}
				else {  /* A client is leaving */
                	queue_delete(fd, &readfds, &num_clients);
              	}
			}
		}
		
	}
    return 0;
}

