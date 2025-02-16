#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <time.h>

#define MAX_THREADS 10
#define BUF_SIZE 1024

typedef struct {
    int points;
    double radius;
    int* inside_circle;
    pthread_mutex_t* mutex;
} ThreadArgs;

void* monte_carlo_thread(void* arg) {
    ThreadArgs* data = (ThreadArgs*) arg;
    int local_count = 0;

    for (int i = 0; i < data->points; i++) {
        double x = (double)rand() / RAND_MAX * data->radius * 2 - data->radius;
        double y = (double)rand() / RAND_MAX * data->radius * 2 - data->radius;
        if (x * x + y * y <= data->radius * data->radius) {
            local_count++;
        }
    }

    pthread_mutex_lock(data->mutex);
    *(data->inside_circle) += local_count;
    pthread_mutex_unlock(data->mutex);

    return NULL;
}

int double_to_str(double value, char* buffer, int precision) {
    int int_part = (int)value;
    double frac_part = value - int_part;
    char* ptr = buffer;
    int n = 0;

    if (int_part == 0) {
        *ptr++ = '0';
    } else {
        char temp[20];
        char* temp_ptr = temp;
        while (int_part > 0) {
            *temp_ptr++ = '0' + (int_part % 10);
            int_part /= 10;
        }
        while (temp_ptr != temp) {
            *ptr++ = *--temp_ptr;
            n++;
        }
    }

    *ptr++ = '.';

    for (int i = 0; i < precision; i++) {
        frac_part *= 10;
        int digit = (int)frac_part;
        *ptr++ = '0' + digit;
        frac_part -= digit;
        n++;
    }

    *ptr = '\n';

    return n;
}

void print_double(double value) {
    char buffer[32];
    int n = double_to_str(value, buffer, 6);
    write(STDOUT_FILENO, buffer, (n + 2));
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        char* msg = "Usage: <radius> <total_points> <num_threads>\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return 1;
    }

    double radius = atof(argv[1]);
    int total_points = atoi(argv[2]);
    int num_threads = atoi(argv[3]);

    if (num_threads <= 0 || num_threads > MAX_THREADS || total_points <= 0 || radius <= 0) {
        char* msg = "Invalid input values.\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        return 1;
    }

    srand(time(NULL));

    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadArgs *args = (ThreadArgs*)malloc(num_threads * sizeof(ThreadArgs));
    int points_per_thread = total_points / num_threads;
    int points_in_circle = 0;

    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);

    for (int i = 0; i < num_threads; i++) {
        args[i].points = points_per_thread;
        args[i].radius = radius;
        args[i].inside_circle = &points_in_circle;
        args[i].mutex = &mutex;
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_create(threads + i, NULL, monte_carlo_thread, args + i);
    }

    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);

    double square_area = 4 * radius * radius;
    double circle_area = (double)points_in_circle / total_points * square_area;

    print_double(circle_area);

    free(threads);
    free(args);

    return 0;
}