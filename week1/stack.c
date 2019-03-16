
#include <stdio.h>
#include "stack.h"

int stack[16];
int head = -1;


int empty() {
	if (head == -1) return 1;
	else return 0;
}


int peek() {
	return stack[head];
}

void push(int data) {
	stack[++head] = data;
}

void pop() {
	if (!empty()) {
		head --;
	}
}



void display() {
	if (!empty()) {
		printf("[");
		for (int i=0; i<=head; i++) {
			printf("%d,", stack[i]);
		}
		printf("]");
	}
	else {
		printf ("[]");
	}
	printf("\n");
}

void create() {
	head = -1;
}

void stack_size() {
	printf("%d\n", head+1);
}
