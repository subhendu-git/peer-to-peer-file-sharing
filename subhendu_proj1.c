/****************************************************
 * Author: Subhendu Saha
 * File : subhendu_proj1.c
 * 
 * ***************************************************/
 
/******************************************************
 * List of resources used
 * 
 * http://www.binarytides.com/socket-programming-c-linux-tutorial/
 * 
 * http://beej.us/guide/bgnet/output/html/multipage/advanced.html
 * 
 * http://www.linuxquestions.org/questions/programming-9/
 * tcp-file-transfer-in-c-with-socket-server-client-on-linux-help-with-code-4175413995/
 * 
 * http://www.unixguide.net/unix/programming/2.1.1.shtml
 * 
 * *****************************************************/

/*****************************************************
 * Header files and global variable declarations
 * 
 ****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <sys/stat.h>


#define LISTENQ 10
#define PACKETSIZE 1400


unsigned short LISTENPORT;

fd_set master,readfds;
int maxfd;


/****************************************************
 * These structs stores/sends the server-ip-list
 * integral to the project
 * 
 ***************************************************/

typedef struct server_ip_list{
	int sockfd;
	char hostname[100];
	char ipv4_addr[16];
	char portno[10];
}server_ip_list;

server_ip_list server_list[5];
int server_list_count;

typedef struct peer_ip_list{
	int sockfd;
	char hostname[100];
	char ipv4_addr[16];
	char portno[10];
}peer_ip_list;

peer_ip_list peer_list[5];
int peer_list_count;


/****************************************************
 * Function: getmyipaddr()
 * This functions returns the actual ip of the 
 * host requesting it
 * 
 ***************************************************/
void getmyipaddress(char **myipaddr){
	int socket_fd;
	char my_ip4_buffer[16];
	  
	*myipaddr = malloc(16);  
	  
	struct sockaddr_in google_dns;
	struct sockaddr_in my_ip;

	socket_fd = socket(AF_INET,SOCK_STREAM,0);

	memset(&google_dns,0,sizeof(struct sockaddr_in));
	google_dns.sin_addr.s_addr = inet_addr("8.8.4.4");
	google_dns.sin_family = AF_INET;
	google_dns.sin_port = htons(53);
	
	if(socket_fd < 0){
		perror("could not create socket");
		return;
	}

	if(connect(socket_fd,(struct sockaddr *)&google_dns,sizeof(google_dns))<0){
		perror("could not connect to google dns");
		return;
	}

	socklen_t my_ip_len = sizeof(my_ip);

	if(getsockname(socket_fd,(struct sockaddr *)&my_ip,&my_ip_len)<0){
		perror("could not get connected socket address");
		return;
	}
	
	inet_ntop(AF_INET,&(my_ip.sin_addr),my_ip4_buffer,sizeof(my_ip4_buffer));
	
	strcpy(*myipaddr,my_ip4_buffer);
	
	close(socket_fd);
}


/****************************************************
 * Function: display_serverlist()
 * This function shows the server ip list on the 
 * console
 * 
 * *************************************************/
void display_serverlist(){
	int i = 0;
	for(i=0;i<server_list_count;i++){
		printf("%d: %s\t%s\t%s\n",i+1,server_list[i].hostname,server_list[i].ipv4_addr,server_list[i].portno);
	}
}


/***************************************************
 * Function: removefrom_serveriplist()
 * This function removes a host from the server ip
 * list based upon index
 * 
 * *************************************************/
void removefrom_serveriplist(int index){
	int j;
	for(j=index;j<server_list_count;j++){
		server_list[j-1].sockfd = server_list[j].sockfd;
		strcpy(server_list[j-1].hostname,server_list[j].hostname);
		strcpy(server_list[j-1].ipv4_addr,server_list[j].ipv4_addr);
		strcpy(server_list[j-1].portno,server_list[j].portno);
	}
	server_list_count--;
}


/***************************************************
 * Function:broadcast_serveriplist()
 * This function broadcasts the list to all connected
 * clients
 * 
 * *************************************************/
