// Replace PUT_USERID_HERE with your actual BYU CS user id, which you can find
// by running `id -u` on a CS lab machine.
#define USERID 1823605493 // Will's
//#define USERID 1823605493
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

int verbose = 0;
int v = 1;

unsigned char scripture[500];
int scriptIndex = 0;

int n;
int opCode;
int opParam1;
int opParam2;
int nonce[4];
int prevLocPort;

void print_bytes(unsigned char *bytes, int byteslen);

void print_scripture() {
	printf("%s\n", scripture);
}

// This function updates the Op Code, Op Params, Message, and Nonce based on the passed in response buffer
void set_values(unsigned char *bytes) {
	n = bytes[0];
	opCode = bytes[n+1];
	//printf("~~~~~SET (%d)~~~~~", opCode);
	// If there wasn't an error (aka Byte 0 > 21 (127) from specs under response diagram), add the chunk to the chunks already recieved
	if (n < 21 && n != 0) {
		for (int i = 1; i <= n; i++) {
			scripture[scriptIndex] = bytes[i];
			scriptIndex = scriptIndex + 1;
		}
	}

	// Update Op Params (Op code 1: these make new port to send to)
	opParam1 = bytes[n+2];
	opParam2 = bytes[n+3];

	// Grab and store eache byte of the nonce in a list (List started from scratch each time)
	for (int j = 0; j < 4; j++) {
		nonce[j] = bytes[n+4+j];
	}
}

// This funtion prints the len of the recently added chunk, updated message, Op Code, Op Params, and Nonce
void print_info() {
	printf("Chunk Length: %d\n", n);
	printf("The current scripture: \n");
	print_scripture();
	printf("Op Code: %d\n", opCode);
	printf("Op Param: %u, %u\n", opParam1, opParam2);
	printf("Nonce: ");
	for (int j = 0; j < 4; j++) {
		printf("%d, ", nonce[j]);
	}
	printf("\n");
}

//unsigned int PortLitToBigEndian(unsigned int x) {
//	return (((x>>8) & 0x00ff) | ((x<<8) & 0xff00));
//}

