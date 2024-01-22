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
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>


bool work = 0;


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

void usr1Handler() {
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
    else if (sig == SIGCONT) {
        work = 0;
    }
    else if (sig == SIGTERM) {
        exit(0);
    }
}

// Tutaj jest printowana liczba znakow w linijce
int main(int argc, char *argv[])
{
    // argv[0] - filepath
    // argv[1] - pipe_read_str
    if(argc != 2)
        return -1;

    signal(SIGTSTP, sigHandler);
    signal(SIGCONT, sigHandler);
    signal(SIGTERM, sigHandler);
    signal(SIGUSR1, usr1Handler);

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
