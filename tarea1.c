#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char* favoritos_path = "./misFavoritos.txt"; //Ruta del archivo de comandos favoritos

void favs_crear(char* path_archivo);
int favs_guardar(char**favoritos, int favoritos_count, char* ruta);
int favs_cargar(char***favoritos, int *favoritos_count, char* ruta);
void favs_borrar(char*ruta);
void favs_mostrar(char*ruta);
char* favs_ejecutar(char**favoritos, int i);
void killChild(int sigNum);

int main() {
    char* input = NULL; //Buffer de entrada por teclado
    char* inputcpy = NULL; //Copia del buffer
    size_t inputLen = 0; //Tamaño del Buffer

    char** favoritos = NULL; //Arreglo dinamico de comandos favoritos
    int favoritos_count = 0; //Cantidad de comandos favoritos

    favs_crear(favoritos_path);

    int error = 0;
    if(error = favs_cargar(&favoritos, &favoritos_count, favoritos_path) != 0) {
        printf("Error %d al cargar comandos favoritos\n", error);
    }

    printf("Tarea:~$ "); //Prompt inicial

    fflush(stdout); //Se limpia el buffer previo a la lectura de datos
    while(getline(&input, &inputLen, stdin) != -1) {
        //Se reemplaza el salto de linea '\n', por un '\0' (fin de string).
        input[strlen(input) - 1] = '\0';
        
        //Copiamos el buffer
        inputcpy = (char*)malloc(strlen(input) + 1);
        strcpy(inputcpy, input);

        //Implementación del comando exit
        if(strcmp(input, "exit") == 0) {
            if(favoritos != NULL) {
                for(int i = favoritos_count - 1; i >= 0; i--) {
                    if(favoritos[i] != NULL) {
                        free(favoritos[i]);
                    }
                }
            }
            free(input);
            break;
        }

        //Parsear comandos pipe, separados por "|"
        char **commands = NULL; //Arreglo dinamico de comandos (cada comando es un token del tipo char*)
        char *command = strtok(input, "|"); //Comando actual
        int commands_count = 0; //Cantidad de comandos pipe

        //Inicializar commands
        while(command != NULL) {
            commands_count += 1;
            commands = (char**)realloc(commands, sizeof(char *) * (commands_count));
            commands[commands_count - 1] = command;
            command = strtok(NULL, "|"); //Retoma desde el último token que se leyo
        }

        /*Nota: A nivel interno, se reemplazaron los char '|' por '\0' en la variable input.
        Los elementos del arreglo commands en verdad son punteros hacia input*/

        /*Un DESCRIPTOR DE ARCHIVO (file descriptor) es el identificador (int) que el SO asigna a 
        un "recurso del sistema" (archivos, dispositivos, etc.) desde el cual un proceso lee o 
        escribe datos.*/
        int in_fd = 0; //"Initial file descriptor" (lectura)
        int pipe_fds[2]; //File descriptors de lectura pipe_fds[0] y escritura pipe_fds[1]
        int exit_status = 0; //0 si lo comandos se ejecutaron sin problema, != 0 en otro caso

        //EJECUCION DE COMANDOS
        int c; //Indice comando pipe actual
        for(c = 0; c < commands_count; c++) {
            //Crear pipe para comunicarse con el proximo proceso
            if(c < commands_count - 1) {
                if(pipe(pipe_fds) == -1) { //Manejo de error de la funcion pipe()
                    perror("Error al crear pipe");
                    exit(EXIT_FAILURE);
                }
            }

            pid_t pid = fork(); //Id del proceso actual
            if(pid == -1) { //Manejo de error de la funcion fork()
                perror("Error al generar nuevo proceso con fork()");
                exit(EXIT_FAILURE);
            }

            //EJECUCION DEL PROCESO HIJO
            if(pid == 0) {
                //MANEJO DE PIPES HIJO
                if(commands_count > 1) {
                    if(dup2(in_fd, STDIN_FILENO) == -1) {
                        perror("Error funcion dup2");
                        exit(EXIT_FAILURE);
                    }
                    close(in_fd);

                    //Redirigir la salida al siguiente pipe si el comando actual no es el ultimo
                    if(c < commands_count - 1) { //Nota: El ultimo comando tiene indice [commands_count - 1]
                        if(dup2(pipe_fds[1], STDOUT_FILENO) == -1) {
                            perror("Error funcion dup2");
                            exit(EXIT_FAILURE);
                        }
                        close(pipe_fds[1]);  
                    }
                }

                //Tokenizar el comando en argumentos
                char **args = NULL; //Arreglo dinamico de argumentos
                int argc = 0; //Contador de argumentos
                char *arg = strtok(commands[c], " "); //Argumento actual
                while(arg != NULL) {
                    argc += 1;
                    args = (char**)realloc(args, sizeof(char *) * (argc));
                    args[argc - 1] = arg;
                    arg = strtok(NULL, " ");
                }
                if(arg == NULL) {
                    argc += 1;
                    args = (char**)realloc(args, sizeof(char *) * (argc));
                    args[argc - 1] = arg;
                }

                //Si el comando no es personalizado:
                //Ejecucion de comandos propios de linux con execvp.
                if(execvp(args[0], args) == -1) {
                    perror("El comando ingresado no es valido");
                    if(args != NULL) {
                        free(args);
                    }
                    exit(1);
                }
            }

            //EJECUCION DEL PROCESO PADRE
            else {
                //Espera a que proceso hijo termine
                int estado;
                pid_t child_pid = wait(&estado);
                if(child_pid == -1) {
                    perror("Error en wait");
                    exit(EXIT_FAILURE);
                }

                //Verificar si el hijo terminó normalmente
                if(WIFEXITED(estado)) {
                    //Si cualquiera de los hijos finalizo con problemas, se vera reflejado en exit_status al final de la ejecucion general
                    if(WEXITSTATUS(estado) > exit_status) {
                        exit_status = WEXITSTATUS(estado);
                    }
                }

                //MANEJO DE PIPES PADRE
                if(commands_count > 1){
                    //Cerrar el pipe de escritura
                    close(pipe_fds[1]);
                    //Pasarle el pipe de lectura al proximo proceso
                    in_fd = pipe_fds[0];
                    if(c == commands_count - 1) {
                        close(pipe_fds[0]);
                    }
                }
            }
        }
        
        //POST EJECUCION DE COMANDOS

        //REGISTRAR COMANDO FAVORITO
        if(exit_status == 0) { //0 si no hubo errores de ejecucion
            printf("Ejecucion exitosa\n");
            int comandoRegistrado = 0;
            for(int i = 0; i < favoritos_count; i++) {
                if(strcmp(inputcpy, favoritos[i]) == 0) {
                    printf("El comando favorito ya se encuentra registrado\n");
                    comandoRegistrado = 1;
                    break;
                }
            }
            if(comandoRegistrado == 0) {
                favoritos_count += 1;
                favoritos = (char**)realloc(favoritos, sizeof(char *) * (favoritos_count));
                favoritos[favoritos_count - 1] = (char*)malloc(strlen(inputcpy) + 1);
                strcpy(favoritos[favoritos_count - 1], inputcpy);
                int error;
                if(error = favs_guardar(favoritos, favoritos_count, favoritos_path) != 0) {
                    printf("Error %d al guardar comandos favoritos\n", error);
                }
            }
        } else {
            printf("Error al registrar comando favorito\n");
        }

        //Limpieza commands
        if(commands != NULL) {
            free(commands);
        }
        //Limpieza buffer
        if(input != NULL) {
            free(input);
            input = NULL;
        }
        if(inputcpy != NULL) {
            free(inputcpy);
            inputcpy = NULL;
        }
        inputLen = 0;

        fflush(stdout);
        printf("Tarea:~$ ");
    }
    return(EXIT_SUCCESS);
}

