#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>

int print_int(const int num) {
    char buf[16];
    char res[32];
    int n = 0;
    int sign = (num < 0);
    int x = abs(num);
    if (x == 0) {
        write(STDOUT_FILENO, "0\n", 2);
        return 0;
    }
    while (x) {
        buf[n] = (x % 10) + '0';
        x = x / 10;
        n++;
    }
    if (sign) {
        res[0] = '-';
    }
    for (int i = 0; i < n; i++) {
        res[i + sign] = buf[n - i - 1];
    }
    res[n + sign] = '\n';
    write(STDOUT_FILENO, res, (n + sign + 1));
    return 0;
}

int read_line(char** buf, int* n){
    char c;
    int i = 0;
    while(1) {
        if (read(STDIN_FILENO, &c, sizeof(char)) == -1) {
            return -1;
        }
        (*buf)[i] = c;
        i++;
        if (i >= *n) {
            *buf = realloc(*buf, (*n) * 2 * sizeof(char));
            *n = *n * 2;
        }
        if (c == '\n') {
            (*buf)[i - 1] = '\0';
            break;
        }
    }
    return 0;
}

int main() {
    char* buf = malloc(128 * sizeof(char));
    int n = 128;
    if (read_line(&buf, &n) == -1) {
        char* msg = "failed to read file name\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    int file_fd = open(buf, O_RDONLY);
    if (file_fd == -1) {
        char* msg = "failed to open file\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    int pipe_fd [2];
    if (pipe(pipe_fd) == -1) {
        char* msg = "failed to create pipe\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }

    __pid_t pid = fork();

    if (pid == 0) {
        if (dup2(file_fd, STDIN_FILENO) == -1) {
            write(STDOUT_FILENO, "dup2 failed\n", 12);
            exit(EXIT_FAILURE);
        }
        close(file_fd);

        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
            write(STDOUT_FILENO, "dup2 failed\n", 12);
            exit(EXIT_FAILURE);
        }
        close(pipe_fd[1]);
        close(pipe_fd[0]);

        char *arg[] = {"./child.out", NULL};
        if (execv("./child.out", arg) == -1) {
            write(STDOUT_FILENO, "execv failed\n", 13);
            exit(EXIT_FAILURE);
        }
    } else {
        close(pipe_fd[1]);

        int x;
        int received = 0;
        while (read(pipe_fd[0], &x, sizeof(int)) > 0) {
            print_int(x);
            received = 1; 
        }
        close(pipe_fd[0]);

        int status;
        waitpid(pid, &status, 0); 

        if (WIFEXITED(status)) {
            int exit_code = WEXITSTATUS(status);
            if (exit_code != 0) {
                char* msg = "ERROR: child exited with an error\n";
                write(STDERR_FILENO, msg, strlen(msg));
                exit(EXIT_FAILURE);
            }
        }
    }
    
    if (pid == -1) {
        char* msg = "fail to fork\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }
    return 0;
}