int main(int argc, char *argv[]) {
	struct sockaddr_in ipv4addr_remote;
	//socklen_t len;
	struct addrinfo hints;
	struct addrinfo *result;
	struct addrinfo *rp;
	int sfd;
	//char *inpAddr = argv[0];
	//char *inpPort = argv[1];

	//print_ip();

	// INITIALIZE ADDRINFO STUFF
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	int s = getaddrinfo(argv[1], argv[2], &hints, &result);
	if (s != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(s));
		exit(EXIT_FAILURE);
	}
	struct sockaddr_in my_addr;
	char myIP[16];

	struct sockaddr_in firstAddr;
	firstAddr.sin_family = AF_INET;
	firstAddr.sin_addr.s_addr = 0;
	firstAddr.sin_port = 0;
	// LOOP THROUGH EACH RESULT OBJECT (there are multiple responses b/c some sites have multiple IP addresses)
	for (rp = result; rp != NULL; rp = rp->ai_next) {
		// CREATE SOCKET
		sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		// if error, just move on to the next address result
		if (sfd == -1) {
			continue;
		}
		if (bind(sfd, (struct sockaddr *)&firstAddr, sizeof(firstAddr)) < 0) {
			perror("bind failed");
			exit(EXIT_FAILURE);
                }
		// ESTABLISH CONNECTION THEN BREAK IF SUCCESSFUL(-1 means error)
		if (connect(sfd, rp->ai_addr, rp->ai_addrlen) != -1) {
			ipv4addr_remote = *(struct sockaddr_in *)rp->ai_addr;

			// Store Selected Port
			bzero(&my_addr, sizeof(my_addr));
			socklen_t len = sizeof(my_addr);
			getsockname(sfd, (struct sockaddr *) &my_addr, &len);
			inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
			prevLocPort = ntohs(my_addr.sin_port);
			if (v == 1) {
				printf("!!!(PORT)!!!Initial Port: %x\n", prevLocPort);
			}
			break;
		}
		// CLOSE SOCKET ON ERROR
		close(sfd);
	}

	// return error if no results found with that address
	if (rp == NULL) {
		fprintf(stderr, "Could not connect\n");
		exit(EXIT_FAILURE);
	}

	freeaddrinfo(result);

	// SEND MESSAGE AND RECEIVE RESPONSE
	unsigned char buf[8] = { 0, (uintptr_t)atoi(argv[3]), 0x6c, 0xb3, 0x59, 0x29, (uintptr_t)(atoi(argv[4]) >> (8*1)) & 0xff, (uintptr_t)(atoi(argv[4]) >> (8*0)) & 0xff }; // Will's
	//unsigned char buf[8] = { 0, (uintptr_t)atoi(argv[3]), 0x6c, 0xb2, 0x02, 0xf5, (uintptr_t)(atoi(argv[4]) >> (8*1)) & 0xff, (uintptr_t)(atoi(argv[4]) >> (8*0)) & 0xff };
	unsigned char returnBuf[64];
	unsigned char requestBuf[8];
	// send buffer of len 8 bytes
	send(sfd, &buf, 8, 0);
	// RECEIVE 64 BYTES AND POPULATE IN returnBuf (if -1 returned when recv preformed, read error)
	recv(sfd, returnBuf, 64, 0);

	// PRINT RESPONSE MESSAGE
	if (v == 1) {
		printf("Original Return Buffer: \n");
		print_bytes(returnBuf, n+8);
		printf("\n");
	}

	int numComms = 1;
	while(1) {
		set_values(returnBuf);
		//printf("WAHOO");
		if (n == 0 || n > 127) {
			break;
		}

		// OPCODE 0
		if (opCode == 0) {
			if (v == 1) { printf("~~~~~~~~~~~~ROUND %d (OpCode 0)~~~~~~~~~~~~~\n", numComms);}
			numComms = numComms + 1;
			returnBuf[n + 7] = returnBuf[n + 7] + 1;
			for (int j = 0; j < 4; j++) {
				requestBuf[j] = returnBuf[n + 4 + j];
			}
			send(sfd, &requestBuf, 4, 0);
			recv(sfd, returnBuf, 64, 0);
			//set_values(returnBuf);
		}

		// OPCODE 1
		else if (opCode == 1) {
			int port;
			//socklen_t len;
			int firstOpOne = 0;
			while (1) {
				if (v == 1) { printf("~~~~~~~~~~~~ROUND %d (OpCode 1)~~~~~~~~~~~~~\n", numComms);}
				numComms = numComms + 1;

				// UPDATE NONCE, OP PARAMS, AND MESSAGE(add new chunk) FROM PREVIOUS RESPONSE BUFFER (if they havent already from the outer while)
				if (firstOpOne == 1) {
					set_values(returnBuf);
				}
				else {
					firstOpOne = 1;
				}
				if (n == 0 || opCode != 1) {
					break;
				}
				// Print Updated Info
				if (v == 1) {
					printf("Printing Info...\n");
					print_info();
					printf("\n");
				}

				// ADD 1 TO NONCE AND STICK IT IN THE FRONT OF YOUR REQUEST BUFFER (index 0-4)
				returnBuf[n + 7] = returnBuf[n + 7] + 1;
				for (int j = 0; j < 4; j++) {
					requestBuf[j] = returnBuf[n + 4 + j];
				}

				// SET PORT TO NUMBER GIVEN IN UPDATED OP PARAMS (from the response buffer (updated in set_values() call))
				port = (opParam1 << 8) | (opParam2);
				if (v == 1) { printf("Port Number(hex, decimal): %x, %d\n",port,port);}
				ipv4addr_remote.sin_port = htons(port);

				// INITIATE CONNECTION
				int connectRes = connect(sfd, (struct sockaddr *)&ipv4addr_remote, sizeof(struct sockaddr_in));
				if (v == 1) { printf("Connect Returned: %d\n", connectRes);}

				// print request buffer about to be sent
				if (v == 1) {
					printf("OpCode 1 Request Buffer on send(): ");
					print_bytes(requestBuf, 8);
				}

				// SEND REQUEST BUFFER TO NEW PORT
				int sendResult = send(sfd, &requestBuf, 4, 0);
				if (sendResult != 4) {
					fprintf(stderr, "send error\n");
					exit(EXIT_FAILURE);
				}

				if (v == 1) {
					printf("OpCode 1 Return Buffer before recv(): ");
					print_bytes(returnBuf, n+8);
				}

				// receives from that port
				if (recv(sfd, returnBuf, 64, 0) == -1) {
					perror("read");
					exit(EXIT_FAILURE);
				}

				if (v == 1) {
					printf("OpCode 1 Return Buffer after recv(): ");
					print_bytes(returnBuf, n+8);
					printf("\n");
				}
				// Check if you should stop looping here
				if (returnBuf[0] == 0 || returnBuf[returnBuf[0] + 1] != 1) {
					break;
				}
			}
		}

		// OPCODE 2 (BIND)
		//
		// Use bind() to change the client's source port and create a new socket with that port set for the server's destination port.
		// the port number is defined by the op params just like in Opcode 1
		else if (opCode == 2) {
			int clientPort;
			//socklen_t len;
			int firstOpTwo = 0;
			while (1) {
				if (v == 1) { printf("~~~~~~~~~~~~ROUND %d (OpCode 2)~~~~~~~~~~~~~\n", numComms);}
				numComms = numComms + 1;

				// UPDATE NONCE, OP PARAMS, AND MESSAGE(add new chunk) FROM PREVIOUS RESPONSE BUFFER (if they havent already from the outer while)
				if (firstOpTwo == 1) {
					set_values(returnBuf);
				}
				else {
					firstOpTwo = 1;
                                }
                                if (n == 0 || opCode != 2) {
                                        break;
				}
				// Print Updated Info
				if (v == 1) {
					printf("Printing Info...\n");
					print_info();
					printf("\n");
				}
				// ADD 1 TO NONCE AND STICK IT IN THE FRONT OF YOUR REQUEST BUFFER (index 0-4)
				returnBuf[n + 7] = returnBuf[n + 7] + 1;
				for (int j = 0; j < 4; j++) {
					requestBuf[j] = returnBuf[n + 4 + j];
				}

				// SET PORT TO NUMBER GIVEN IN UPDATED OP PARAMS (from the response buffer (updated in set_values() call))
				clientPort = (opParam1 << 8) | (opParam2);
				clientPort = htons(clientPort);
				if (v == 1) { printf("Port Number(hex, decimal): %x, %d\n",clientPort,clientPort);}

				// CLOSE OLD SOCKET
				close(sfd);

				// MAKE A NEW SOCKET AND USE BIND() TO SET SOURCE PORT TO THE NEW VALUE
				struct sockaddr_in clientAddr;
				clientAddr.sin_family = AF_INET;
				clientAddr.sin_addr.s_addr = 0;
				clientAddr.sin_port = clientPort;


				if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) < -1) {
					perror("socket()");
				}

				if (bind(sfd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
                                        perror("bind failed");
                                        exit(EXIT_FAILURE);
                                }

				if (connect(sfd, (struct sockaddr *)&ipv4addr_remote, sizeof(struct sockaddr_in)) == -1) {
                                        perror("connect failed");
                                }

				// Store Selected Port
				bzero(&my_addr, sizeof(my_addr));
				socklen_t len = sizeof(my_addr);
				getsockname(sfd, (struct sockaddr *) &my_addr, &len);
				inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
				prevLocPort = ntohs(my_addr.sin_port);
				if (v == 1) {
					printf("!!!(PORT)!!!(OpCode 2) New Port: %x (%x)\n", prevLocPort, clientPort);
				}
				// SEND
				int sendResult = send(sfd, &requestBuf, 4, 0);
                                if (sendResult != 4) {
                                        fprintf(stderr, "send error\n");
                                        exit(EXIT_FAILURE);
                                }

                                if (v == 1) {
                                        printf("OpCode 2 Return Buffer before recv(): \n");
                                        print_bytes(returnBuf, n+8);
                                }

                                // RECEIVE
                                if (recv(sfd, returnBuf, 64, 0) == -1) {
                                        perror("read");
					printf("\nEreerno: %d\n", errno);
                                        exit(EXIT_FAILURE);
                                }

                                if (v == 1) {
                                        printf("OpCode 2 Return Buffer after recv(): ");
                                        print_bytes(returnBuf, n+8);
                                        printf("\n");
                                }
				if (returnBuf[0] == 0 || returnBuf[returnBuf[0] + 1] != 2) {
                                        break;
                                }
			}
		}

		// OPCODE 3 (READ)
		//
		// Read m datagrams from the socket where m = the previous nonce (n + 2, n + 3). Rule: 1 < m < 7. Each datagram comes from a randomly selected remote port (use recvfrom())
		// Each datagram has 0 length with irrelevant contents. To get the nonce, add all remote ports of these datagrams. Will need to be stored in unsigned int instead of unsigned
		// short b/c the total may exceed 16 bits.
		// Ports:
		// To be able to accept these datagrams coming from any port, we need to create a new socket each time this opCode is called and set its local port to the local port
		// of the previous socket. The previous port is stored as PrevLocPort(may need to htons it)
		else if (opCode == 3) {
			if (v == 1) { 
				printf("~~~~~~~~~~~~ROUND %d (OpCode 3)~~~~~~~~~~~~~\n", numComms);
				numComms = numComms + 1;
			}
			if (v == 1) {
                                printf("Printing Info...\n");
                                print_info();
                                printf("\n");
                        }
			if (v == 1) {printf("m = %d (%x(OpParam1) + %x(OpParam2)\n", opParam1 + opParam2, opParam1, opParam2);}

			// SET m AND INITIALIZE remPortSum
			unsigned int remPortSum = 0;
			int m = opParam1 + opParam2;
			// CALL CLOSE ON PREVIOUS SOCKET
			close(sfd);
			// CREATE NEW SOCKET
                        if ((sfd = socket(AF_INET, SOCK_DGRAM, 0)) < -1) {
                                perror("socket()");
                        }

			// BIND NEW SOCKET TO PrevLocPort (htons())
			struct sockaddr_in clientAddr;
                        clientAddr.sin_family = AF_INET;
                        clientAddr.sin_addr.s_addr = 0;
                        clientAddr.sin_port = htons(prevLocPort);
                        if (bind(sfd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
                                perror("bind failed");
                                exit(EXIT_FAILURE);
                        }

			// CALL recvfrom() m TIMES
			socklen_t lenn = sizeof(struct sockaddr_in);
			struct sockaddr_in ipv4addr_remote_portless; //= ipv4addr_remote;
			//ipv4addr_remote_portless.sin_port = 0;
			for (int k = 0; k < m; k++) {
				ipv4addr_remote_portless = ipv4addr_remote;
                        	ipv4addr_remote_portless.sin_port = 0;
				recvfrom(sfd, returnBuf, 64, 0, (struct sockaddr *) &ipv4addr_remote_portless, &lenn);
				int port = ntohs(ipv4addr_remote_portless.sin_port);
				// COMPUTE NONCE BY ADDING PORTS (unsigned int)
				remPortSum = remPortSum + port;
				if (v == 1){printf("(%d)p = %d(%x)  Total = %d(%x)\n", k , port, port, remPortSum, remPortSum);}
			}
			remPortSum = remPortSum + 1;

			unsigned char bytes[4];
			bytes[0] = (remPortSum >> 24) & 0xFF;
			bytes[1] = (remPortSum >> 16) & 0xFF;
			bytes[2] = (remPortSum >> 8) & 0xFF;
			bytes[3] = remPortSum & 0xFF;
			if (v == 1) {printf("Bytes(after +1): %x %x %x %x (%x)\n", bytes[0], bytes[1], bytes[2], bytes[3], remPortSum);}

			//char p1 = remPortSum & 0x000000ff;
			//returnBuf[n + 7] = returnBuf[n + 7] + 1;
			for (int j = 0; j < 4; j++) {
				requestBuf[j] = bytes[j];
			}
			// CALL CONNECT ON THE NEW SOCKET TO SET REMOTE ADDR AND PORT
                        if (connect(sfd, (struct sockaddr *)&ipv4addr_remote, sizeof(struct sockaddr_in)) == -1) {
                                perror("connect failed");
                        }

                        // Store Selected Port
                        bzero(&my_addr, sizeof(my_addr));
                        lenn = sizeof(my_addr);
                        getsockname(sfd, (struct sockaddr *) &my_addr, &lenn);
                        inet_ntop(AF_INET, &my_addr.sin_addr, myIP, sizeof(myIP));
                        prevLocPort = ntohs(my_addr.sin_port);
                        if (v == 1) {
                                printf("(OpCode 3) New Port: %x\n", prevLocPort);
                        }

			// SEND REQUEST WITH NONCE + 1
			int sendResult = send(sfd, &requestBuf, 4, 0);
                        if (sendResult != 4) {
                                fprintf(stderr, "send error\n");
                                exit(EXIT_FAILURE);
                        }

                        if (v == 1) {
                                printf("OpCode 3 Return Buffer before recv(): \n");
                                print_bytes(returnBuf, n+8);
                        }

                        // RECEIVE
                        if (recv(sfd, returnBuf, 64, 0) == -1) {
                                perror("read");
                                printf("\nEreerno: %d\n", errno);
                                exit(EXIT_FAILURE);
                        }

                        if (v == 1) {
                                printf("OpCode 3 Return Buffer after recv(): ");
                                print_bytes(returnBuf, n+8);
                                printf("\n");
                        }
			//print_scripture();
			//exit(EXIT_SUCCESS);
		}

		//OPCODE 4
		else if (opCode == 4) {

		}
	}

	print_scripture();

	//printf(returnBuf);
	//print_bytes(returnBuf, returnBuf[0]+8);

	exit(EXIT_SUCCESS);
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
