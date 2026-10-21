#include <stdio.h>
#include <stdlib.h>


void generate_points(int n) {
    FILE *file = fopen("lab03/bench_input.txt", "w");

    if (file == NULL) {
        perror("Ошибка создания файла");
        exit(1);
    }

    srand(42);
    fprintf(file, "%d\n", n);

    for (int i = 0; i < n; i++) {
        int x = rand() % 201 - 100;
        int y = rand() % 201 - 100;
        int z = rand() % 201 - 100;

        fprintf(file, "%d %d %d\n", x, y, z);
    }

    fclose(file);
}


double run(int threads, double *area) {
    char command[128];

    snprintf(command, sizeof(command), "./build/lab03/lab03 -t %d < lab03/bench_input.txt", threads);

    FILE *pipe = popen(command, "r");

    if (pipe == NULL) {
        perror("Ошибка запуска программы");
        exit(1);
    }

    char line[256];
    double time = -1;
    *area = -1;

    while (fgets(line, sizeof(line), pipe) != NULL) {
        sscanf(line, "Максимальная площадь: %lf", area);
        sscanf(line, "Время: %lf", &time);
    }

    if (pclose(pipe) != 0 || time < 0 || *area < 0) {
        fprintf(stderr, "Ошибка измерения времени или площади\n");
        exit(1);
    }

    return time;
}


int main(void) {
    int sizes[] = {500, 1000, 1500};

    for (int s = 0; s < 3; s++) {
        int n = sizes[s];
        generate_points(n);

        printf("\nКоличество точек: %d\n", n);
        printf("Потоки | Время (с) | Ускорение | Эффективность (%%)\n");

        double time_one = 0;
        double expected_area = -1;

        for (int threads = 1; threads <= 16; threads *= 2) {
            double area;

            run(threads, &area);

            if (threads == 1) {
                expected_area = area;
            } else if (area != expected_area) {
                fprintf(stderr, "Ошибка: площади не совпадают\n");
                return 1;
            }

            double total = 0;

            for (int repeat = 0; repeat < 5; repeat++) {
                total += run(threads, &area);

                if (area != expected_area) {
                    fprintf(stderr, "Ошибка: площади не совпадают\n");
                    return 1;
                }
            }

            double average = total / 5;

            if (threads == 1) {
                time_one = average;
            }

            double speedup = time_one / average;
            double efficiency = speedup / threads * 100;

            printf("%6d | %9.6f | %9.2f | %15.1f%%\n",
                   threads, average, speedup, efficiency);
        }
    }

    remove("lab03/bench_input.txt");

    return 0;
}
