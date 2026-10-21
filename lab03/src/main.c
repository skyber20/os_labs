#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <pthread.h>


typedef struct {
    double x, y, z;
} Point;

typedef struct {
    Point *points;
    int n;
    int next_i;
    pthread_mutex_t mutex;
} Shared;

typedef struct {
    Shared *shared;
    double max_score;
    int best_i, best_j, best_k;
} ThreadData;


double area_score(Point a, Point b, Point c) {
    double ab_x = b.x - a.x;
    double ab_y = b.y - a.y;
    double ab_z = b.z - a.z;

    double ac_x = c.x - a.x;
    double ac_y = c.y - a.y;
    double ac_z = c.z - a.z;

    double cross_x = ab_y * ac_z - ab_z * ac_y;
    double cross_y = ab_z * ac_x - ab_x * ac_z;
    double cross_z = ab_x * ac_y - ab_y * ac_x;

    return cross_x * cross_x + cross_y * cross_y + cross_z * cross_z;
}


double seconds(void) {
    struct timespec current_time;

    if (clock_gettime(CLOCK_MONOTONIC, &current_time) != 0) {
        perror("clock_gettime");
        exit(1);
    }

    return (double)current_time.tv_sec + (double)current_time.tv_nsec / 1e9;
}


void *worker(void *arg) {
    ThreadData *data = arg;
    Shared *shared = data->shared;

    while (1) {
        pthread_mutex_lock(&shared->mutex);

        int i = shared->next_i;
        shared->next_i++;

        pthread_mutex_unlock(&shared->mutex);

        if (i >= shared->n - 2) {
            break;
        }

        for (int j = i + 1; j < shared->n - 1; j++) {
            for (int k = j + 1; k < shared->n; k++) {
                double score = area_score(shared->points[i], shared->points[j], shared->points[k]);

                if (score > data->max_score) {
                    data->max_score = score;
                    data->best_i = i;
                    data->best_j = j;
                    data->best_k = k;
                }
            }
        }
    }

    return NULL;
}


int main(int argc, char *argv[]) {
    if (argc != 3 || strcmp(argv[1], "-t") != 0) {
        fprintf(stderr, "Использование: ./lab03 -t N\n");
        return 1;
    }

    int threads;

    if (sscanf(argv[2], "%d", &threads) != 1 || threads < 1) {
        fprintf(stderr, "Некорректное количество потоков\n");
        return 1;
    }

    int n;

    printf("Введите количество точек:\n");

    if (scanf("%d", &n) != 1 || n < 3) {
        fprintf(stderr, "Некорректное количество точек\n");
        return 1;
    }

    if (threads > n - 2) {
        threads = n - 2;
    }

    Point *points = malloc(n * sizeof(Point));

    if (points == NULL) {
        perror("malloc");
        return 1;
    }

    printf("Введите координаты каждой точки x, y, z:\n");

    for (int i = 0; i < n; i++) {
        if (scanf("%lf %lf %lf", &points[i].x, &points[i].y, &points[i].z) != 3) {
            fprintf(stderr, "Ошибка чтения координат\n");
            free(points);
            return 1;
        }
    }

    Shared shared = {points, n, 0};

    if (pthread_mutex_init(&shared.mutex, NULL) != 0) {
        fprintf(stderr, "Ошибка создания mutex\n");
        free(points);
        return 1;
    }

    pthread_t *ids = malloc(threads * sizeof(pthread_t));
    ThreadData *data = malloc(threads * sizeof(ThreadData));

    if (ids == NULL || data == NULL) {
        perror("malloc");
        free(ids);
        free(data);
        pthread_mutex_destroy(&shared.mutex);
        free(points);
        return 1;
    }

    for (int i = 0; i < threads; i++) {
        data[i].shared = &shared;
        data[i].max_score = -1;
        data[i].best_i = 0;
        data[i].best_j = 0;
        data[i].best_k = 0;
    }

    printf("Потоков: %d\n", threads);

    double start = seconds();

    for (int i = 1; i < threads; i++) {
        if (pthread_create(&ids[i], NULL, worker, &data[i]) != 0) {
            fprintf(stderr, "Ошибка создания потока\n");
            exit(1);
        }
    }

    worker(&data[0]);

    for (int i = 1; i < threads; i++) {
        if (pthread_join(ids[i], NULL) != 0) {
            fprintf(stderr, "Ошибка ожидания потока\n");
            exit(1);
        }
    }

    double elapsed = seconds() - start;

    double max_score = -1;
    int best_i = 0, best_j = 0, best_k = 0;

    for (int i = 0; i < threads; i++) {
        if (data[i].max_score > max_score) {
            max_score = data[i].max_score;
            best_i = data[i].best_i;
            best_j = data[i].best_j;
            best_k = data[i].best_k;
        }
    }

    double max_area = sqrt(max_score) / 2;

    printf("Максимальная площадь: %f\n", max_area);
    printf("Индексы точек: %d %d %d\n", best_i, best_j, best_k);
    printf("Точка A: %g %g %g\n", points[best_i].x, points[best_i].y, points[best_i].z);
    printf("Точка B: %g %g %g\n", points[best_j].x, points[best_j].y, points[best_j].z);
    printf("Точка C: %g %g %g\n", points[best_k].x, points[best_k].y, points[best_k].z);
    printf("Время: %f секунд\n", elapsed);

    pthread_mutex_destroy(&shared.mutex);

    free(ids);
    free(data);
    free(points);

    return 0;
}
