#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#define unrestricted 0
#define exclusive 1
#define transaction 2


struct hostent *server;
int portNum = 8004;
char fmode[20];
char filename[100];
char flag[3];

void error(char *message){
	perror(message);
	exit(0);
}
int netserverinit(char *hostname, int filemode){
	server = gethostbyname(hostname);
	if(server == NULL){
		error("Hostname does not exist.\n");
	}
	if(filemode == unrestricted){
		sprintf(fmode, "unrestricted");
	}else if(filemode == exclusive){
		sprintf(fmode, "exclusive");
	}else if(filemode == transaction){
		sprintf(fmode, "transaction");
	}else{
		error("INVALID_FILE_MODE \n");
	}
	return 0;
}

int netopen(const char *pathname, int flags){
	int sockfd, n;
	
	sprintf(filename, "%s", pathname);
	
	char buffer[256];

	if (flags == O_RDONLY){
		sprintf(buffer, "OPEN %s %s r ", fmode, pathname);
		sprintf(flag, "r");
	}
	else if (flags == O_WRONLY){
		sprintf(buffer, "OPEN %s %s a ", fmode, pathname);
		sprintf(flag, "a");
	}
	else if (flags == O_RDWR){
		sprintf(buffer, "OPEN %s %s a+ ", fmode, pathname);
		sprintf(flag, "a+");
	}
	else{
		error("Try again with a proper flag.");
	}

    struct sockaddr_in serveradd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		error("ERROR opening socket");
	}
	bzero(&serveradd, sizeof(serveradd));
    serveradd.sin_family = AF_INET;
	bcopy(server->h_addr, &serveradd.sin_addr.s_addr, server->h_length);
	
	serveradd.sin_port = htons(portNum);
	
	if (connect(sockfd, (struct sockaddr *)&serveradd, sizeof(serveradd)) < 0){
		error("ERROR connecting");
	}
	
	n = write(sockfd, buffer, strlen(buffer));
	if (n < 0) 
		error("ERROR writing to socket");
		
	bzero(buffer, 256);
	n = read(sockfd, buffer, 255);
	if (n < 0) {
		printf("%s \n", buffer);
		return -1; 
	}
	char zero[1];
	sprintf(zero, "0");
	int zerotest = strcmp(buffer, zero);
	if(zerotest == 0){
		int z = 0;
		close(sockfd);
		return z;
	}
	char ermsg[256];
	sprintf(ermsg, "This file is already open in write mode by another client.");
	int test = strcmp(buffer, ermsg);
	if(test == 0){
		error("Access was denied\n");
	}
	printf("%s\n", buffer);
	int num = atoi(buffer);
	if (num == 0){
		error("ERROR was not able to convert string to int properly.");
	}
	close(sockfd);
	return num;
}

ssize_t netread(int fildes, void *buf, size_t nbyte){
	int sockfd, n;
	char message[256];
	char count[100];
	sprintf(message, "READ %d buf %zd ",fildes, nbyte);

	struct sockaddr_in serveradd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0){
		error("ERROR opening socket");
	}
	bzero((char *) &serveradd, sizeof(serveradd));
	serveradd.sin_family = AF_INET;
	bcopy((char *) server->h_addr, (char *)&serveradd.sin_addr.s_addr, server->h_length);
	printf("Created a socket \n");

	serveradd.sin_port = htons(portNum);
	printf("Created a port number \n");

	if(connect(sockfd, (struct sockaddr *)&serveradd, sizeof(serveradd)) < 0){
		error("ERROR connecting to socket");
	}
	printf("Connected to the socket \n");

	n = write(sockfd,message,strlen(message));
	if (n < 0) 
		error("ERROR writing to socket");
	
	n = recvfrom(sockfd, count, nbyte, 0, NULL, NULL);
	if (n < 0){
		printf("%s\n", count);
		return -1;
	}
	printf("%s \n", count);
	char zero[1];
	sprintf(zero, "0");
	int zerotest = strcmp(count, zero);
	if(zerotest == 0){
		int z = 0;
		close(sockfd);
		return z;
	}
	int num = atoi(count);
	if(num == 0){
		error("ERROR was not able to convert string to int properly.");
	}
	close(sockfd);
	return num;
}

