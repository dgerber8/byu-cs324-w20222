#include <sys/types.h>
#include <sys/wait.h>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>

int main(int argc, char *argv[]) {
	int pid;
	int p[2];
	FILE *fp;
	FILE *wStr;
	FILE *rStr;

	if (pipe(p) < 0) {
		exit(1);
	}

	printf("Starting program; process has pid %d\n", getpid());

	fp = fopen("fork-output.txt","w");
	fprintf(fp,"BEFORE FORK\n");
	fflush(fp);
	if ((pid = fork()) < 0) {
		fprintf(stderr, "Could not fork()");
		exit(1);
	}

	/* BEGIN SECTION A */

	printf("Section A;  pid %d\n", getpid());
	fprintf(fp,"SECTION A\n");
	fflush(fp);
	//sleep(30);

	/* END SECTION A */
	if (pid == 0) {
		/* BEGIN SECTION B */

		printf("Section B\n");
		fprintf(fp,"SECTION B\n");
		fflush(fp);
		close(p[0]);
		wStr = fdopen(p[1], "w");
		fputs("Hello from section B\n", wStr);
		//sleep(30);
		//sleep(30);
		//printf("Section B done sleeping\n");
		char *newenviron[] = { NULL };

        	printf("Program \"%s\" has pid %d. Sleeping.\n", argv[0], getpid());
        	sleep(30);

		if (argc <= 1) {
			printf("No program to exec.  Exiting...\n");
			exit(0);
 		}

		printf("Running exec of \"%s\"\n", argv[1]);
		dup2(fileno(fp), 1);
		execve(argv[1], &argv[1], newenviron);
		printf("End of program \"%s\".\n", argv[0]);
		exit(0);

		/* END SECTION B */
	} else {
		/* BEGIN SECTION C */

		wait(NULL);
		printf("Section C\n");
		fprintf(fp,"SECTION C\n");
		fflush(fp);
		close(p[1]);
		rStr = fdopen(p[0], "r");
		char myStr[255];
		//printf("4\n");
		if (fgets(myStr, 255, rStr) != NULL) {
			//printf("5\n");
			puts(myStr);
			//printf("6\n");
		}
		//sleep(30);
		//printf("Section C done sleeping\n");

		exit(0);

		/* END SECTION C */
	}
	/* BEGIN SECTION D */

	printf("Section D\n");
	fprintf(fp,"SECTION D\n");
	fflush(fp);
	//sleep(30);

	/* END SECTION D */
}


