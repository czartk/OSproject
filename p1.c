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
#include <stdbool.h>

static const int g_max_read_characters = 12288; // 12 kB
bool work = 0;
union senum {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void usr1Handler() {
    //printf("Odbieram w P1\n");
    kill(getpid() + 1, SIGUSR1);
    int sig;
    //Odczytanie z kolejki fifo
    char* fifo = "fifo_1";
    mkfifo(fifo, 0666);
    int fd = open(fifo, O_RDONLY);
    read(fd, &sig, sizeof(int));
    close(fd);

    if (sig == SIGTSTP) {
        work = 1;
    }
    else if (sig == SIGCONT) {
        work = 0;
    }
    else if (sig == SIGTERM) {
        exit(0);
    }
}


int main(int argc, char *argv[])
{
    // Ustawienie obslugi sygnalow
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGUSR1, usr1Handler);

    // argv[0] - filepath
    // argv[1] - shmkey
    // argv[2] - sem_producer_name
    // argv[3] - sem_consumer_name
    if(argc != 2)
        return -1;

    const key_t shm_key = atoi(argv[1]);

    //Tworzenie zestawu semaforow
    const key_t sem_key = ftok("sem", 1);
    int semid = semget(sem_key, 2, 0666 | IPC_CREAT);
    union senum arg;
    arg.val = 0;
    semctl(semid, 0, SETVAL, 0);
    // char* sem_producer_name = argv[2];
    // char* sem_consumer_name = argv[3];

    bool isReadFile;

    // // Inicjalizacja Semaforow dla p1 p2
    // // Jezeli z jakiegos powodu sa juz takie semafory to je zamknij
    // sem_unlink(sem_producer_name);
    // sem_unlink(sem_consumer_name);

    // // Jezeli zawolamy wait przy wartosci semafora 0, to semafor czeka
    // // Przy wyzszych musi zejsc do 0
    // //							nazwa			, flaga	   ,dostep, wartosc
    // sem_t* sem_prod = sem_open(sem_producer_name, IPC_CREAT, 0666, 0);
    // if (sem_prod == SEM_FAILED)
    // {
    // 	printf("Nie mozna otworzyc semafora producenta\n");
    // 	exit(-1);
    // }
    // sem_t* sem_cons = sem_open(sem_consumer_name, IPC_CREAT, 0666, 0);
    // if (sem_cons == SEM_FAILED)
    // {
    // 	printf("Nie mozna otworzyc semafora konsumenta\n");
    // 	exit(-1);
    // }


    while(1)
    {
        int shmid = shmget(shm_key, g_max_read_characters, 0666 | IPC_CREAT); // Pamiec dzielona na 12288 znakow
        char* shm_str = shmat(shmid, 0, 0); //shm_str mozna traktowac jako char shm_str[12288]

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
                memcpy(shm_str, ch, 1); // Jezeli to nie dziala strncpy powinno
            }

            fclose(fptr);
        }

        // Podniesienie semafora w P2
        arg.val = 1;
        semctl(semid, 0, SETVAL, arg);

        printf("Wyslano %s\n", s);

        // Oddziela pamiec od procesu (zeby ja zniszczyc)
        shmdt(shm_str);
        // Zniszczenie pamieci
        shmctl(shmid, IPC_RMID, NULL);


        //Oczekiwanie na podniesienie semafora z P2
        struct sembuf sem_op = {1, -1, 0};
        semop(semid, &sem_op, 1);

        printf("Odebrano\n");
        // sem_post(sem_cons); // P2 jest odblokowany
        // sem_wait(sem_prod); // P1 jest zablokowany
    }

    // sem_close(sem_prod);
    // sem_close(sem_cons);
    // sem_unlink(sem_producer_name);
    // sem_unlink(sem_consumer_name);

    return 0;
}