ssize_t netwrite(int fildes, const void *buf, size_t nbyte){
    char readLine[nbyte];
    bzero(readLine, nbyte);
    char storeMessage[nbyte];
    int sockfd;
    struct sockaddr_in servAddr;
	int bytesWritten = 0;
	char originalbuff[sizeof(buf)];

	sprintf(originalbuff, "%s", buf);
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = (*(unsigned long *)(server->h_addr));
    servAddr.sin_port = htons(portNum);
    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
        error("Could NOT connect to server.");
    }
	printf("Connected to server\n");
	
		sprintf(storeMessage, "WRITE %d %zd %s ", fildes, nbyte, buf);
		sendto(sockfd, storeMessage, strlen(storeMessage), 0, (struct sockaddr *) &servAddr, sizeof(servAddr));
		int n;
		n = recvfrom(sockfd, readLine, nbyte, 0, NULL, NULL);
		if (n < 0){ 
			printf("%s \n", readLine);
			return -1;
		}
		char zero[1];
		sprintf(zero, "0");
		int zerotest = strcmp(readLine, zero);
		if(zerotest == 0){
			int z = 0;
			close(sockfd);
			return z;
		}
		
		bytesWritten = atoi(readLine);
		if (bytesWritten < 0){
			error("Failed to read from fildes.");
		}
		else if (bytesWritten == 0) {
			printf("Write length of zero.\n");
		}
	//}
    close(sockfd);
    return bytesWritten;
}

int netclose(int fd){
    char readLine[2000];
    bzero(readLine, 2000);
    char storeMessage[2000];
    int sockfd;
    struct sockaddr_in servAddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    bzero(&servAddr, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = (*(unsigned long *)(server->h_addr));
    servAddr.sin_port = htons(portNum);
    if (connect(sockfd, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0){
        error("Could NOT connect to server.");
    }

    sprintf(storeMessage, "CLOSE %d %s %s %s ", fd, fmode, filename, flag);
    sendto(sockfd, storeMessage, strlen(storeMessage), 0, (struct sockaddr *) &servAddr, sizeof(servAddr));
    printf("%s\n", storeMessage);
    int result;
	int n;
    n = recvfrom(sockfd, readLine, sizeof(readLine), 0, NULL, NULL);
	if (n < 0){ 
		printf("%s \n", readLine);
	return -1;
	}
	result = atoi(readLine);
    if (result < 0){
        perror("Failed to close fildes.");
        return -1;
    }
    else if (result == 0){
        printf("Fildes has been closed.\n");
    }

    close(sockfd);
    return 0;
}


int main(int argc, char* argv[]){
	netserverinit(argv[1], exclusive);
	int netfd = netopen("test.txt", O_WRONLY);
	printf("Netfd from Open: %d\n", netfd);

	char buffer[200];
	int bytesRead = netread(netfd, buffer, 200);
	if (bytesRead < 0){
		printf("ERROR: Reading was not completed.\n");
	}
	else{
		printf("%d bytes were read from netfd.\n", bytesRead);
	}

	printf("Netfd from Read: %d \n", netfd);

	char message[200];
	strcpy(message, "testing my server\0");
	int bytesWritten = netwrite(netfd, message, strlen(message));
	if (bytesWritten < 0){
		printf("ERROR: Writing was not completed.\n");
	}
	else{
		printf("%d bytes were written into netfd.\n", bytesWritten);
	}

	int close = netclose(netfd);
	if (close < 0){
		printf("ERROR: Close was not completed.\n");
	}
	else{
		printf("Netfd was closed.\n");
	}
	return 0;
}