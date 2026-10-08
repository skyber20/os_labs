#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


int close_pipe(int pipefd[2]) {
    int result1 = close(pipefd[0]);
    int result2 = close(pipefd[1]);

    if (result1 == -1 || result2 == -1) {
        fprintf(stderr, "Ошибка закрытия канала\n");
        return -1;
    }

    return 0;
}


int write_all(int fd, const char *buffer, size_t size) {
    size_t written_total = 0;

    while (written_total < size) {
        ssize_t written = write(fd, buffer + written_total, size - written_total);

        if (written <= 0) {
            return -1;
        }

        written_total += written;
    }

    return 0;
}


pid_t start_child(const char *filename, int own_pipe[2], int other_pipe[2], int number) {
    pid_t pid = fork();

    if (pid != 0) {
        return pid;
    }

    int result1 = close(own_pipe[1]);
    int result2 = close(other_pipe[0]);
    int result3 = close(other_pipe[1]);

    if (result1 == -1 || result2 == -1 || result3 == -1) {
        fprintf(stderr, "Ошибка закрытия дескрипторов\n");
        _exit(1);
    }

    fprintf(stderr, "Child%d PID: %d\n", number, getpid());

    int file_fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    if (file_fd == -1) {
        perror("Ошибка открытия файла");
        _exit(1);
    }

    if (dup2(own_pipe[0], STDIN_FILENO) == -1 ||
        dup2(file_fd, STDOUT_FILENO) == -1) {
        perror("Ошибка перенаправления");
        _exit(1);
    }

    result1 = close(own_pipe[0]);
    result2 = close(file_fd);

    if (result1 == -1 || result2 == -1) {
        fprintf(stderr, "Ошибка закрытия дескрипторов\n");
        _exit(1);
    }

    execl(CHILD_PATH, "lab02_child", NULL);

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
    ssize_t file1_length = getline(&file1, &file1_size, stdin);

    if (file1_length == -1) {
        fprintf(stderr, "Ошибка чтения имени первого файла\n");
        free(file1);
        return 1;
    }

    if (file1_length > 0 && file1[file1_length - 1] == '\n') {
        file1[file1_length - 1] = '\0';
        file1_length--;
    }

    if (file1_length == 0) {
        fprintf(stderr, "Имя первого файла не может быть пустым\n");
        free(file1);
        return 1;
    }

    printf("Введите имя файла для child2: ");
    ssize_t file2_length = getline(&file2, &file2_size, stdin);

    if (file2_length == -1) {
        fprintf(stderr, "Ошибка чтения имени второго файла\n");
        free(file1);
        free(file2);
        return 1;
    }

    if (file2_length > 0 && file2[file2_length - 1] == '\n') {
        file2[file2_length - 1] = '\0';
        file2_length--;
    }

    if (file2_length == 0) {
        fprintf(stderr, "Имя второго файла не может быть пустым\n");
        free(file1);
        free(file2);
        return 1;
    }

    if (strcmp(file1, file2) == 0) {
        fprintf(stderr, "Имена выходных файлов должны отличаться\n");
        free(file1);
        free(file2);
        return 1;
    }

    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1) {
        perror("Ошибка создания pipe1");
        free(file1);
        free(file2);
        return 1;
    }

    if (pipe(pipe2) == -1) {
        perror("Ошибка создания pipe2");

        close_pipe(pipe1);

        free(file1);
        free(file2);
        return 1;
    }

    pid_t child1_pid = start_child(file1, pipe1, pipe2, 1);

    if (child1_pid == -1) {
        perror("Ошибка создания child1");

        close_pipe(pipe1);
        close_pipe(pipe2);

        free(file1);
        free(file2);
        return 1;
    }

    pid_t child2_pid = start_child(file2, pipe2, pipe1, 2);

    if (child2_pid == -1) {
        perror("Ошибка создания child2");

        close_pipe(pipe1);
        close_pipe(pipe2);

        wait_child(child1_pid);

        free(file1);
        free(file2);
        return 1;
    }

    printf("Parent PID: %d\n", getpid());
    printf("Child1 PID: %d\n", child1_pid);
    printf("Child2 PID: %d\n", child2_pid);

    free(file1);
    free(file2);

    int failed = 0;

    if (close(pipe1[0]) == -1) {
        perror("Ошибка закрытия pipe1");
        failed = 1;
    }

    if (close(pipe2[0]) == -1) {
        perror("Ошибка закрытия pipe2");
        failed = 1;
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("Ошибка настройки SIGPIPE");
        failed = 1;
    }

    char *line = NULL;
    size_t line_size = 0;

    if (!failed) {
        printf("Введите строки. Для завершения нажмите Ctrl+D:\n");

        while (1) {
            ssize_t line_length = getline(&line, &line_size, stdin);

            if (line_length == -1) {
                if (ferror(stdin)) {
                    perror("Ошибка чтения строки");
                    failed = 1;
                }
                break;
            }

            size_t text_length = line_length;

            if (text_length > 0 && line[text_length - 1] == '\n') {
                text_length--;
            }

            int fd = text_length > 10 ? pipe2[1] : pipe1[1];

            if (write_all(fd, line, line_length) == -1) {
                perror("Ошибка записи в канал");
                failed = 1;
                break;
            }
        }
    }

    free(line);

    if (close(pipe1[1]) == -1) {
        perror("Ошибка закрытия pipe1");
        failed = 1;
    }

    if (close(pipe2[1]) == -1) {
        perror("Ошибка закрытия pipe2");
        failed = 1;
    }

    if (wait_child(child1_pid) == -1) {
        failed = 1;
    }

    if (wait_child(child2_pid) == -1) {
        failed = 1;
    }

    return failed;
}