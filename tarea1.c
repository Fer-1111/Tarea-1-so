#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char *input = NULL;
    size_t len = 0;

    printf("Tarea:~$ ");

    //Se limpia el buffer previo a la lectura de la entrada para prevenir errores.
    fflush(stdout);

    //Si getline tiene un error al leer la entrada retorna -1.
    while ((getline(&input, &len, stdin)) != -1) {
        
        //Se reemplaza el salto de linea de la entrada por '\0' (fin de string).
        if (input[len - 1] == '\n') {
            input[len - 1] = '\0';
        }

        //Implementación del comando exit.
        if (strcmp(input, "exit") == 0) {
            break;
        }

        //Un TOKEN es un substring extraido del original mediante un proceso de division (tokenizacion).

        // Dividir la línea de entrada en comandos separados por "|"
        char **commands = NULL; //Vector de comandos.
        int command_count = 0;
        char *command = strtok(input, "|"); //Se asigna solo el primer comando (en forma de token) a command

        //Se ingresan los comandos al vector commands.
        while (command != NULL) {
            commands = (char**)realloc(commands, sizeof(char *) * (command_count + 1));
            commands[command_count++] = command;
            command = strtok(NULL, "|"); //Retoma desde el último token que se leyó.
        }

        /*Un PIPE permite que los datos de salida (escritura) de un proceso se redirija directamente a 
        la entrada (lectura) de otro proceso.*/

        /*Un DESCRIPTOR DE ARCHIVO (file descriptor) es un identificador que el SO asigna a un "recurso del
        sistema" (archivos, dispositivos, etc.) en el cual un proceso lee o escribe datos.*/

        //"Initial file descriptor", simboliza la entrada del primer proceso (se inicializa en 0)
        int in_fd = 0;
        //File descriptors de escritura (proceso 1) y lectura (proceso 2) usado para crear pipes.
        int pipe_fds[2];

        for(int i = 0; i < command_count; i++) {
            // Crear un pipe si no es el último comando
            if(i < command_count - 1) {
                pipe(pipe_fds);
            }
            
            //Ejecucion del proceso hijo.
            if(fork() == 0) {
                //Recibir la entrada desde el pipe anterior
                dup2(in_fd, STDIN_FILENO);

                if (i < command_count - 1) {
                    //Redirigir la salida al siguiente pipe
                    dup2(pipe_fds[1], STDOUT_FILENO);
                }

                //Cerrar los file descriptors de pipe no usados
                close(pipe_fds[0]); 
                close(pipe_fds[1]);

                //Tokenizar el comando en argumentos

                //Vector de argumentos.
                char **args = NULL;
                //Contador de argumentos.
                int argc = 0;
                //Argumento actual.
                char *arg = strtok(commands[i], " ");

                //Ingreso de argumentos al vector "args".
                while (arg != NULL) {
                    args = (char**)realloc(args, sizeof(char *) * (argc + 1));
                    args[argc++] = arg;
                    arg = strtok(NULL, " ");
                }

                //El último argumento debe ser NULL
                args = (char**)realloc(args, sizeof(char *) * (argc + 1));
                args[argc] = NULL;

                // Ejecutar el comando
                if (execvp(args[0], args) == -1) {
                    perror("execvp");
                    exit(EXIT_FAILURE);
                }
            } 
            
            //Ejecución del proceso padre.
            else {
                //Esperar a que el proceso hijo termine
                wait(NULL);

                //Cerrar la escritura en el pipe y mantener la lectura para el próximo comando
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
