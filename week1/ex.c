
#include "stack.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include<sys/wait.h>


int main() {
	int fd[2];
    char buffer[80] = "hello world\n";

    pipe(fd);

    pid_t pid = fork();

	if (pid > 0) {
		// parent-client
		printf("Enter command that will be executed with stack DS. Possible commands are: \n1)pushData (e.g. push100 will push integer 100) \n2)pop \n3)peek \n4)display \n5)create \n6)empty \n7)stack_size \n8)create\n");

		// reading command 
		while (1) {
			scanf("%s", buffer);
			close(fd[0]);
        	write(fd[1], &buffer, (sizeof(buffer)));
        	printf("\n");
		}
		
		
        close(fd[1]);



	}
	else {
		// child-server

		while (1) {
	        close(fd[1]);

	        // reading command from the pipe
	        read(fd[0], &buffer, sizeof(buffer));

	        // execute commands on server side and print the result

	        if (strncmp(buffer, "push", 4) == 0) {
	        	char *digitData = buffer + 4;
	        	int data = atoi(digitData);
	        	push(data);
	        	printf("\n");
	        }

	        if (strcmp(buffer, "display") == 0) {
	        	display();
	        }

	        else if (strcmp(buffer, "pop") == 0) {
	        	pop();
	        	printf("\n");
	        }

	        else if (strcmp(buffer, "peek") == 0) {
	        	printf("%d\n", peek());
	        }

	        else if (strcmp(buffer, "empty") == 0) {
	        	printf("%d\n", empty());
	        }

	        else if (strcmp(buffer, "stack_size") == 0) {
	        	stack_size();
	        }

	        else if (strcmp(buffer, "create") == 0) {
	        	create();
	        	printf("\n");
	        }
    	}
        close(fd[0]);
	
	}
	return 0;
}


