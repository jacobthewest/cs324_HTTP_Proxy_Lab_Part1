#include<stdio.h>
#include<stdlib.h>

int main() {
	printf("Content-type: text/plain\r\n\r\nhello world!%s\n", getenv("QUERY_STRING"));
}
