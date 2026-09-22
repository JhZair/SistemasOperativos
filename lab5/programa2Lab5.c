#include <stdio.h>
#include <unistd.h>

int main(void)
{
    char buffer[100];
    ssize_t n;

    n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);

    if (n > 0) {
        buffer[n] = '\0';
        printf("Programa 2 recibió: %s", buffer);
    }

    return 0;
}