#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {

    char *input = NULL;
    size_t len = 0;
    ssize_t nread;
    
    printf("Tarea:~$  ");
    
    while ((nread = getline(&input, &len, stdin)) != -1) {
        // Remover el salto de línea al final de la entrada
        if (input[nread - 1] == '\n') {
            input[nread - 1] = '\0';
                    }        
        // Tokenizar la línea de entrada
        char **args = malloc(sizeof(char *) * 1);
        int argc = 0;
        char *token = strtok(input, " ");
        
        while (token != NULL) {
            args = realloc(args, sizeof(char *) * (argc + 2));
            args[argc] = token;
            argc++;
            token = strtok(NULL, " ");
        }
        
        args[argc] = NULL; // El último argumento debe ser NULL para execvp
        if (argc == 0) {
            printf("Tarea:~$  ");
            continue;
        }
        
        // Ejecutar el comando
        if (fork() == 0) {
            
            if(execvp(args[0], args) == -1){ 
				printf("ERROR DE COMANDO\n");
                fflush(stdout);
			}

            exit(EXIT_FAILURE);

        } else {

            wait(NULL); // Esperar a que el hijo termine

        }

        free(args); // Liberar la memoria de los argumentos
        printf("Tarea:~$  "); // Mostrar el prompt de nuevo
        fflush(stdout);  //Libera el buffer para imprimir directamente 

     }

    free(input); // Liberar la memoria de la línea de entrada
    return 0;
}




void favs(char **args){





}

