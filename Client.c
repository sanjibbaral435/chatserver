#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define MSG_SIZE 200

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, fd, result;
    fd_set clientfds;
    
    char msg[MSG_SIZE + 1];     
   	char kb_msg[MSG_SIZE + 10];
    
    
    struct sockaddr_in serv_addr;
    struct hostent *server;

    if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    portno = atoi(argv[2]);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
         
    serv_addr.sin_port = htons(portno);
    
	if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    fflush(stdout);
     
	while(1){
		FD_ZERO(&clientfds);
     	FD_SET(sockfd,&clientfds);
     	FD_SET(0,&clientfds);//stdin
     
		select(FD_SETSIZE,&clientfds,NULL,NULL,NULL);
		
		for(fd=0;fd<FD_SETSIZE;fd++){
          if(FD_ISSET(fd,&clientfds)){
             if(fd==sockfd){   
			 /*Accept data from open socket */
                
                //read data from open socket
                result = read(sockfd, msg, MSG_SIZE);
                msg[result] = '\0';  /* Terminate string with null */
                printf("%s", msg);
                memset(&msg, 0, sizeof(msg));
                
                if (msg[0] == 'X') {                   
                    close(sockfd);
                    exit(0);
                }                             
             }
             else if(fd == 0){ /*process keyboard activiy*/
                fgets(kb_msg, MSG_SIZE+1, stdin);
                //printf("Input message: %s\n",kb_msg);
                if (strcmp(kb_msg, "quit\n")==0) {
                    sprintf(msg, "I am Quiting. Byee\n");
                    write(sockfd, msg, strlen(msg));
                    close(sockfd); //close the current client
                    exit(0); //end program
                }
                else {
                    sprintf(msg, "%s", kb_msg);
                    write(sockfd, msg, strlen(msg));
                    memset(&msg, 0, sizeof(msg));
                }                                                 
             }
			}
		}
	}
		
    close(sockfd);
    return 0;
}
