#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

int main(void) {
    char *line = NULL;
    size_t line_size = 0;

    while (1) {
        ssize_t line_length = getline(&line, &line_size, stdin);

        if (line_length == -1) {
            break;
        }

        size_t text_length = line_length;

        if (text_length > 0 && line[text_length - 1] == '\n') {
            text_length--;
        }

        if (text_length > 0) {
            size_t left = 0;
            size_t right = text_length - 1;

            while (left < right) {
                char temp = line[left];
                line[left] = line[right];
                line[right] = temp;

                left++;
                right--;
            }
        }

        if (printf("%s", line) < 0) {
            perror("Ошибка записи результата");
            free(line);
            return 1;
        }
    }

    if (ferror(stdin)) {
        perror("Ошибка чтения строки");
        free(line);
        return 1;
    }

    if (fflush(stdout) == EOF) {
        perror("Ошибка записи результата");
        free(line);
        return 1;
    }

    free(line);

    return 0;
}