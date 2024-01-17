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

static const int g_max_read_characters = 12288; // 12 kB

// Struct sembuf
// unsigned short sem_num;  /* semaphore number */
// short          sem_op;   /* semaphore operation */
// short          sem_flg;  /* operation flags */

union senum
{
	int val;
	struct semid_ds *buf;
	unsigned short* array;
};

/*
int read(key_t shm_key, key_t sem_key)
{
	// Semafor ustawiony na 1 puszcza program, program dziala; ustawiony 0 zatrzymuje program
	// struct sembuf sem_op_0 = { 1, -1, 0 };
	// semop(semid, &sem_op_0, 1); // Semop powoduje czekanie, ktore zalezy od wartosci semafora
	// drugi parametr moze byc arrayem sembufow (polecen), trzeci parametr okresla ilosc polecen do zczytania
	// u nas sembuf jest tylko jeden wiec size=1
	int semid = semget(sem_key, 2, 0666 | IPC_CREAT);
	union senum arg;
	arg.val = 1;
	semctl(semid, 0, SETVAL, arg);
	// Semid - ktroy zestaw semaforow, 0 - ktory semafor (liczone od 0), SETVAL - flaga na ustawienie wartosci, arg.val - wartosc ktora ustawic

	char* str;
	
	// Tworze pamiec dzielona na 1024 znakow; dlatego, ze key=1 dla kazdego z procesow, shmid zawsze points
	// Do tego samego miejsca
	int shmid = shmget(shm_key, 1024, 0666 | IPC_CREAT);
	// Zwraca adres do pamieci wspolnej; Przydziela pamiec do procesu
	shmStr = shmat(shmid, 0, 0);
	
	//while(1)
	//{
		printf("Podaj tekst:\n> ");
		scanf("%s", str);
	//}
	
	// Oddziela pamiec od procesu (zeby ja zniszczyc)
	shmdt(str);
	// Zniszczenie pamieci
	shmctl(shmid, IPC_RMID, NULL);
	
	return 0;
}
*/

int main(int argc, char *argv[])
{
	// argv[0] - filepath
	// argv[1] - shmkey
	// argv[2] - semkey
	if(argc != 3)
		return -1;
	
	key_t shm_key = atoi(argv[1]);
	key_t sem_key = atoi(argv[2]);
	
	bool isReadFile;
	
	int shmid = shmget(shm_key, g_max_read_characters, 0666 | IPC_CREAT); // Pamiec dzielona na 1024 znakow
	char* shm_str = shmat(shmid, 0, 0); //shm_str mozna traktowac jako char shm_str[1024]
	
	while(1)
	{
		do{
			int choice;
			printf("Skad chcesz zczytywac dane? (0) Z klawiatury (1) Z pliku\n> ");
			if (!scanf("%d", &choice) || (choice != 0 && choice != 1)) // Scanf zwraca 0 gdy wystapi blad w czytaniu
			{
				printf("Wprowadzono zla wartosc. Wprowadz jeszcze raz\n");
				continue;
			}
			
			isReadFile = choice;
		}
		while(0);
		
		if (!isReadFile) { // Z klawiatury
			printf("Wprowadz dane: (Maximum 12287 znaki)\n> ");
			while ((getchar()) != '\n');
			fgets(shm_str, g_max_read_characters, stdin);
		}
		else
		{
			printf("Umiesc plik w folderze z programami. Podaj nazwe pliku (max 96 znakow) wraz z rozszerzeniem: (Maximum 12287 znakow w pliku)\n> ");
			char filepath[100] = "./";
			char filename[97];
			while ((getchar()) != '\n'); // Wyczyszczenie stdin
			fgets(filename, 97, stdin);
			strncat(filepath, filename, 97);
			
			FILE* fptr;
			char ch[1];
			fptr = fopen(filepath, "r");
			
			int iii;
			for (iii=0; iii<g_max_read_characters && !feof(fptr); ++iii)
			{
				ch[0] = fgetc(fptr);
				memcpy(shm_str, ch, 1);
			}
 
			fclose(fptr);
		}
		
		// Oddziela pamiec od procesu (zeby ja zniszczyc)
		shmdt(shm_str);
		// Zniszczenie pamieci
		shmctl(shmid, IPC_RMID, NULL);
	}

	return 0;
}