void broadcast_serveriplist(){
	
	char list[700],list1[100],list2[16],list3[10];
	
	strcpy(list,"L$");
	
	int i;
	for(i=0;i<server_list_count;i++){
		sprintf(list1,"%s ",server_list[i].hostname);
		sprintf(list2,"%s ",server_list[i].ipv4_addr);
		sprintf(list3,"%s ",server_list[i].portno);
		strcat(list,list1);
		strcat(list,list2);
		strcat(list,list3);
	}
	
	//printf("%s\n",list);
	
	int len = strlen(list);
	list[len-1] = '\0';
	
	for(i=1;i<server_list_count;i++){
		
		int clientfd;
		
		char* clientip = server_list[i].ipv4_addr;
		unsigned short clientport = atoi(server_list[i].portno);
		
		struct sockaddr_in client;
		
		if((clientfd = socket(AF_INET,SOCK_STREAM,0))<0){
			perror("socket:broadcast");
			break;
		}
		client.sin_addr.s_addr = inet_addr(clientip);
		client.sin_family = AF_INET;
		client.sin_port = htons(clientport);
		
		if(connect(clientfd,(struct sockaddr *)&client,sizeof(client))<0){
			perror("connect:broadcast");
			break;
		}
		
		if(send(clientfd,list,sizeof(list),0)<0){
				perror("send:broadcast");
				break;
		}
		
		printf("Server-IP-List sent to %s\n",server_list[i].hostname);
		
		//close(clientfd);
	}
	
}


/****************************************************
 * Function: server_command_handler()
 * This function handles all the user inputs when
 * program is running as server
 * 
 * *************************************************/
void server_command_handler(){
	char input[256];
	char *inputparse;
	char *command[7];
	int size,j=0;
	
	fgets(input,sizeof(input),stdin);
	size = strlen(input);
	input[size-1] = '\0';
	
	inputparse = strtok(input," ");
	while(inputparse!=NULL){
		command[j] = inputparse;
		inputparse = strtok(NULL," ");
		j++;
	}
	
	while(1){
		if(strcasecmp(command[0],"help")==0){
			printf("- HELP\n- MYIP\n- MYPORT\n");
			printf("- LIST\n- TERMINATE <connection#>\n");
			printf("- CREATOR\n- EXIT\n");
			break;
		}
		else if(strcasecmp(command[0],"myport")==0){
			printf("Server's listening port is %d\n",LISTENPORT);
			break;
		}
		else if(strcasecmp(command[0],"myip")==0){
			char *ip_addr;
			getmyipaddress(&ip_addr);
			printf("Server's IPv4 address is %s\n",ip_addr);
			break;
		}
		else if(strcasecmp(command[0],"creator")==0){
			printf("Author : Subhendu Saha\n");
			printf("  UBIT : subhendu\n");
			printf(" Email : subhendu@buffalo.edu\n");
			break;
		}
		else if(strcasecmp(command[0],"terminate")==0){
			
			if(j==2){
				/*find out index of client in the serverlist*/
				int index = atoi(command[1]);
				
				if(server_list_count<index){
					printf("Not a valid connection.\n");
					break;
				}
				
				/*reject if user tries to self terminate*/
				if(index==1){
					printf("Cannot terminate self. Try EXIT.\n");
					break;
				}
				
				/*notify remote client about termination*/
				char header[100];
				strcpy(header,"T$");
				strcat(header,server_list[index-1].hostname);
				
				send(server_list[index-1].sockfd,header,sizeof(header),0);
				
				
				/* remove client from master fdset 
				 * & close the connection*/ 
				
				FD_CLR(server_list[index-1].sockfd,&master);
				close(server_list[index-1].sockfd);
				
				
				/* remove client from serveriplist
				 * & bradcast new list to all */
				 
				removefrom_serveriplist(index);
				broadcast_serveriplist();
				
				break;
			}
			printf("Incorrect no of arguments.\n");
			break;
		}
		else if(strcasecmp(command[0],"list")==0){
			display_serverlist();
			break;
		}
		else if(strcasecmp(command[0],"exit")==0){
		
			int i;
			for(i=server_list_count-1;i>=0;i--){
				close(server_list[i].sockfd);
			}
			exit(0);
		}
		else{
			printf("Invalid command. Try Again.\n");
			break;
		}
		
	}//end of while
}


/****************************************************
 * Function: initiate_server_list()
 * This function initiate the server ip lis with 
 * server's details when server is first started
 * 
 * *************************************************/
void initiate_server_list(int serverfd){
	server_list_count = 0;
	char host[100];
	char port[10];
	char *ip_addr;
	
	sprintf(port,"%d",LISTENPORT);
	
	gethostname(host,sizeof(host));
	
	getmyipaddress(&ip_addr);
	
	server_list[server_list_count].sockfd = serverfd;
	strcpy(server_list[server_list_count].hostname,host);
	strcpy(server_list[server_list_count].ipv4_addr,ip_addr);
	strcpy(server_list[server_list_count].portno,port);
		
	//printf("1: %s\t%s\t%s\n",server_list[0].hostname,server_list[0].ipv4_addr,server_list[0].portno);
	
	server_list_count++;
}


