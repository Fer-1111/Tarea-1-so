Compilar:
Usar Compilador gcc en el archivo tarea1.c
Sintaxis de los comandos:
	-exit
	detiene la ejecucion de nuestro interprete
	-favs 
	asd
	-set_recordatorio [tiempo en segundos] [texto del recordatiorio]
	se imprimira el texto del recordatior pasado el tiempo ingresado
El resto de comandos linux son ejecutados la mayoria con normalidad pero hay algunas excepciones estre estas:
	-Las pipes se puede ejecutar con la pipe unida al comando anterior no es necesario un espacion para separarlas comando1|comando2
	-El comando cd no funciona correctamente por las limitaciones de execvp pero una forma de implementarlo seria con la funcion chdir() de unistd.h
