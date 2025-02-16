#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 512

int is_prime(int n) {
    if (n < 2) {
        return 0;
    }
    for (int i = 2; i * i <= n; i++){
        if (n % i == 0) {
            return 0;
        }
    }
    return 1;
}

int is_num(char c) {
    return (c >= '0') && (c <= '9');
}

int is_space(char c) {
    return (c == ' ') || (c == '\t') || (c == '\n');
}

int process_line(int* res) {
    char buffer[BUFFER_SIZE];
    int index = 0;
    char c;
    int sign = 1;
    *res = 0;

    while(read(STDIN_FILENO, &c, sizeof(char)) > 0) {
        if (c == '-') {
            if (index != 0) {
                return -1;
            }
            sign = -1;
        } else if(is_num(c)) {
            *res = *res * 10 + (c - '0');
        } else if (is_space(c)) {
            break;
        } else {
            return -1;
        }
        index++;
    }
    *res *= sign;
    return (index > 0) ? 0 : -1;
}

int main() {
    int n;
    int has_composite_numbers = 0;

    while (1) {
        if (n <= 0 || is_prime(n)) {
            exit(EXIT_FAILURE);
        } else {
            has_composite_numbers = 1;
            if (write(STDOUT_FILENO, &n, sizeof(int)) != sizeof(int)) {
                exit(EXIT_FAILURE);
            }
        }
        if (process_line(&n) < 0) {
            if (has_composite_numbers) {
                exit(EXIT_SUCCESS);
            } else {
                exit(EXIT_FAILURE);
            }
        } 
    }
    return 0;
}