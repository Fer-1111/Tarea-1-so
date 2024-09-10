#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

char *favoritos_path = NULL;  // Variable global para almacenar la ruta del archivo de favoritos

void favs_crear(char *ruta);
void favs_guardar(char**favoritos, int favoritos_count, char* ruta);
void favs_cargar(char***favoritos, int *favoritos_count, char* ruta);
void favs_borrar();
void favs_mostrar();
void favs_ejecutar();
//void killChild(int sigNum);

int main() {
    char *input = NULL;
    size_t bufflen = 0;
    char **favoritos = NULL; // Vector dinámico de favoritos.
    int favoritos_count = 0;

    favs_crear("./misfavoritos.txt");

    favs_cargar(&favoritos, &favoritos_count, "./misfavoritos.txt");

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
        //Variable que indica si los comandos se ejcutaron correctamente.
        int huboerror = 0;
        //Se reemplaza el salto de linea '\n' de la entrada por un '\0' (fin de string).
        if (input[strlen(input) - 1] == '\n') {
            input[strlen(input) - 1] = '\0';
        }

        //Implementación del comando exit.
        if (strcmp(input, "exit") == 0) {
            favs_guardar(favoritos, favoritos_count, "./misfavoritos.txt");
            for(int s = favoritos_count - 1; s >= 0; s--) {
                free(favoritos[s]);
            }
            free(favoritos);
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

                //Se verifica que el primer argumento del comando sea nuestro comando personalizado.
                /*if (strcmp(args[0], "set_recordatorio") == 0){
                    //Identificador del proceso que se encargara del recordatorio.
                    pid_t reminder_pid = fork();
                    //Manejo de error de la funcion fork().
                    if (reminder_pid == -1){                
                        perror("Error al generar nuevo proceso con fork().");
                        exit(EXIT_FAILURE);
                    }
                    if (reminder_pid == 0){
                        //receptor de señal que hara funcionar killChild cuando reciba una señal de alarm.
                        signal(SIGALRM,killChild);
                        //Tiempo del Recordatorio dado por el argumento 1 del comando.
                        int time = atoi(args[1]);
                        if (time != 0 && argc > 2){
                            //el proceso se detiene por el tiempo proporcionado por el comando.
                            sleep(time);
                            printf("\nRecordatorio: ");
                            for (int i = 2; i < argc; ++i){
                                printf("%s ", args[i]);
                            }
                            printf("\n");
                            printf("Tarea:~$ ");
                            fflush(stdout);
                            //Liberamos la memoria de los argumentos del comando.
                            free(args);
                            //señal utilizada para marcar el final del proceso.
                            alarm(1); 
                            //loop que da tiempo a la señal de activarse.
                            while(1);
                        }else{
                            printf("Error argumentos del comando no valido.\n");
                            printf("Tarea:~$ ");
                            fflush(stdout);
                            free(args);
                            alarm(1);
                            while(1);
                        }
                    }else{
                        fflush(stdout);
                        free(args);
                        kill(getpid(), SIGKILL);
                    }
                }*/
               
                //Si el comando no es personalizado:
                // Ejecucion de comandos propios de linux con execvp.
                if (execvp(args[0], args) == -1) {
                    printf("El comando ingresado no es valido\n");
                    huboerror = 1;
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
        //Si no hubo error de ejecucion, se intenta ingresar el comando a favoritos
	    if(huboerror == 0){
            //Se verifica si el comando ya esta en los favoritos
            int comandoNoEsta = 0;
            for(int k = 0; k < favoritos_count; k++) {
                if(*favoritos[k] == *input) {
                    comandoNoEsta = 1;
                    break;
                }
            }
            //Si corresponde se ingresa el ultimo comando ejecutado a favoritos
            if(comandoNoEsta == 0) {
                favoritos_count +=1;
                favoritos = (char**)realloc(favoritos,sizeof(char*)*(favoritos_count));
                favoritos[favoritos_count -1] = strdup(input);
            }
        } 
        //Si no corresponde, no se ingresa el comando a favoritos y reiniciamos el flag de error de ejecucion
        else {
	        huboerror = 0;	
	    }

        if(input != NULL) {
            free(input);
            input = NULL;
        }
        bufflen = 0;
        fflush(stdout);
        free(commands);
        printf("Tarea:~$ ");
    }
    return 0;
}

void favs_crear(char *ruta) {
    // Verificar si el archivo ya existe
    FILE *archivo = fopen(ruta, "r");
    if (archivo != NULL) {
        // El archivo ya existe
        fclose(archivo);
        return;
    }
    archivo = fopen(ruta, "w");  // Intentar crear el archivo en la ruta especificada
    if (archivo == NULL) {
        perror("Error al crear el archivo de favoritos");
        return;
    }
    fclose(archivo);

    // Guardar la ruta del archivo en la variable global
    favoritos_path = strdup(ruta);
}

void favs_guardar(char** favoritos, int favoritos_count, char* ruta) {
    //Manejo de errores: Si los punteros son NULL, la funcion no hace nada.
    if(favoritos == NULL) {
        return;
    }
    if(ruta == NULL) {
        return;
    }
    FILE *archivo = fopen(ruta, "w");
    if (archivo == NULL) {
        return;
    }
    //El primer dato del archivo será la cantidad de comandos favoritos
    fprintf(archivo, "%d\n", favoritos_count);

    //Guardamos los comandos en el archivo
    for(int i = 0; i < favoritos_count; i++) {
        fputs(favoritos[i], archivo);
        fputs("\n", archivo);
    }
    fclose(archivo);
    return;
}

void favs_cargar(char***favoritos, int *favoritos_count, char*ruta) {
    if(ruta == NULL) {
        return;
    }
    // Abrir el archivo en modo lectura
    FILE *archivo = fopen(ruta, "r");
    // Verificar si el archivo se abrió correctamente
    if (archivo == NULL) {
        printf("Error al cargar favoritos\n");
        return;
    }
    // Leer línea por línea
    fscanf(archivo, "%d", favoritos_count);
    *favoritos = (char **)realloc(*favoritos, sizeof(char *) * (*favoritos_count));
    if (*favoritos == NULL) {
        fclose(archivo);
        printf("Error al cargar favoritos\n");
        return;
    }

    for (int k = 0; k < *favoritos_count; k++) {
        // Asignar memoria para cada cadena
        (*favoritos)[k] = (char *)malloc(512); // Asumiendo un tamaño máximo de 1024 caracteres
        if ((*favoritos)[k] == NULL) {
            // Liberar memoria previamente asignada en caso de fallo
            for (int j = 0; j < k; j++) {
                free((*favoritos)[j]);
            }
            free(*favoritos);
            fclose(archivo);
            return;
        }
        
        // Leer la línea
        fgets((*favoritos)[k], 512, archivo);
        // Eliminar el salto de línea si existe
        (*favoritos)[k][strcspn((*favoritos)[k], "\n")] = '\0';
    }
    // Cerrar el archivo
    fclose(archivo);
    return;
}

void favs_borrar() {

}

void favs_mostrar() {

}

void favs_ejecutar() {

}
/*//Funcion que elimina el proceso al que pertenece tras recibir una señal
void killChild(int sigNum){
    kill(getpid(), SIGKILL);
}*/