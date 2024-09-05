#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    char *input = NULL;
    size_t bufflen = 0;

    printf("Tarea:~$ ");

    /*Un BUFFER es un área de memoria temporal utilizada para almacenar datos mientras
    se trasladan de un lugar a otro. En la terminal, los caracteres de la entrada se guardan
    en el buffer hasta que se presiona Enter, momento en el que se procesan. Con getline() al
    presionar Enter se genera el caracter '\n', que es almacenado como cualquier otro caracter,
    lo que podría causar problemas.*/

    //Se limpia el buffer previo a la lectura de la entrada para prevenir errores.
    fflush(stdout);

    //Si getline tiene un error al leer la entrada retorna -1.
    while (getline(&input, &bufflen, stdin) != -1) { //Aquí len es el tamaño que se le asigna al buffer.
        
        //Se reemplaza el salto de linea '\n' de la entrada por un '\0' (fin de string).
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        //Implementación del comando exit.
        if (strcmp(input, "exit") == 0) {
            free(input);
            return 0;
        }
        /*Un TOKEN es un substring extraido del original mediante un proceso de division (tokenizacion).*/

        // Dividir la línea de entrada en comandos separados por "|"
        char **commands = NULL; //Vector dinámico de comandos (cada comando es un char*/string dinámico).
        char *command = strtok(input, "|"); //Se asigna solo el primer comando (en forma de token) a command.
        int command_count = 0;

        //Se ingresan los comandos al vector commands.
        while (command != NULL) {
            command_count += 1;
            commands = (char**)realloc(commands, sizeof(char *) * (command_count));
            commands[command_count - 1] = command;
            command = strtok(NULL, "|"); //Retoma desde el último token que se leyó.
        }
        /*Nota 1: Es necesario escribir los comandos con pipes SIN ESPACIOS EN BLANCO alrededor de '|', pero
        con espacios en blanco entre argumentos de un mismo comando.
         Ej:$ ps -aux|sort -nr -k 4|head -20
         Nota 2: El ultimo comando del vector será null, esto debido al funcionamiento de exec en linux.*/

        /*Un PIPE permite que los datos de salida (escritura) de un proceso se redirija directamente a 
        la entrada (lectura) de otro proceso.*/

        /*Un DESCRIPTOR DE ARCHIVO (file descriptor) es el identificador (int) que el SO asigna a 
        un "recurso del sistema" (archivos, dispositivos, etc.) desde el cual un proceso lee o 
        escribe datos.*/

        //"Initial file descriptor", simboliza la entrada del primer proceso (se inicializa en 0)
        int in_fd = 0;
        //File descriptors de escritura (proceso 1) y lectura (proceso 2) usado para crear pipes.
        int pipe_fds[2];

        //Ejecución de comandos
        for(int i = 0; i < command_count; i++) {
            //Crear un pipe si no es el último comando
            if(i < command_count - 1) {
                if(pipe(pipe_fds) == -1) { //Manejo de error de la funcion pipe().
                    perror("Error al crear pipe.");
                    exit(EXIT_FAILURE);
                }
            }
            //Identificador del proceso actual.
            pid_t pid = fork();
            if(pid == -1) { //Manejo de error de la funcion fork().
                perror("Error al generar nuevo proceso con fork().");
                exit(EXIT_FAILURE);
            }
            //Ejecucion del proceso hijo.
            if(pid == 0) {
                //Recibir la entrada desde el pipe anterior
                if(command_count != 1) {
                    dup2(in_fd, STDIN_FILENO);
                    close(pipe_fds[0]); 
                } 
                if (i < command_count - 1) {
                    //Redirigir la salida al siguiente pipe
                    dup2(pipe_fds[1], STDOUT_FILENO);
                    close(pipe_fds[1]);
                }
                //Tokenizar el comando en argumentos

                //Vector de argumentos.
                char **args = NULL;
                //Contador de argumentos.
                int argc = 0;
                //Argumento actual.
                char *arg = strtok(commands[i], " ");

                //Ingreso de argumentos del comando actual 'i' al vector "args".
                while (arg != NULL) {
                    argc += 1;
                    args = (char**)realloc(args, sizeof(char *) * (argc));
                    args[argc - 1] = arg;
                    arg = strtok(NULL, " ");
                }

                /*Nota para los desarrolladores: Debemos diferenciar nuestros comandos de los de linux
                antes de ingresarlos a execvps
                
                Se debe mejorar el manejo de errores de execvp*/

                // Ejecucion de comandos propios de linux con execvp.
                if (execvp(args[0], args) == -1) {
                    perror("execvp");
                    free(args);
                    exit(EXIT_FAILURE);
                }
            } 
            //Ejecución del proceso padre.
            else {
                //Esperar a que el proceso hijo termine
                wait(NULL);

                if(command_count != 1){
                    //Cerrar el pipe de escritura
                    close(pipe_fds[1]);
                    //Pasarle el pipe de lectura al proximo proceso
                    in_fd = pipe_fds[0];
                    close(pipe_fds[0]);
                }
            }
        }
        if(input != NULL) {
            free(input);
            input = NULL;
        }
        bufflen = 0;
        fflush(stdout);
        free(commands); //Liberamos la memoria de los comandos
        printf("Tarea:~$ ");
    }
    printf("\nAdios\n");
    return 0;
}
