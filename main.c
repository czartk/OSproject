#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>

static const int g_ftok_shm_id = 100;
static const int g_ftok_sem_id = 200;

int main()
{
	pid_t pid1, pid2, pid3;
	
	// Przygotuj shared memory key
	key_t shm_key;
	if ((shm_key = ftok("./main", g_ftok_shm_id)) == -1) {
		printf("Nie mozna wygenerowac klucza pamieci dzielonej\n");
		exit(-1);
	}
	
	// Przygotuj semaphore key
	key_t sem_key;
	if ((sem_key = ftok("./main", g_ftok_sem_id)) == -1) {
		printf("Nie mozna wygenerowac klucza semaforu\n");
		exit(-1);
	}
	
	// Zamien na string aby przekazac do procesow potomnych
	char shm_key_str[12];
	char sem_key_str[12];
	sprintf(shm_key_str, "%d", shm_key);
	sprintf(sem_key_str, "%d", sem_key);
	
	pid1 = fork();
	switch(pid1) 
	{
		case 0: // Semafory i pamiec wspoldzielona test
			execl("./p1", "./p1", shm_key_str, sem_key_str, (char*)NULL); // Wykonywane w tym samym procesie potomnym
			exit(0);
		case -1:
			printf("Blad tworzenia procesu");
			exit(1);
		default:
			/*
			pid2 = fork();
			switch(pid2)
			{
				case 0:
					execl("./p2", "./p2", shm_key_str, sem_key_str, (char*)NULL); // Wykonywane w tym samym procesie potomnym
					exit(0);
				case -1:
					printf("Blad tworzenia procesu");
					exit(1);
				default:
					pid3 = fork();
					switch(pid3)
					{
						case 0:
						
							exit(0);
						case -1:
							printf("Blad tworzenia procesu");
							exit(1);
						default:
							
							wait(NULL);
							break;
					}
					
				wait(NULL);
				break;
			}
			*/
			wait(NULL);
			break;
	}
	
	return 0;
}