//Crea un archivo de comandos favoritos si no existe
void favs_crear(char* path_archivo) {
    //Verificar si el archivo ya existe
    FILE *archivo = fopen(path_archivo, "r");
    if(archivo != NULL) {
        //El archivo ya existe
        fclose(archivo);
        return;
    }
    //Si no existe intentar crear el archivo en la ruta especificada
    else {
        archivo = fopen(path_archivo, "w");
        if(archivo == NULL) {
            perror("Error al crear el archivo de favoritos");
            return;
        }
        fclose(archivo);
    }
    //Nota: Archivo inicializado sin caracteres
    return;
}

int favs_guardar(char** favoritos, int favoritos_count, char* ruta) {
    //Manejo de errores
    if(favoritos == NULL) {
        //Error: La lista de comandos favoritos no es valida
        return 1;
    }
    if(ruta == NULL) {
        //Error: La ruta especificada para guardar no es valida
        return 2;
    }
    if(favoritos_count < 1) {
        //Error: La cantidad de comandos favoritos no es valida
        return 3;
    }
    FILE *archivo = fopen(ruta, "w");
    if(archivo == NULL) {
        return 4;
    }
    //Guardamos los comandos en el archivo
    for(int i = 0; i < favoritos_count; i++) {
        fputs(favoritos[i], archivo);
        fputs("\n", archivo);
    }
    fclose(archivo);
    return 0;
}

int favs_cargar(char***favoritos, int *favoritos_count, char*ruta) {
    if(ruta == NULL) {
        //Error: La ruta especificada para guardar no es valida
        return 1;
    }
    if(*favoritos_count != 0) {
        //Error: Cantidad de favoritos no inicializada correctamente
        return 2;
    }
    //Abrir el archivo en modo lectura
    FILE *archivo = fopen(ruta, "r");
    if (archivo == NULL) {
        printf("Error al abrir archivo\n");
        return 3;
    }
    //Leer línea por línea
    char buffer[512]; //Buffer para almacenar la cadena leída
    *favoritos_count = 0;
    while(fgets(buffer, sizeof(buffer), archivo) != NULL) { 
        buffer[strcspn(buffer, "\n")] = '\0'; //Elimina el salto de línea
        *favoritos_count += 1;
        *favoritos = (char**)realloc(*favoritos, sizeof(char *) * (*favoritos_count));
        if(*favoritos == NULL) {
            printf("Error de asignación de memoria\n");
            fclose(archivo);
            return 4;
        }
        (*favoritos)[*favoritos_count - 1] = (char*)malloc(sizeof(char) * strlen(buffer) + 1);
        if((*favoritos)[*favoritos_count - 1] == NULL) {
            printf("Error de asignación de memoria\n");
            fclose(archivo);
            //Podria haber fuga?
            return 4;
        }
        strcpy((*favoritos)[*favoritos_count - 1], buffer);
    }
    // Cerrar el archivo
    fclose(archivo);
    return 0;
}