/***************************************************
 * Function: create_listener()
 * This function creates the listener socket when 
 * the program is invoked for both server and client
 * 
 * *************************************************/
int create_listener(){
	
	int listenerfd;
	struct sockaddr_in listener;
	int yes = 1;/* for setting port reuse*/
	
	/*creates a TCP socket*/
	listenerfd = socket(AF_INET,SOCK_STREAM,0);
	
	/*setting socket for reuse*/
	setsockopt(listenerfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int));
	
	/*setting values in the struct*/
	listener.sin_family = AF_INET;
	listener.sin_addr.s_addr = INADDR_ANY;
	listener.sin_port = htons(LISTENPORT);
	
	/*binding listener to the port*/
	if(bind(listenerfd,(struct sockaddr *)&listener,sizeof(listener))<0){
		perror("bind");
		return -1;
	}
	
	/*setting up listener*/
	if(listen(listenerfd,LISTENQ)<0){
		perror("listen");
		return -1;
	}
	printf("Listening for connections at port: %d\n",LISTENPORT);
	return listenerfd;
}


/****************************************************
 * Function: start_server()
 * This function invokes the server when the program 
 * is launched as server
 * 
 * *************************************************/
void start_server(){
	
	int remotefd,nbytes,listenerfd;
	char buf[750];
	struct sockaddr_in remote;
	
	int c = sizeof(struct sockaddr_in);
	
	/*calling listener to do initial setup*/
	listenerfd = create_listener();
	
	//serverfd = listenerfd;
	
	/*initiate server ip list*/
	initiate_server_list(listenerfd);
	
	/*this is the maximum now*/
	maxfd = listenerfd;
	
	/*setting initial fd sets zero*/
	FD_ZERO(&master);
	FD_ZERO(&readfds);
	
	/*setting the master with stdin and listener*/
	FD_SET(0,&master);
	FD_SET(listenerfd,&master);
	
	
	while(1){
		
		/*displaying user prompt*/
		printf("Prog1>> ");
		fflush(stdout);
		
		/*setting readfd set to master*/
		readfds = master;
		
		/*calling select to multiplex input*/
		if(select(maxfd+1,&readfds,NULL,NULL,NULL)<0){
			perror("select");
			break;
		}
		
		/*if stdin becomes active*/
		if(FD_ISSET(0,&readfds)){
			server_command_handler();
		}
		
		/*if TCP becomes active*/
		if(FD_ISSET(listenerfd,&readfds)){
			
			/*calling accept to get new connections*/
			if((remotefd = accept(listenerfd,(struct sockaddr*)&remote,&c))<0){
				perror("accept");
				break;
			}
			
			/*set new fd to master set*/
			FD_SET(remotefd,&master);
			
			
			/*update maxfd*/
			if(remotefd>maxfd){
				maxfd = remotefd;
			}
			
			printf("\n");
		}
		else{
			int i;
			for(i=1;i<=maxfd;i++){
				
				/*checking if data available from an existing
				 * connection*/ 
				if(FD_ISSET(i,&readfds) && i!=listenerfd){
					
					if((nbytes=recv(i,buf,sizeof(buf),0))<=0){
						
						/*connection closed by a client*/
						int j,index;
						for(j=1;j<server_list_count;j++){
							if(server_list[j].sockfd == i){
								index = j;break;
							}
						}
						
						printf("\n%s has closed connection.\n",server_list[index].hostname);
						
						removefrom_serveriplist(index+1);
						broadcast_serveriplist();
						
						/*remove fd from master fdset and close connection*/
						FD_CLR(i,&master);
						close(i);
					}
					else{
						
						/*recv data from client*/
						if(strncmp(buf,"R",1)==0){
						
							printf("\nRegister request received.\n");
							
							char *parse, *port;
							struct sockaddr_in client;
							char clientname[120];
							int len = sizeof(client);
								
							parse = strtok(buf,"$");
							parse = strtok(NULL,"$");
							port = parse;
							
							getpeername(i,(struct sockaddr *)&client,&len);
							
							char *ip_addr = inet_ntoa(client.sin_addr);
							
							getnameinfo((const struct sockaddr*)&client,len,clientname,sizeof(clientname),NULL,0,0);
							
							server_list[server_list_count].sockfd = i;
							strcpy(server_list[server_list_count].hostname,clientname);
							strcpy(server_list[server_list_count].ipv4_addr,ip_addr);
							strcpy(server_list[server_list_count].portno,port);
							
							server_list_count++;
							broadcast_serveriplist();
							
						}
					}
				}//end of recv
			}//end of for loop
		}//end of else of FD_ISSET for TCP listener
		
	}//end of while
	
}


