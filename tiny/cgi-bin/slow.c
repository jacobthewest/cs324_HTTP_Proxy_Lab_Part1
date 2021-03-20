#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void) {
	char *name;
	char *value;
	char *next_pair;
	int i, j;
	int remaining;
	int chunk;
	int size = 0;
	int sleep_time = 0;
	int chunk_size = 0;
	char *buf;

	name = getenv("QUERY_STRING");
	if (name != NULL) { 
		do {
			next_pair = strchr(name, '&');
			if (next_pair != NULL) {
				*next_pair = '\0';
				next_pair++;
			}
			value = strchr(name, '=');
			if (value != NULL) {
				*value = '\0';
				value++;
				if (strcmp(name, "size") == 0) {
					size = atoi(value);
				} else if (strcmp(name, "sleep") == 0) {
					sleep_time = atoi(value);
				} else if (strcmp(name, "chunksize") == 0) {
					chunk_size = atoi(value);
				}
			}
			name = next_pair;
		} while (name != NULL);
	}

	if (chunk_size <= 1) {
		chunk_size = size/2;
	}

	buf = malloc(sizeof(char) * (size + 1));

	/* Generate the HTTP response */
	printf("Connection: close\r\n");
	printf("Content-length: %d\r\n", size);
	printf("Content-type: text/plain\r\n\r\n");
	fflush(stdout);
	i = 0;
	while (i < size) {
		sleep(sleep_time);
		chunk = i + chunk_size;
		if (chunk > size) {
			chunk = size;
		}
		for (j = 0; i < chunk; i++, j++) {
			buf[j] = '0' + (i % 10);
			if (buf[j] == '0') {
				buf[j] = '\n';
			}
		}
		buf[j] = '\0';
		printf("%s", buf);
		fflush(stdout);
	}
}
