#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <string.h>


bool work = 0;
union senum {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

//ID pototmnych potrzebne globalnie do przesylania powiadomienia dalej
pid_t pid1, pid2, pid3;
void usr1HandlerPM() {
    const char* fifo[3] = {"fifo_1", "fifo_2", "fifo_3"};
    int fd[3] = {};
    int sig = 11;
    // Odczytanie wartosci sygnalu
    mkfifo(fifo[2], 0666);
    fd[2] = open(fifo[2], O_RDONLY);
    read(fd[2], &sig, sizeof(int));
    close(fd[2]);

    // Powiadomienie do P1
    kill(pid1, SIGUSR1);

    // Tworzenie kolejek fifo
    for (int i = 0; i < 3; i++) {
        mkfifo(fifo[i], 0666);
        fd[i] = open(fifo[i], O_WRONLY);
        write(fd[i], &sig, sizeof(int));
        close(fd[i]);
    }
}

void usr1HandlerP1() {
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
    else if (sig == SIGQUIT) {
        work = 0;
    }
    else if (sig == SIGTERM) {
        printf("Zamykam(P1)\n");
        exit(0);
    }
}

void usr1HandlerP2() {
    int sig;
    kill(getpid() + 1, SIGUSR1);
    //Odczytanie z kolejki fifo
    char* fifo = "fifo_2";
    mkfifo(fifo, 0666);
    int fd = open(fifo, O_RDONLY);
    read(fd, &sig, sizeof(int));
    close(fd);

    if (sig == SIGTSTP) {
        work = 1;
    }
    else if (sig == SIGQUIT) {
        work = 0;
    }
    else if (sig == SIGTERM) {
        printf("Zamykam(P2)\n");
        exit(0);
    }
}

void sigHandler(int sig) {
    //Powiadomienie procesu potomnego
    kill(getppid(), SIGUSR1);
    //Przekazanie informacji o sygnale
    char* fifo = "fifo_3";
    mkfifo(fifo, 0666);
    int fd = open(fifo, O_WRONLY | O_CREAT);
    write(fd, &sig, sizeof(int));
    close(fd);
}

void usr1HandlerP3() {
    int sig;
    //Odczytanie z kolejki fifo
    char* fifo = "fifo_3";
    mkfifo(fifo, 0666);
    int fd = open(fifo, O_RDONLY);
    read(fd, &sig, sizeof(int));
    close(fd);

    if (sig == SIGTSTP) {
        work = 1;
    }
    else if (sig == SIGQUIT) {
        work = 0;
    }
    else if (sig == SIGTERM) {
        printf("Zamykam(P3)\n");
        exit(0);
    }
}

int main()
{
    // Ustawienie obslugi sygnalow
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGUSR1, usr1HandlerPM);

