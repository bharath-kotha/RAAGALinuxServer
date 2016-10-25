#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main()
{
	int sock, cli;
	struct sockaddr_in server, client;
	unsigned int len;

	if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
	{
		fprintf(stderr,"Error creating sockets");
		exit(1);
	}

	server.sin_family = AF_INET;
	server.sin_port = htons(1111);
	server.sin_addr.s_addr = INADDR_ANY;
	//memset(&server.sin_zero,0,8 );
	int i;
	for(i =0 ; i< 8 ; i++)
	{
		server.sin_zero[i] = 0;
	}

	len = sizeof(struct sockaddr_in);
	if(bind(sock,(struct sockaddr*) &server, len) == -1)
	{
		fprintf(stderr, "Bind\n");
		exit(1);
	}

	if(listen(sock,5) == -1)
	{
		fprintf(stderr, "listen");
		exit(1);
	}
	
	while(1)
	{
		if((cli = accept(sock, (struct sockaddr *) & client, &len)) == -1)
		{
			fprintf(stderr, "accept");
			exit(-1);
		}

		char MOTD[256] = "Welcome! This is the message of the day";
		send(cli, MOTD, sizeof(MOTD), 0);

		//close(cli);
	}

	


return 0;
}
