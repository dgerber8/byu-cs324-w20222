/*
PART 1: 
What are the numbers associated with the manual sections for executable programs, system calls, and library calls, respectively?
	1,2,3

Which section number(s) contain a man page for open?
	2

What three #include lines should be included to use the open() system call?
	<sys/types.h>, <sys/stat.h>, <fcntl.h>

Which section number(s) contain a man page for socket?
	2

Which socket option "Returns a value indicating whether or not this socket has been marked to accept connections with listen(2)"?
	SO_ACCEPTCONN

How many man pages (in any section) contain keyword references to getaddrinfo?
	There are 3:
	gai.conf (5)         - getaddrinfo(3) configuration file
	getaddrinfo (3)      - network address and service translation
	getaddrinfo_a (3)    - asynchronous network address and service translation

According to the "DESCRIPTION" section of the man page for string, the functions described in that man page are used to perform operations on strings that are ________________. (fill in the blank)
	null-terminated

What is the return value of strcmp() if the value of its first argument (i.e., s1) is greater than the value of its second argument (i.e., s2)?
	An integer greater than zero
 
PART 2:
I Completed the TMUX exercise from part 2
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
int main(int argc, char *argv[]) {
	fprintf(stderr, "PID: %ld\n\n", (long)getpid());
	
	FILE *fileptr;
	fileptr = fopen(argv[1],"r");
	char *cmPattern;
	char nextLine[256];
	while(fgets(nextLine, sizeof(nextLine), fileptr)) {
		int patFound = 0;
		if(getenv("CATMATCH_PATTERN")) {
			cmPattern = getenv("CATMATCH_PATTERN");
			char* p;
			p = strstr(nextLine, cmPattern);
			//fprintf(stdout, "%s  %s  %s\n", cmPattern, nextLine, p);
			if(p) {
				patFound = 1;
				fprintf(stdout, "1 %s\n", nextLine);
			}
		}
		if (patFound == 0) {
			fprintf(stdout, "0 %s\n", nextLine);
		}
	}	 
	fclose(fileptr);	
	return 0;
}
