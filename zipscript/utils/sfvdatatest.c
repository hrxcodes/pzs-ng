#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <limits.h>

#include "race-file.h"

int main(int argc, char **argv)
{
	int release_type = 0, version = 0;
	SFVDATA sd;
	FILE *f;
	
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	if (!(f = fopen(argv[1], "r"))) {
		perror("fopen");
		return EXIT_FAILURE;
	}

	fread(&version, sizeof(short int), 1, f);
	fread(&release_type, sizeof(short int), 1, f);
	printf("Release type: %i\n", release_type);
	printf("File version: %i\n\n", version);

	while ((fread(&sd, sizeof(SFVDATA), 1, f))) {
		printf("%s %.8x\n", sd.fname, sd.crc32);
	}

	fclose(f);

	return EXIT_SUCCESS;
}