/****************************************************
 * Function: removefrom_peeriplist()
 * This function works for client, removes a peer
 * from peer list maintained by client
 * 
 * *************************************************/
void removefrom_peeriplist(int index){
	int j;
	for(j=index;j<peer_list_count;j++){
		peer_list[j-1].sockfd = peer_list[j].sockfd;
		strcpy(peer_list[j-1].hostname,peer_list[j].hostname);
		strcpy(peer_list[j-1].ipv4_addr,peer_list[j].ipv4_addr);
		strcpy(peer_list[j-1].portno,peer_list[j].portno);
	}
	peer_list_count--;
}


/****************************************************
 * Function: process_peeriplist()
 * This function adjusts the local peer list when 
 * server sends new server ip list
 * 
 * *************************************************/
void process_peeriplist(){

	if(peer_list_count==0){
		strcpy(peer_list[0].hostname,server_list[0].hostname);
		strcpy(peer_list[0].ipv4_addr,server_list[0].ipv4_addr);
		strcpy(peer_list[0].portno,server_list[0].portno);
		
		peer_list_count++;
		
		char host[100];
		gethostname(host,sizeof(host));
		
		int i;
		for(i=1;i<server_list_count;i++){
			if(strcasecmp(server_list[i].hostname,host)==0)
				break;
		}
		
		strcpy(peer_list[1].hostname,server_list[i].hostname);
		strcpy(peer_list[1].ipv4_addr,server_list[i].ipv4_addr);
		strcpy(peer_list[1].portno,server_list[i].portno);
		
		peer_list_count++;
		
		return;
	}
	
	int i,j,k,flag=1;
	for(i=2;i<peer_list_count;i++){
		for(j=1;j<server_list_count;j++){
			if(strcasecmp(peer_list[i].hostname,server_list[j].hostname)==0){
				i=i+1;flag=0;
				break;
			}
		}
		if(flag==1){
			for(k=i;k<peer_list_count-1;k++){
				strcpy(peer_list[k].hostname,peer_list[k+1].hostname);
				strcpy(peer_list[k].ipv4_addr,peer_list[k+1].ipv4_addr);
				strcpy(peer_list[k].portno,peer_list[k+1].portno);
			}
			peer_list_count--;
		}
	}
}


/***************************************************
 * Function: process_serveriplist()
 * This function parses the string send by server
 * and stores it in local server list
 * 
 * ************************************************/
void process_serveriplist(char* buf){
	
	char* bufferparse;
	char* buffer[15];
	int i,size=0;
	
	
	bufferparse = strtok(buf," ");
	while(bufferparse!=NULL){
		buffer[size] = bufferparse;
		bufferparse = strtok(NULL," ");
		size++;
	}
	
	server_list_count = 0;
	
	for(i=0;i<size;i++){
	
		strcpy(server_list[server_list_count].hostname,buffer[i]);
		strcpy(server_list[server_list_count].ipv4_addr,buffer[i+1]);
		strcpy(server_list[server_list_count].portno,buffer[i+2]);
		server_list_count++;
		i = i + 2;
	}
	
}


/**************************************************
 * Function: upload_file()
 * This function conatins the logic of creating
 * a new TCP connection and sending file to the 
 * peer requesting it
 * 
 * ***********************************************/
