#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>
#include "sbuf.h"
#include "sbuf.c"
#include <string.h>

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define BUF_SIZE 500

static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (Macintosh; Intel Mac OS X 10.15; rv:97.0) Gecko/20100101 Firefox/97.0";

int all_headers_received(char *);
int parse_request(char *, char *, char *, char *, char *, char *);
void test_parser();
void print_bytes(unsigned char *, int);
int open_sfd(char *port);
void *handle_clients(void *vargp);
void handle_client(int cfd);

sbuf_t sbuf;
int v = 0;

int main(int argc, char *argv[]) {
	struct sockaddr_storage peer_addr;
        socklen_t peer_addr_len = sizeof(struct sockaddr_storage);

        int sfd = open_sfd(argv[1]);
        int cfd;

	pthread_t tid;

	sbuf_init(&sbuf, 5);
	for (int i = 0; i < 8; i++)
		pthread_create(&tid, NULL, handle_clients, NULL);  

	while(1) {
		if (v == 1) {
                        printf("Calling accept\n");
                }

		memset (&peer_addr, 0, sizeof(peer_addr));

		if ((cfd = accept(sfd, (struct sockaddr*)&peer_addr, &peer_addr_len)) == -1) {
			printf("accept error\n");
			exit(EXIT_FAILURE);
		}

		sbuf_insert(&sbuf, cfd);
	}
	close(cfd);
        close(sfd);
        printf("%s\n", user_agent_hdr);
	return 0;
}


// This function takes in the following arg: request: an HTTP request string with no body that s>//
// The function checks whether or not that end-of-header sequence is present in the request mean>//
// Returns 1 if EOH sequence present and 0 if not
int all_headers_received(char *request) {
        if (strstr(request, "\r\n\r\n") == NULL) {
                return 0;
        }
        else {
                return 1;
        }
}

int open_sfd(char *port) {
    int sfd;
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(port));
    addr.sin_addr.s_addr = 0;

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket error\n");
        exit(EXIT_FAILURE);
    }
    else {
        if (v == 1) {
            printf("socket works\n");
        }
    }

    int optval = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        perror("setsockopt error\n");
        exit(EXIT_FAILURE);
    }
    else {
        if (v == 1)
            printf("setsockopt works\n");
    }

    if (bind(sfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) == -1) {
        close(sfd);
        perror("bind error\n");
        exit(EXIT_FAILURE);
    }
    else {
        if (v == 1) {
            printf("bind works\n");
        }
    }

    if (listen(sfd, MAX_OBJECT_SIZE) == -1) {
        close(sfd);
        perror("listen error\n");
        exit(EXIT_FAILURE);
    }
    else {
        if (v == 1) {
            printf("listen works\n");
        }
    }
//    close(sfd);
    return sfd;

}

void *handle_clients(void *vargp) {
	pthread_detach(pthread_self());
	while (1) {
		int cfd = sbuf_remove(&sbuf);
		handle_client(cfd);
		close(cfd);
	}
}

void handle_client(int cfd) {
        if (v == 1) {
                printf("Starting handle_client()");
        }

        char returnBuf[MAX_OBJECT_SIZE];
        memset(returnBuf, 0, MAX_OBJECT_SIZE);

	char requestBuf[MAX_OBJECT_SIZE];
        memset(requestBuf, 0, MAX_OBJECT_SIZE);

        // Recieve HTTP Request
	int totBytesRead = 0;
	int cBytes = 0;

	while (!all_headers_received(returnBuf)) {
		cBytes = read(cfd, returnBuf + totBytesRead, MAX_OBJECT_SIZE);
		totBytesRead += cBytes;
	}

        // Print the contents of the HTTP request
        if (v == 1) {
                printf("Beginning Byte Read");
                char *endRead = strchr((char *)returnBuf, '\x00');
                int lenRead = endRead - (char *)returnBuf;
                print_bytes((unsigned char *)returnBuf, lenRead);
                printf("End of Byte Read");
        }

	// Parse and store request information
	char method[16], hostname[64], port[8], path[64], headers[1024];

	if (parse_request(returnBuf, method, hostname, port, path, headers)) {
                printf("METHOD: %s\n", method);
                printf("HOSTNAME: %s\n", hostname);
                printf("PORT: %s\n", port);
                printf("PATH: %s\n", path);
                printf("HEADERS: %s\n", headers);
        }
        else {
                printf("REQUEST INCOMPLETE\n");
        }


        char serverRequest[MAX_OBJECT_SIZE];
        char space[2] = " ";
        char httpVers[12] = " HTTP/1.0\r\n";
        char hostLabel[7] = "Host: ";
        char lineEnding[3] = "\r\n";
        char colon[2] = ":";
        char connectionLine[20] = "Connection: close\r\n";
        char proxyReqEnd[28] = "Proxy-Connection: close\r\n\r\n";

        memset(serverRequest, 0, MAX_OBJECT_SIZE);

        strcat(serverRequest, method);
        strcat(serverRequest, space);
        strcat(serverRequest, path);
        strcat(serverRequest, httpVers);
        strcat(serverRequest, hostLabel);
        strcat(serverRequest, hostname);
        if (strcmp("80", port) ==0) {
                strcat(serverRequest, lineEnding);
        }
        else {
                strcat(serverRequest, colon);
                strcat(serverRequest, port);
                strcat(serverRequest, lineEnding);
        }
        strcat(serverRequest, user_agent_hdr);
        strcat(serverRequest, lineEnding);
        strcat(serverRequest, connectionLine);
        strcat(serverRequest, proxyReqEnd);

        printf("Sent Request:\n[%s]\n", serverRequest);

	// COMMUNICATE WITH HTTP SERVER
	struct addrinfo hints;
	struct addrinfo *result, *rp;
	int fd, s;

	memset(&hints, 0, sizeof(struct addrinfo));

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;

        /* getaddrinfo() returns a list of address structures.  However,
           because we have only specified a single address family (AF_INET or
           AF_INET6) and have only specified the wildcard IP address, there is
           no need to loop; we just grab the first item in the list. */
	s = getaddrinfo(hostname, port, &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		fd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (fd == -1)
			continue;
		if (connect(fd, rp->ai_addr, rp->ai_addrlen) != -1)
			break;
		close(fd);
	}
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
	}

	freeaddrinfo(result);


	if (write(fd, serverRequest, MAX_OBJECT_SIZE) == -1) {
		printf("Error with write\n");
		exit(EXIT_FAILURE);
	}

	memset(returnBuf, 0, MAX_OBJECT_SIZE);

	printf("Beginning to Read response...\n");

	totBytesRead = 0;
	cBytes = 1;

	while(cBytes != 0) {
                printf("looping\n");
		cBytes = read(fd, returnBuf + totBytesRead, MAX_OBJECT_SIZE);
		totBytesRead += cBytes;
                printf("after\n");
	}

        printf("Server Response\n");
	printf("[%s]/n", returnBuf);

	if (write(cfd, returnBuf, totBytesRead + 1) == -1) {
		printf("Error with write\n");
		exit(EXIT_FAILURE);
	}
	printf("Write sent\n");
        //memset(serverRequest, 0, MAX_OBJECT_SIZE);
        //return 1;
}

