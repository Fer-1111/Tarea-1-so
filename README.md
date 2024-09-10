Compilar código: 
- En una máquina WSL ejecutar $ gcc tarea1.c
- Ejecutar $ ./a.out

Syntaxis de comandos:
- La terminal soporta los comandos linux en general, en este caso se escriben exactamente igual. (Nota: El comando cd no funciona correctamente por las limitaciones de execvp pero una forma de implementarlo seria con la funcion chdir() de unistd.h)
- Comando exit: $ exit
- Comandos favs:
- $ favs mostrar
- $ favs borrar
- favs crear, guardar y cargar se ejecutan por defecto, no estan a disposicion del usuario.
- Set recordatorio: $ set_recordatorio [tiempo en segundos] [texto del recordatiorio] (Nota: Se imprimira el texto del recordatiorio un vez pasado el tiempo especificado
