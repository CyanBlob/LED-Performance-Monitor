#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <regex.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ERROR_MSG 0x1000
#define BUFSIZE 128

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

float get_resource(const char *cmd, const char *regexText)
{
	regex_t reg;

	int status = regcomp (&reg, regexText, REG_EXTENDED|REG_NEWLINE);
	if (status != 0)
	{
		char error_message[MAX_ERROR_MSG];
		regerror (status, &reg, error_message, MAX_ERROR_MSG);
		printf ("Regex error compiling '%s': %s\n",
			regexText, error_message);
		return -1;
	}

	FILE *fp;

	if ((fp = popen(cmd, "r")) == NULL)
	{
		printf("Error opening pipe!\n");
		return -1;
	}

	char *buf = (char*) malloc(BUFSIZE);
	while (fgets(buf, BUFSIZE, fp) != NULL)
	{
	}

	if(pclose(fp))
	{
		printf("Command not found or exited with error status\n");
		free(buf);
		return -1;
	}

	regmatch_t pmatch[1];
	status = regexec(&reg, buf, 1, pmatch, 0);
	regfree(&reg);
	if (!status)
	{
		float cpu = atof(buf + pmatch[0].rm_so);

		free(buf);
		return cpu;
	}

	free(buf);
	return -1;
} //get_resource(void)

float get_cpu()
{
	const char *cmd = "mpstat 1 1";
	const char *regexText = "[0-9]*\\.[0-9]*$";
	return 100.0 - get_resource(cmd, regexText);
}

float get_mem()
{
	const char *cmdTotal = "cat /proc/meminfo | grep MemTotal ";
	const char *regexTextTotal = "[0-9]+";
	float total = get_resource(cmdTotal, regexTextTotal);

	const char *cmdFree = "cat /proc/meminfo | grep MemAvailable";
	const char *regexTextFree = "[0-9]+";
	float free = get_resource(cmdFree, regexTextFree);

	return 100.0 - ((free / total) * 100);
}

float get_swap()
{
	const char *cmdTotal = "cat /proc/meminfo | grep SwapTotal ";
	const char *regexTextTotal = "[0-9]+";
	float total = get_resource(cmdTotal, regexTextTotal);

	const char *cmdFree = "cat /proc/meminfo | grep SwapFree";
	const char *regexTextFree = "[0-9]+";
	float free = get_resource(cmdFree, regexTextFree);

	return 100.0 - ((free / total) * 100);
}

char get_all(char* buf)
{
	buf[0] = '<';
	sprintf(buf + strlen(buf), "%2.2f", get_cpu());
	sprintf(buf + strlen(buf), ",");
	sprintf(buf + strlen(buf), "%2.2f", get_mem());
	sprintf(buf + strlen(buf), ",");
	sprintf(buf + strlen(buf), "%2.2f>", get_swap());
	return *buf;
}

int main(int argc, char *argv[])
{
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int n;

	char buf[32];

	if (argc < 2)
	{
		fprintf(stderr,"ERROR, no port provided\n");
		exit(1);
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
	{
		error("ERROR opening socket");
	}

	bzero((char *) &serv_addr, sizeof(serv_addr));

	portno = atoi(argv[1]);

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);

	if (bind(sockfd, (struct sockaddr *) &serv_addr,
		 sizeof(serv_addr)) < 0)
	{
		error("ERROR on binding");
	}

	listen(sockfd, 5);
	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd,
			   (struct sockaddr *) &cli_addr,
			   &clilen);

	printf("Client connected");
	if (newsockfd < 0)
	{
		error("ERROR on accept");
	}

	while (1)

	{
		memset(buf, 0, 32);
		get_all(buf);
		printf("%s\n",buf);

		n = write(newsockfd, buf, strlen(buf));
		if (n < 0)
		{
			error("ERROR writing to socket");
		}
		else
		{
			printf("Wrote %d bytes\n", n);
		}
	}
	return 0;
}
