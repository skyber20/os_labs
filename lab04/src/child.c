#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Не указано имя файла\n");
        return 1;
    }

    int fd = open(argv[1], O_RDWR);

    if (fd == -1) {
        perror("Ошибка открытия файла");
        return 1;
    }

    struct stat info;
    if (fstat(fd, &info) == -1) {
        perror("Ошибка получения размера файла");
        close(fd);
        return 1;
    }

    if (info.st_size == 0) {
        if (close(fd) == -1) {
            perror("Ошибка закрытия файла");
            return 1;
        }
        return 0;
    }

    size_t size = (size_t)info.st_size;
    char *text = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if (close(fd) == -1) {
        perror("Ошибка закрытия файла");
        if (text != MAP_FAILED) {
            munmap(text, size);
        }
        return 1;
    }

    if (text == MAP_FAILED) {
        perror("Ошибка mmap");
        return 1;
    }

    size_t start = 0;

    for (size_t i = 0; i <= size; i++) {
        if (i == size || text[i] == '\n') {
            size_t left = start;
            size_t right = i;

            while (left < right) {
                char temp = text[left];
                text[left] = text[right - 1];
                text[right - 1] = temp;
                left++;
                right--;
            }

            start = i + 1;
        }
    }

    if (msync(text, size, MS_SYNC) == -1) {
        perror("Ошибка msync");
        munmap(text, size);
        return 1;
    }

    if (munmap(text, size) == -1) {
        perror("Ошибка munmap");
        return 1;
    }

    return 0;
}
