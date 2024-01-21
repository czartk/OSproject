#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <string.h>

// Tutaj jest printowana liczba znakow w linijce
int main(int argc, char *argv[])
{
	// argv[0] - filepath
	// argv[1] - pipe_read_str
	if(argc != 2)
		return -1;
	
	const int pipe_read = atoi(argv[1]);
	int line=1;
	
	while(1)
	{
		int iii;
		char countStr[11];
		// Wczytaj string
		for (iii=0; read(pipe_read, countStr, 1) != -1; ++iii)
		{
			if (countStr[iii] == '\0')
				break;
		}
		//printf("Linia %d ma %s znakow.\n", line, countStr);
		line++;
	}

	return 0;
}