void upload_file(int index,char *filepath){
	
	int peer_fd;
	char peer_addr[16];
	strcpy(peer_addr,server_list[index-1].ipv4_addr);
	unsigned short peerport = atoi(server_list[index-1].portno);
	
	char sendname[100],recvname[100];
	char *sender,*receiver;
	
	gethostname(sendname,sizeof(sendname));
	strcpy(recvname,server_list[index-1].hostname);
	
	
	sender = strtok(sendname,".");
	
	receiver = strtok(recvname,".");
	
	
	peer_fd = socket(AF_INET,SOCK_STREAM,0);
	
	struct sockaddr_in peer;
	struct stat filestat;
	
	peer.sin_addr.s_addr = inet_addr(peer_addr);
	peer.sin_family = AF_INET;
	peer.sin_port = htons(peerport);
	
	connect(peer_fd,(struct sockaddr *)&peer,sizeof(peer));
	
	if(stat(filepath,&filestat)<0){
		printf("File doesn't exist.\n");
		return;
	}
	
	int filesize = filestat.st_size;
	char *filename,*token;
	char filesizestr[10];
	
	token = strtok(filepath,"/");
	while(token!=NULL){
		filename = token;
		token = strtok(NULL,"/");
	}
	
	FILE *fp;
	if((fp=fopen(filepath,"r"))==NULL){
		printf("File doesn't exist\n'");
		return;
	}
	
	
	char header[50];
	
	/*create upload header with filename and size*/
	sprintf(filesizestr,"%d",filesize);
	strcpy(header,"U$");
	strcat(header,filename);
	strcat(header,"$");
	strcat(header,filesizestr);
	
	/*send header to peer*/
	send(peer_fd,header,sizeof(header),0);
	
	char sendbuffer[PACKETSIZE];
	
	bzero(sendbuffer,PACKETSIZE);
	int send_size,flag=0;
	
	time_t start,end;
	
	time(&start);
	
	/*sending the actual file*/
	while((send_size = fread(sendbuffer,sizeof(char),PACKETSIZE,fp))>0){
		
		if(send(peer_fd,sendbuffer,send_size,0)<0){
			perror("send:could not send file\n");
			flag = 1;
			break;
		}
		bzero(sendbuffer,PACKETSIZE);
	}
	
	fclose(fp);
	if(flag == 1){
		return;
	}
	time(&end);
	double time_diff = difftime(end,start);
	double rate = ((double)filesize/time_diff)*8;
	
	printf("Upload successful\n");
	printf("Tx (%s): %s -> %s, FileSize: %d Bytes, TimeTaken: %.0f seconds, Tx Rate: %.0f bits/second\n",sender,sender,receiver,filesize,time_diff,(float)rate);
}


/**************************************************
 * Function: client_command_handler()
 * This function handles the user inputs when the 
 * program is invoked as client
 * 
 * ***********************************************/
