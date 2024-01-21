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
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

static const int g_max_read_characters = 12288;

// Tutaj jest zczytywana ilosc znakow na pobrana linie tekstu
int main(int argc, char *argv[])
{
	// argv[0] - filepath
	// argv[1] - shmkey
	// argv[2] - sem_producer_name
	// argv[3] - sem_consumer_name
	// argv[4] - pipe_write_str
	if(argc != 5)
		return -1;
	
	const key_t shm_key = atoi(argv[1]);
	char* sem_producer_name = argv[2];
	char* sem_consumer_name = argv[3];
	const int pipe_write = atoi(argv[4]);
	
	// Semafory sa juz stworzone w P1, tutaj tylko uzyskujemy do nich dostep
	//							nazwa           , flaga
	sem_t* sem_prod = sem_open(sem_producer_name, 0);
	if (sem_prod == SEM_FAILED)
	{
		printf("Nie mozna otworzyc semafora producenta\n");
		exit(-1);
	}
	sem_t* sem_cons = sem_open(sem_consumer_name, 0);
	if (sem_cons == SEM_FAILED)
	{
		printf("Nie mozna otworzyc semafora konsumenta\n");
		exit(-1);
	}
	
	char buffer[12288];
	
	while(1)
	{
		sem_wait(sem_cons);
		
		int shmid = shmget(shm_key, g_max_read_characters, 0666 | IPC_CREAT); // Pamiec dzielona na 12288 znakow
		char* shm_str = shmat(shmid, 0, 0); //shm_str mozna traktowac jako char shm_str[12288]
		
		// Pobierz linie
		while(fgets(buffer, g_max_read_characters, shm_str)) // Jezeli fgets natrafi na EOF to zwraca 0; Zczytuje do \n lub EOF
		{
			// Zlicz ilosc znakow w lini i wyslij do procesu 3
			int cCount=0;
			int iii;
			for(iii=0; iii < g_max_read_characters; ++iii)
			{
				if (buffer[iii] == '\0')
					break;
				else
					cCount++;
			}
			
			// wysylanie pipem (Konswersja do string)
			char countStr[11];
			sprintf(countStr, "%d", cCount);
			if(write(pipe_write, countStr, strlen(countStr)+1) == -1)
			{
				printf("Blad w zapisywaniu do pipe'a\n");
				exit(-1);
			}
		}
		
		// Oddziela pamiec od procesu (zeby ja zniszczyc)
		shmdt(shm_str);
		// Zniszczenie pamieci
		shmctl(shmid, IPC_RMID, NULL);
		
		sem_post(sem_prod);
	}
	
	sem_close(sem_prod);
	sem_close(sem_cons);

	return 0;
}