    int fd[2];
    pipe(fd);
    pid1 = fork();
    switch(pid1)
    {
        case 0:	//P1
            //Przygotowanie do obslugi pamieci wspoldzielonej
            key_t shm_key;
            shm_key = ftok("shm_key", 100);

            //Tworzenie zestawu semaforow
            const key_t sem_key = ftok("sem", 1);
            int semid = semget(sem_key, 2, 0666 | IPC_CREAT);
            union senum arg;
            arg.val = 0;
            semctl(semid, 0, SETVAL, 0);


            int shmid = shmget(shm_key, 1024, 0666 | IPC_CREAT); // Pamiec dzielona na 12288 znakow
            char* shm_str = shmat(shmid, 0, 0); //shm_str mozna traktowac jako char shm_str[12288]

            while(1)
            {
                signal(SIGTSTP, SIG_IGN);
                signal(SIGQUIT, SIG_IGN);
                signal(SIGTERM, SIG_IGN);
                signal(SIGINT, SIG_IGN);
                signal(SIGUSR1, usr1HandlerP1);
                int choice;

                printf("Skad chcesz zczytywac dane? (0) Z klawiatury (1) Z pliku\n> ");
                if (!scanf("%d", &choice) || (choice != 0 && choice != 1)) // Scanf zwraca 0 gdy wystapi blad w czytaniu
                {
                    gets();
                    printf("Wprowadzono zla wartosc. Wprowadz jeszcze raz\n");
                    continue;
                }


                if (!choice) { // Z klawiatury
                    printf("Wprowadz dane:\n> ");
                    getchar();
                    fgets(shm_str, 1024, stdin);
                    while(work) {;}

                    // Podniesienie semafora w P2
                    arg.val = 1;
                    semctl(semid, 0, SETVAL, arg);
                    //Oczekiwanie na podniesienie semafora z P2
                    struct sembuf sem_op = {1, -1, 0};
                    semop(semid, &sem_op, 1);
                }
                else
                {
                    printf("Umiesc plik w folderze z programami. Podaj nazwe pliku wraz z rozszerzeniem:\n> ");
                    char filename[100];
                    scanf("%s", filename);

                    FILE* file;
                    file = fopen(filename, "r");

                    if (file == NULL) {printf("Nie ma takiego pliku\n"); continue;}
                    while(fgets(shm_str, 1024, file)) {
                        while(work) {;}
                        // Podniesienie semafora w P2
                        arg.val = 1;
                        semctl(semid, 0, SETVAL, arg);

                        //Oczekiwanie na podniesienie semafora z P2
                        struct sembuf sem_op = {1, -1, 0};
                        semop(semid, &sem_op, 1);
                    }

                    fclose(file);
                }

            }

            shmdt(shm_str);
            shmctl(shmid, IPC_RMID, NULL);
            exit(0);
        case -1:
            printf("Blad tworzenia procesu\n");
            exit(1);
        default:
            pid2 = fork();
            switch(pid2)
            {
                case 0: //P2
                    signal(SIGTSTP, SIG_IGN);
                    signal(SIGQUIT, SIG_IGN);
                    signal(SIGTERM, SIG_IGN);
                    signal(SIGINT, SIG_IGN);
                    signal(SIGUSR1, usr1HandlerP2);
                    key_t sem_key = ftok("sem", 1);
                    int semid = semget(sem_key, 2, 0666 | IPC_CREAT);
                    union senum arg;
                    arg.val = 0;
                    semctl(semid, 0, SETVAL, 0);

                    key_t shm_key = ftok("shm", 100);
                    int shmid = shmget(shm_key, 1024, 0666 | IPC_CREAT); // Pamiec dzielona na 12288 znakow
                    char* shm_str = shmat(shmid, 0, 0); //shm_str mozna traktowac jako char shm_str[12288]

                    close(fd[0]);
                    int i;
                    while(1)
                    {

                        struct sembuf sem_op_0 = {0 , -1, 0};
                        semop(semid, &sem_op_0, 1);
                        i = strlen(shm_str)-1;
                        while(work) {;}
                        write(fd[1], &i, sizeof(int));

                        //Zwrotna informacja do P1
                        struct sembuf sem_op = {1, 1, 0};
                        semop(semid, &sem_op, 1);
                    }

                    //Usuwanie pamieci wspoldzielonej
                    shmdt(shm_str);
                    shmctl(shmid, IPC_RMID, NULL);
                    close(fd[1]);
                    exit(0);
                case -1:
                    printf("Blad tworzenia procesu\n");
                    exit(1);
                default:
                    pid3 = fork();
                    switch(pid3)
                    {
                        case 0:	//P3

                            signal(SIGTSTP, sigHandler);
                            signal(SIGQUIT, sigHandler);
                            signal(SIGTERM, sigHandler);
                            signal(SIGINT, SIG_IGN);
                            signal(SIGUSR1, usr1HandlerP3);
                            close(fd[1]);
                            int data;
                            long line = 1;
                            while(1)
                            {
                                read(fd[0], &data, sizeof(int));
                                while(work) {;}
                                printf("Liczba znakow w %ld linijce: %d\n",line++, data);
                            }
                            close(fd[0]);
                            exit(0);
                        case -1:
                            printf("Blad tworzenia procesu\n");
                            exit(1);
                        default:
                            wait(NULL);
                            break;
                    }

                    wait(NULL);
                    break;
            }
            wait(NULL);
            break;
    }
    printf("Zamykam(PM)\n");
    return 0;
}
