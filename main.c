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

static const int g_ftok_shm_id = 100;
char sem_producer_name[6] = "/prod";
char sem_consumer_name[6] = "/cons";

//Id pototmnych potrzebne globalnie do przesylania powiadomienia dalej
pid_t pid1, pid2, pid3;
void usr1Handler() {
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

int main()
{
    // Ustawienie obslugi sygnalow
    signal(SIGTSTP, SIG_IGN);
    signal(SIGCONT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGUSR1, usr1Handler);

    // Przygotuj pipes
    // pipes[0] - read
    // pipes[1] - write
    // Pipe przekazuje tylko 1 byte na raz, wiec trzeba konwertowac int na string


    // Przygotuj shared memory key
    key_t shm_key;
    if ((shm_key = ftok("./main", g_ftok_shm_id)) == -1) {
        printf("Nie mozna wygenerowac klucza pamieci dzielonej\n");
        return -1;
    }

    // Zamien na string aby przekazac do procesow potomnych
    char shm_key_str[12];
    char pipe_write_str[11];
    char pipe_read_str[11];
    sprintf(shm_key_str, "%d", shm_key);

    pid1 = fork();
    switch(pid1)
    {
        case 0: // Semafory i pamiec wspoldzielona test
            execl("./p1", "./p1", shm_key_str, (char*)NULL); // Wykonywane w tym samym procesie potomnym
            exit(0);
        case -1:
            printf("Blad tworzenia procesu\n");
            exit(1);
        default:
            pid2 = fork();
            switch(pid2)
            {
                case 0:
                    // Pisze do p3
                    close(pipes[0]);
                    execl("./p2", "./p2", shm_key_str, pipe_write_str, (char*)NULL);
                    exit(0);
                case -1:
                    printf("Blad tworzenia procesu\n");
                    exit(1);
                default:
                    pid3 = fork();
                    switch(pid3)
                    {
                        case 0:
                            close(pipes[1]);
                            execl("./p3", "./p3", pipe_read_str, (char*)NULL);
                            exit(0);
                        case -1:
                            printf("Blad tworzenia procesu\n");
                            exit(1);
                        default:
                            close(pipes[0]);
                            close(pipes[1]);

                            wait(NULL);
                            break;
                    }

                    wait(NULL);
                    break;
            }
            wait(NULL);
            break;
    }

    return 0;
}