void test_parser() {
	int i;
	char method[16], hostname[64], port[8], path[64], headers[1024];

    char *reqs[] = {
		"GET http://www.example.com/index.html HTTP/1.0\r\n"
		"Host: www.example.com\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html?foo=1&bar=2 HTTP/1.0\r\n"
		"Host: www.example.com:8080\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://localhost:1234/home.html HTTP/1.0\r\n"
		"Host: localhost:1234\r\n"
		"User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:68.0) Gecko/20100101 Firefox/68.0\r\n"
		"Accept-Language: en-US,en;q=0.5\r\n\r\n",

		"GET http://www.example.com:8080/index.html HTTP/1.0\r\n",

		NULL
	};

	for (i = 0; reqs[i] != NULL; i++) {
		printf("Testing %s\n", reqs[i]);
		if (parse_request(reqs[i], method, hostname, port, path, headers)) {
			printf("METHOD: %s\n", method);
			printf("HOSTNAME: %s\n", hostname);
			printf("PORT: %s\n", port);
			printf("PATH: %s\n", path);
			printf("HEADERS: %s\n", headers);
		} else {
			printf("REQUEST INCOMPLETE\n");
		}
	}
}

void print_bytes(unsigned char *bytes, int byteslen) {
	int i, j, byteslen_adjusted;

	if (byteslen % 8) {
		byteslen_adjusted = ((byteslen / 8) + 1) * 8;
	} else {
		byteslen_adjusted = byteslen;
	}
	for (i = 0; i < byteslen_adjusted + 1; i++) {
		if (!(i % 8)) {
			if (i > 0) {
				for (j = i - 8; j < i; j++) {
					if (j >= byteslen_adjusted) {
						printf("  ");
					} else if (j >= byteslen) {
						printf("  ");
					} else if (bytes[j] >= '!' && bytes[j] <= '~') {
						printf(" %c", bytes[j]);
					} else {
						printf(" .");
					}
				}
			}
			if (i < byteslen_adjusted) {
				printf("\n%02X: ", i);
			}
		} else if (!(i % 4)) {
			printf(" ");
		}
		if (i >= byteslen_adjusted) {
			continue;
		} else if (i >= byteslen) {
			printf("   ");
		} else {
			printf("%02X ", bytes[i]);
		}
	}
	printf("\n");
}

int parse_request(char *request, char *method, char *hostname, char *port, char *path, char *headers) {
        if (all_headers_received(request) == 1) {
                // METHOD
                char *fromSpace = strchr(request, ' ');
                size_t lenOfMethod = fromSpace - request;
                memset(method, 0, 16);
                strncpy(method, request, lenOfMethod);

                // HOSTNAME AND PORT
                char *startHost = strstr(request, "Host: ") + 6;
                char *endHost;
                size_t lenHost;
                // check if a port is specified
                char *startPort = strchr(startHost, ':');
                char *nextLine = strstr(startHost, "\r\n");
                if (nextLine > startPort && startPort != NULL) {
                        char *endPort = strstr(startPort, "\n");
                        size_t lenPort = endPort - startPort;
                        memset(port, 0, 8);
                        strncpy(port, startPort + 1, lenPort);
                        port[strcspn(port, "\r\n")] = 0;
                        endHost = startPort;
                }
                else {
                        memset(port, 0, 8);
                        strcpy(port, "80");
                        endHost = strstr(startHost, "\r\n");
                }
                lenHost = endHost - startHost;
                memset(hostname, 0, 64);
                strncpy(hostname, startHost, lenHost);
                hostname[strcspn(hostname, "\r\n")] = 0;

                // PATH
                char *startPath = strstr(strstr(request, "http") + 9, "/");
                char *endPath = strchr(startPath, ' ');
                size_t lenPath = endPath - startPath;
                memset(path, 0, 64);
                strncpy(path, startPath, lenPath);
                path[strcspn(path, "\r\n")] = 0;

                // HEADERS
                char *startHeaders = strchr(request, '\n') + 1;
                memset(headers, 0, 1024);
                strcpy(headers, startHeaders);

                return 1;
        }
        else {
                return 0;
        }
}