void client_command_handler(){
	
	char input[256];
	char *inputparse;
	char *command[7];
	int size,j=0;
	
	fgets(input,sizeof(input),stdin);
	size = strlen(input);
	input[size-1] = '\0';
	
	inputparse = strtok(input," ");
	while(inputparse!=NULL){
		command[j] = inputparse;
		inputparse = strtok(NULL," ");
		j++;
	}
	
	while(1){
		if(strcasecmp(command[0],"help")==0){
			printf("- HELP\n- MYIP\n- MYPORT\n");
			printf("- REGISTER <server ip> <port#>\n");
			printf("- CONNECT <connection#> <port#>\n");
			printf("- LIST\n- TERMINATE <connection#>\n");
			printf("- UPLOAD <connection#> <file_name>\n");
			printf("- DOWNLOAD [<connection#> <file_name>,..]\n");
			printf("- CREATOR\n- EXIT\n");
			break;
		}
		else if(strcasecmp(command[0],"myport")==0){
			printf("Server's listening port is %d\n",LISTENPORT);
			break;
		}
		else if(strcasecmp(command[0],"myip")==0){
			char *ip_addr;
			getmyipaddress(&ip_addr);
			printf("Server's IPv4 address is %s\n",ip_addr);
			break;
		}
		else if(strcasecmp(command[0],"creator")==0){
			printf("Author : Subhendu Saha\n");
			printf("  UBIT : subhendu\n");
			printf(" Email : subhendu@buffalo.edu\n");
			break;
		}
		else if(strcasecmp(command[0],"register")==0){
			
			int serverfd;
			
			char* serverip = command[1];
			unsigned short serverport = (unsigned short)atoi(command[2]);
			
			char port[10],header[20];
			
			sprintf(port,"%d",LISTENPORT);
			
			struct sockaddr_in server;
			
			if((serverfd = socket(AF_INET,SOCK_STREAM,0))<0){
				perror("socket:register");
				break;
			}
			server.sin_addr.s_addr = inet_addr(serverip);
			server.sin_family = AF_INET;
			server.sin_port = htons(serverport);
			
			if(connect(serverfd,(struct sockaddr *)&server,sizeof(server))<0){
				perror("connect:register");
				break;
			}
			
			/*create header for register request*/
			strcpy(header,"R$");
			strcat(header,port);
			
			if(send(serverfd,header,sizeof(header),0)<0){
				perror("send:register");
				break;
			}
			
			printf("Registered with server..\n");
			
			break;
			
		}
		else if(strcasecmp(command[0],"terminate")==0){
			
			if(j==2){
				/*find out index of client in the serverlist*/
				int index = atoi(command[1]);
				
				if(index==1){
					printf("Cannot terminate connection to server.\n");
					break;
				}
				
				/*reject if user tries to self terminate*/
				
				char host[100];
				gethostname(host,sizeof(host));
				
				if(strcasecmp(host,server_list[index-1].hostname)==0){
					printf("Cannot terminate self.Try Exit.\n");
					break;
				}
				
				/*invalid termination request*/
				int i,peerindex=-1;
				for(i=2;i<peer_list_count;i++){
					if(strcasecmp(peer_list[i].hostname,server_list[index-1].hostname)==0){
						peerindex = i;
						break;
					}
				}
				
				if(peerindex!=-1){
					/*notify remote peer about termination*/
					char header[100];
					strcpy(header,"T$");
					strcat(header,peer_list[1].hostname);
					
					int peerfd;
			
					char* peerip = server_list[index-1].ipv4_addr;
					unsigned short peerport = atoi(server_list[index-1].portno);
					
					struct sockaddr_in peer;
					
					if((peerfd = socket(AF_INET,SOCK_STREAM,0))<0){
						perror("socket:terminate");
						break;
					}
					peer.sin_addr.s_addr = inet_addr(peerip);
					peer.sin_family = AF_INET;
					peer.sin_port = htons(peerport);
					
					if(connect(peerfd,(struct sockaddr *)&peer,sizeof(peer))<0){
						perror("connect:terminate");
						break;
					}
					
					if(send(peerfd,header,sizeof(header),0)<0){
							perror("send:terminate");
							break;
					}
					
					/* remove client from master fdset 
					 * & close the connection*/ 
					
					FD_CLR(peer_list[index-1].sockfd,&master);
					close(peer_list[index-1].sockfd);
					
					
					/* remove client from peeriplist*/
					 
					removefrom_peeriplist(peerindex);
				}
				else{
					printf("You are not connected to requested host.\n");
				}
				break;
			}
			else{
				printf("Incorrect no of arguments.\n");
			}
			break;
		}
		
		else if(strcasecmp(command[0],"list")==0){
			display_serverlist();
			break;
		}
		
		else if(strcasecmp(command[0],"connect")==0){
			if(j==3){
				
				char* destination = command[1];
				unsigned short port = atoi(command[2]);
				
				char* connectip;
				
				connectip = malloc(16);
				//printf("%s\n",destination);
				
				int i,flag=-1;
				for(i=1;i<server_list_count;i++){
					if(strcasecmp(server_list[i].hostname,destination)==0 || strcasecmp(server_list[i].ipv4_addr,destination)==0){
						strcpy(connectip,server_list[i].ipv4_addr);
						flag = i;break;
					}
				}
				
				for(i=1;i<peer_list_count;i++){
					if(strcasecmp(connectip,peer_list[i].ipv4_addr)==0){
						if(i==1){
							printf("Self connection not allowed.\n");
						}
						else{
							printf("Duplicate connection not allowed.\n");
						}
						flag = -1;
						break;
					}
				}
				
				if(flag!=-1){
				
					if(peer_list_count<5){
						struct sockaddr_in peer;
						int peer_fd;
						
						if((peer_fd=socket(AF_INET,SOCK_STREAM,0))<0){
							perror("socket:connect");
							break;
						}
						
						peer.sin_addr.s_addr = inet_addr(connectip);
						peer.sin_family = AF_INET;
						peer.sin_port = htons(port);
						
						if(connect(peer_fd,(struct sockaddr *)&peer,sizeof(peer))<0){
							perror("connect:connect");
							break;
						}
						
						peer_list[peer_list_count].sockfd = peer_fd;
						strcpy(peer_list[peer_list_count].hostname,server_list[flag].hostname);
						strcpy(peer_list[peer_list_count].ipv4_addr,server_list[flag].ipv4_addr);
						strcpy(peer_list[peer_list_count].portno,server_list[flag].portno);
						
						peer_list_count++;
						
						FD_SET(peer_fd,&master);
						if(peer_fd>maxfd){
							maxfd = peer_fd;
						}
					}
					else{
						printf("Maximum peer connections reached.\n");
					}
				}
				else{
					printf("Invalid hostname/IPaddress.\n");
				}
				
				free(connectip);
				
			}
			else{
				printf("Incorrect no of arguments.\n");
			}
			break;
		}
		
		else if(strcasecmp(command[0],"exit")==0){
		
			int i;
			for(i=peer_list_count-1;i>=0;i--){
				close(peer_list[i].sockfd);
			}
			exit(0);
		}
		
		else if(strcasecmp(command[0],"upload")==0){
			
			if(j==3){
				int index = atoi(command[1]);
				char filepath[100];
				strcpy(filepath,command[2]);
				int peer_fd;
				
				if(index==1){
					printf("Upload to server not allowed.\n");
					break;
				}
				
				if(index>server_list_count){
					printf("Invalid connectionid.\n");
					break;
				}
				
				upload_file(index,filepath);
					
			}
			else{
				printf("Incorrect no of arguments.\n");
			}
			break;
		}
		
		else if(strcasecmp(command[0],"download")==0){
		
			if(j!=3 && j!=5 && j!=7){
				printf("Incorrect no of arguments.\n");
				break;
			}
			else{
				int i;
				for(i=0;i<j-1;i++){
					i++;
					int index = atoi(command[i]);
					i++;
					char *filepath = command[i];
					
					//printf("%d\n",i);
					
					char header[100];
					
					int peer_fd;
					struct sockaddr_in peer;
					
					peer_fd = socket(AF_INET,SOCK_STREAM,0);
					
					peer.sin_addr.s_addr = inet_addr(server_list[index-1].ipv4_addr);
					peer.sin_family = AF_INET;
					peer.sin_port = htons(atoi(server_list[index-1].portno));
					
					connect(peer_fd,(struct sockaddr*)&peer,sizeof(peer));
					
					strcpy(header,"D$");
					strcat(header,filepath);
					
					send(peer_fd,header,sizeof(header),0);
					
					//printf("hello %d\n",i);
					
					i--;
				}
				printf("Download in progress..\n");
			}
			break;
		}
		else{
			printf("Invalid command. Try Again.\n");
			break;
		}
		
	}//end of while
}


