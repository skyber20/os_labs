#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


typedef struct {
    char *text;
    size_t size;
} Buffer;


int append(Buffer *buffer, const char *line, size_t length) {
    char *text = realloc(buffer->text, buffer->size + length);

    if (text == NULL) {
        return -1;
    }

    buffer->text = text;
    memcpy(buffer->text + buffer->size, line, length);
    buffer->size += length;
    return 0;
}


int map_output(const char *filename, Buffer *buffer, char **mapping) {
    int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, 0644);

    if (fd == -1) {
        perror("Ошибка открытия файла");
        return -1;
    }

    if (ftruncate(fd, (off_t)buffer->size) == -1) {
        perror("Ошибка изменения размера файла");
        close(fd);
        return -1;
    }

    if (buffer->size > 0) {
        *mapping = mmap(NULL, buffer->size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

        if (*mapping == MAP_FAILED) {
            perror("Ошибка mmap");
            *mapping = NULL;
            close(fd);
            return -1;
        }

        memcpy(*mapping, buffer->text, buffer->size);
    }

    if (close(fd) == -1) {
        perror("Ошибка закрытия файла");
        return -1;
    }

    return 0;
}


pid_t start_child(const char *filename) {
    pid_t pid = fork();

    if (pid != 0) {
        return pid;
    }

    execl(CHILD_PATH, "lab04_child", filename, NULL);
    perror("Ошибка запуска дочернего процесса");
    _exit(1);
}


int wait_child(pid_t pid) {
    int status;
    pid_t result;

    do {
        result = waitpid(pid, &status, 0);
    } while (result == -1 && errno == EINTR);

    if (result == -1) {
        perror("Ошибка ожидания дочернего процесса");
        return -1;
    }

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "Дочерний процесс %d завершился с ошибкой\n", pid);
        return -1;
    }

    return 0;
}


int main(void) {
    char *file1 = NULL;
    char *file2 = NULL;
    size_t file1_size = 0;
    size_t file2_size = 0;

    printf("Введите имя файла для child1: ");
    fflush(stdout);
    ssize_t file1_length = getline(&file1, &file1_size, stdin);

    if (file1_length == -1) {
        fprintf(stderr, "Ошибка чтения имени первого файла\n");
        free(file1);
        return 1;
    }

    if (file1_length > 0 && file1[file1_length - 1] == '\n') {
        file1[--file1_length] = '\0';
    }

    if (file1_length == 0) {
        fprintf(stderr, "Имя первого файла не может быть пустым\n");
        free(file1);
        return 1;
    }

    printf("Введите имя файла для child2: ");
    fflush(stdout);
    ssize_t file2_length = getline(&file2, &file2_size, stdin);

    if (file2_length == -1) {
        fprintf(stderr, "Ошибка чтения имени второго файла\n");
        free(file1);
        free(file2);
        return 1;
    }

    if (file2_length > 0 && file2[file2_length - 1] == '\n') {
        file2[--file2_length] = '\0';
    }

    if (file2_length == 0 || strcmp(file1, file2) == 0) {
        fprintf(stderr, "Имена файлов должны быть непустыми и разными\n");
        free(file1);
        free(file2);
        return 1;
    }

    Buffer first = {NULL, 0};
    Buffer second = {NULL, 0};
    char *line = NULL;
    size_t line_size = 0;
    int failed = 0;

    printf("Введите строки. Для завершения нажмите Ctrl+D:\n");

    while (1) {
        ssize_t length = getline(&line, &line_size, stdin);

        if (length == -1) {
            if (ferror(stdin)) {
                perror("Ошибка чтения строки");
                failed = 1;
            }
            break;
        }

        size_t text_length = (size_t)length;
        if (text_length > 0 && line[text_length - 1] == '\n') {
            text_length--;
        }

        Buffer *buffer = text_length > 10 ? &second : &first;

        if (append(buffer, line, (size_t)length) == -1) {
            perror("Ошибка выделения памяти");
            failed = 1;
            break;
        }
    }

    free(line);

    char *map1 = NULL;
    char *map2 = NULL;

    if (!failed && map_output(file1, &first, &map1) == -1) {
        failed = 1;
    }
    if (!failed && map_output(file2, &second, &map2) == -1) {
        failed = 1;
    }

    if (!failed) {
        pid_t child1 = start_child(file1);
        pid_t child2 = child1 == -1 ? -1 : start_child(file2);

        if (child1 == -1 || child2 == -1) {
            perror("Ошибка создания дочернего процесса");
            failed = 1;
        }

        if (child1 != -1 && wait_child(child1) == -1) {
            failed = 1;
        }
        if (child2 != -1 && wait_child(child2) == -1) {
            failed = 1;
        }
    }

    if (map1 != NULL && munmap(map1, first.size) == -1) {
        perror("Ошибка munmap");
        failed = 1;
    }
    if (map2 != NULL && munmap(map2, second.size) == -1) {
        perror("Ошибка munmap");
        failed = 1;
    }

    free(first.text);
    free(second.text);
    free(file1);
    free(file2);
    return failed;
}
