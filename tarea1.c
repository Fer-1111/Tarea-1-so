#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char *input = NULL;
    size_t len = 0;
    ssize_t nread;

    printf("Tarea:~$ ");
    fflush(stdout);

    while ((nread = getline(&input, &len, stdin)) != -1) {
        if (input[nread - 1] == '\n') {
            input[nread - 1] = '\0';
        }

        // Verificar si el comando es "exit"
        if (strcmp(input, "exit") == 0) {
            break;
        }

        // Dividir la línea de entrada en comandos separados por "|"
        char **commands = NULL;
        int command_count = 0;
        char *command = strtok(input, "|");

        while (command != NULL) {
            commands = realloc(commands, sizeof(char *) * (command_count + 1));
            commands[command_count++] = command;
            command = strtok(NULL, "|");
        }

        int in_fd = 0;  // Para la redirección de entrada del primer comando
        int pipe_fds[2];

        for (int i = 0; i < command_count; i++) {
            // Crear un pipe si no es el último comando
            if (i < command_count - 1) {
                pipe(pipe_fds);
            }

            if (fork() == 0) {
                // Redirigir la entrada desde el pipe anterior
                dup2(in_fd, STDIN_FILENO);

                if (i < command_count - 1) {
                    // Redirigir la salida al siguiente pipe
                    dup2(pipe_fds[1], STDOUT_FILENO);
                }

                // Cerrar los descriptores de pipe no usados
                close(pipe_fds[0]); 
                close(pipe_fds[1]);

                // Tokenizar el comando en argumentos
                char **args = NULL;
                int argc = 0;
                char *arg = strtok(commands[i], " ");
                while (arg != NULL) {
                    args = realloc(args, sizeof(char *) * (argc + 1));
                    args[argc++] = arg;
                    arg = strtok(NULL, " ");
                }
                args = realloc(args, sizeof(char *) * (argc + 1));
                args[argc] = NULL;  // El último argumento debe ser NULL

                // Ejecutar el comando
                if (execvp(args[0], args) == -1) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Esperar a que el proceso hijo termine
                wait(NULL);

                // Cerrar la escritura en el pipe y mantener la lectura para el próximo comando
                close(pipe_fds[1]);
                in_fd = pipe_fds[0];
            }
        }

        free(commands);  // Liberar la memoria de los comandos
        printf("Tarea:~$ ");
        fflush(stdout);
    }

    free(input);  // Liberar la memoria de la línea de entrada
    return 0;
}