/**************************************************
 * Function: start_client()
 * This function starts client when program is 
 * invoked as client. This contains the main
 * logic behind select()
 * 
 * ***********************************************/
void start_client(){
		
	int remotefd,nbytes,listenerfd;
	char buf[750];
	struct sockaddr_in remote;
	
	int c = sizeof(struct sockaddr_in);
	
	/*calling listener to do initial setup*/
	listenerfd = create_listener();
	
	//serverfd = listenerfd;
	
	/*initiate server ip list*/
	server_list_count = 0;
	peer_list_count = 0;
	
	/*this is the maximum now*/
	maxfd = listenerfd;
	
	/*setting initial fd sets zero*/
	FD_ZERO(&master);
	FD_ZERO(&readfds);
	
	/*setting the master with stdin and listener*/
	FD_SET(0,&master);
	FD_SET(listenerfd,&master);
	
	int d = 1;
	
	while(1){
		
		/*displaying user prompt*/
		printf("Prog1>> ");
		fflush(stdout);
		
		//printf("%d\n",d);
		
		/*setting readfd set to master*/
		readfds = master;
		
		//printf("%d\n",d);
		
		/*calling select to multiplex input*/
		if(select(maxfd+1,&readfds,NULL,NULL,NULL)<0){
			perror("select");
			break;
		}
		
		//printf("%d\n",d);
		
		/*if stdin becomes active*/
		if(FD_ISSET(0,&readfds)){
			//printf("%d\n",d);
			client_command_handler();
		}
		
		//printf("%d\n",d);
		
		if(FD_ISSET(listenerfd,&readfds)){
			
			//printf("%d\n",d);
			
			/*calling accept to get new connections*/
			if((remotefd = accept(listenerfd,(struct sockaddr*)&remote,&c))<0){
				perror("accept");
				break;
			}
			
			/*set new fd to master set*/
			FD_SET(remotefd,&master);
			
			
			/*update maxfd*/
			if(remotefd>maxfd){
				maxfd = remotefd;
			}
			
			printf("\n");
		
		}//end of TCP
		else{
			
			//printf("%d\n",d);
			
			int i;
			for(i=1;i<=maxfd;i++){
				
				/*checking if data available from an existing
				 * connection*/ 
				if(FD_ISSET(i,&readfds) && i!=listenerfd){
					
					//printf("%d\n",d);
					
					if((nbytes=recv(i,buf,sizeof(buf),0))<=0){
						
						printf("\n");
						FD_CLR(i,&master);
						//close(i);
						
					}
					else{
						
						/*recv data from server*/
						if(strncmp(buf,"L",1)==0){
						
							char *tempbuf;
							tempbuf = strtok(buf,"$");
							tempbuf = strtok(NULL,"$");
							
							printf("\nServer-IP-list received.");
							
							process_serveriplist(tempbuf);
							printf("\n");
							display_serverlist();
							process_peeriplist();
							//printf("Hello\n");
							
						}
						else if(strncmp(buf,"T",1)==0){
							
							char *tempbuf;
							tempbuf = strtok(buf,"$");
							tempbuf = strtok(NULL,"$");
							
							printf("\n%s terminated connection.\n",tempbuf);
							
							int j;
							for(j=2;j<peer_list_count;j++){
								if(strcasecmp(tempbuf,peer_list[j].hostname)==0){
									break;
								}
							}
							removefrom_peeriplist(j+1);
						}
						else if(strncmp(buf,"U",1)==0){
						
							char *parse,*filename,*filesizestr;
							int filesize;
							struct sockaddr_in senderinfo;
							int len = sizeof(senderinfo);
							
							char sendname[100],recvname[100];
							char *sender,*receiver;
							
							gethostname(recvname,sizeof(recvname));
							receiver = strtok(recvname,".");
							
							getpeername(i,(struct sockaddr *)&senderinfo,&len);
							getnameinfo((const struct sockaddr*)&senderinfo,len,sendname,sizeof(sendname),NULL,0,0);
							
							sender = strtok(sendname,".");
							
							/*get header information*/
							parse = strtok(buf,"$");
							parse = strtok(NULL,"$");
							filename = parse;
							parse = strtok(NULL,"$");
							filesizestr = parse;
							filesize = atoi(filesizestr);
							
							char recvbuffer[PACKETSIZE];
							int nofbytes = 0,bytes_recv;
							
							FILE *fp;
							
							if((fp = fopen(filename,"a"))==NULL){
								printf("Could not create file\n");
								break;
							}
							
							bzero(recvbuffer,PACKETSIZE);
							
							time_t start,end;
							time(&start);
							
							int flag = 1;
							
							while(nofbytes < filesize){
								bytes_recv = recv(i,recvbuffer,sizeof(recvbuffer),0);
								int writesize;
								if((writesize=fwrite(recvbuffer,sizeof(char),bytes_recv,fp))<bytes_recv){
									printf("Failed to write to file\n");
									flag = 0;break;
								}
								nofbytes = nofbytes + bytes_recv;
								bzero(recvbuffer,PACKETSIZE);
							}
							
							time(&end);
							fclose(fp);
							if(flag==0){
								break;
							}
							
							double time_diff = difftime(end,start);
							double rate = ((double)filesize/time_diff)*8;
							
							printf("\nDownload successful\n");
							printf("Rx (%s): %s -> %s, FileSize: %d Bytes, TimeTaken: %.0f seconds, Tx Rate: %.0f bits/second\n",receiver,sender,receiver,filesize,time_diff,(float)rate);
							fflush(stdout);
							
							FD_CLR(i,&master);
						}
						else if(strncmp(buf,"D",1)==0){
							
							char *filepath;
							filepath = strtok(buf,"$");
							filepath = strtok(NULL,"$");
							
							struct sockaddr_in peer;
							int len = sizeof(peer);
							
							char peername[100];
							
							getpeername(i,(struct sockaddr *)&peer,&len);
							getnameinfo((const struct sockaddr*)&peer,len,peername,sizeof(peername),NULL,0,0);
							
							int index;
							int i;
							for(i=0;i<server_list_count;i++){
								if(strcasecmp(peername,server_list[i].hostname)==0){
									index = i;break;
								}
							}
							
							if(index==0){
								printf("\nDownload from server not allowed.\n");
								break;
							}
							printf("\n");
							upload_file(index+1,filepath);
							
						}
					}
				}//end of recv
				
			}//end of for loop
		}//end of else of TCP
		d++;
	}//end of while
}


/*************************************************
 * Function: main()
 * This is where program starts and depending upon
 * command line args, server/client part is invoked
 * 
 * ************************************************/

int main(int argc, char **argv){
	if(argc!=3){
		fprintf(stderr,"Usage: prog1 [s|c] <port#>\n");
		exit(EXIT_FAILURE);
	}
	
	LISTENPORT = atoi(argv[2]);
	
	if(strcasecmp(argv[1],"s")==0){
		start_server();
	}
	else if(strcasecmp(argv[1],"c")==0){
		start_client();
	}
	
	return 0;
}



