#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ftw.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>

#define MAXENTRADA 2048
#define MAXNOMBRE 1024
#define MAXLIST 20
#define MAXDATE 64
#define MAXPATH 1024


struct COMANDO{
  char * nombre;
  void (*funcion)(char**);
};

struct HELP{
  char * nombre;
  char * descripcion;
};

struct FILE{
  char * nombre;
  int df;
};

static struct FILE fileList[MAXLIST];

void MostrarDirActual()
{
   char dir[MAXNOMBRE];
   
    if (getcwd(dir, MAXNOMBRE) == NULL)
	perror("Imposible obtener directorio");
    else
       printf ("%s\n",dir);
}

int ComprobarSegundoPlano (char *tr[])
{
    int i;
    for (i = 0; tr[i] != NULL; i++)
        if (!strcmp(tr[i], "&")){ /*& indica segund plano*/ 
            tr[i] = NULL;         /*es el ultimo argumento*/
            return i;       /*si solo hay un & no se ejecuta nada en pplano*/
            }
    return 0;
}


void Proceso (char *tr[], int splano)
{
   pid_t pid;
   void Cmd_exec (char **);
   int background = splano || ComprobarSegundoPlano(tr);
   if ((pid = fork()) == -1){
        perror ("Imposible crear proceso");
        return;
        }
  if (pid == 0){  /*proceso hijo*/
    Cmd_exec (tr);
    exit(255); /*por si falla exec*/
    }
  if (!background) 
    waitpid(pid, NULL, 0);
}

int EsDirectorio (char * dir){          /*para saber si algo es directorio o no*/
    struct stat s;

    if (lstat(dir,&s) == -1){       /*si no puedo acceder: para mi no es directorio*/
        return 0;
    }
    
    return (S_ISDIR(s.st_mode));
}

char LetraTF (mode_t m)
{
     switch (m&S_IFMT) { /*and bit a bit con los bits de formato,0170000 */
        case S_IFSOCK: return 's'; /*socket */
        case S_IFLNK: return 'l'; /*symbolic link*/
        case S_IFREG: return '-'; /* fichero normal*/
        case S_IFBLK: return 'b'; /*block device*/
        case S_IFDIR: return 'd'; /*directorio */ 
        case S_IFCHR: return 'c'; /*char device*/
        case S_IFIFO: return 'p'; /*pipe*/
        default: return '?'; /*desconocido, no deberia aparecer*/
     }
}

char * ConvierteModo3 (mode_t m)
{
    char *permisos;

    if ((permisos = (char *) malloc (12)) == NULL)
        return NULL;
    strcpy (permisos, "---------- ");
    
    permisos[0] = LetraTF(m);
    if (m&S_IRUSR) permisos[1]='r';    /*propietario*/
    if (m&S_IWUSR) permisos[2]='w';
    if (m&S_IXUSR) permisos[3]='x';
    if (m&S_IRGRP) permisos[4]='r';    /*grupo*/
    if (m&S_IWGRP) permisos[5]='w';
    if (m&S_IXGRP) permisos[6]='x';
    if (m&S_IROTH) permisos[7]='r';    /*resto*/
    if (m&S_IWOTH) permisos[8]='w';
    if (m&S_IXOTH) permisos[9]='x';
    if (m&S_ISUID) permisos[3]='s';    /*setuid, setgid y stickybit*/
    if (m&S_ISGID) permisos[6]='s';
    if (m&S_ISVTX) permisos[9]='t';
    
    return permisos;
}

/*********************************************/
/*************COMANDOS DEL SHELL************************/
void Cmd_fin (char * arg[])  /*todos los cmd_ comparten prototipo*/
{                            /*reciben los mismos parametros aunque no los usen*/
  exit(0);
}

void Cmd_authors(char *arg[])
{
  if(arg[0]==NULL){
    printf ("Jose Manuel Iglesias Díaz y Daniel Marrero Sánchez\n ");
    printf("jose.iglesias.diaz@udc.es  daniel.marrero.sanchez@udc.es\n");
  }
  else if(strcmp(arg[0],"-l")==0){
    printf ("jose.iglesias.diaz@udc.es  daniel.marrero.sanchez@udc.es\n ");
  }
  else if(strcmp(arg[0], "-n")==0){
    printf ("Jose Manuel Iglesias Díaz y Daniel Marrero Sánchez\n");
  }
}

void Cmd_date(char *arg[])
{
  if(arg[0]==NULL){
    time_t ahora= time(NULL);
    printf("Fecha: %.10s\n",ctime(&ahora));
    printf("Hora: %.8s\n",ctime(&ahora)+11);
  }
  else if(strcmp(arg[0], "-d")==0){
    time_t ahora=time(NULL);
    printf("Fecha: %.10s\n",ctime(&ahora));
  }
  else if(strcmp(arg[0], "-t")==0){
    time_t ahora=time(NULL);
    printf("Hora: %.8s\n",ctime(&ahora)+11);
  }
}

void Cmd_exec (char *arg[])
{
  if (execvp(arg[0], arg) == -1)
	perror ("Imposible ejecutar");
}

void Cmd_splano (char *arg[])
{
  Proceso (arg,1);
}
void Cmd_pplano (char *arg[])
{
  Proceso(arg,0);
}

void Cmd_pwd(char * arg[])
{
  MostrarDirActual();
}

void Cmd_pid (char * arg[])
{
    if (arg[0] == NULL)
        printf ("El pid del proceso es %d\n", (int) getpid());
    else
        if (!strcmp (arg[0],"-p"))
            printf ("El pid del proceso padre es %d\n", (int) getppid());
}

void Cmd_sysinfo(char *arg[])
{
    system("uname -a");
}

void Cmd_dup(char *arg[])
{
  int df, newdf;
  if(arg[0]==NULL){
    printf("Uso: dup <descriptor>\n");
    return;
  }
  df=atoi(arg[0]);

  newdf=dup(df);

  if(newdf==-1){
    perror("Error al duplicar descriptor");
  }else{
    printf("Descriptor duplicado: %d\n", newdf);
  }
}

void Cmd_lseek(char *arg[]){
  int df, origen;
  off_t offset, resultado;
  if(arg[0]==NULL || arg[1]==NULL || arg[2]==NULL){
    printf("Uso: lseek <descriptor><offset><SEEK_SET|SEEK_CUR|SEEK_END>\n");
    return;
  }
  df = atoi(arg[0]);
  offset=atol(arg[1]); 

  if(strcmp(arg[2], "SEEK_SET")==0){
    origen=SEEK_SET;
  }else if(strcmp(arg[2], "SEEK_CUR")==0){
    origen=SEEK_CUR;
  }else if(strcmp(arg[2], "SEEK_END")==0){
    origen=SEEK_END;
  }else{
    printf("Origen no valido");
    return;
  }

  resultado=lseek(df,offset,origen);

  if(resultado == (off_t)-1){
    perror("Error al mover el puntero del descriptor");
  }
  else{
    printf("Nueva posicion: %ld\n", (long)resultado);
  }
}

void Cmd_makefile(char *arg[]){
  FILE *file;

  if(arg[0]==NULL){
    printf("Falta el nombre del archivo\n");
    return;
  }

  file=fopen(arg[0], "wx");

  if(file ==NULL){
    perror("Error al crear el archivo");
    return;
  }
  fclose(file);

}

void Cmd_readstr(char *arg[]){
  int df, cont, leidos;
    char cadena[1024];

    if (arg[0] == NULL || arg[1] == NULL) {
        printf("Uso: readstr <df> <cont>\n");
        return;
    }

    df = atoi(arg[0]);
    cont = atoi(arg[1]);

    if (cont > 1023 || cont <= 0) {
        printf("Cantidad no valida\n");
        return;
    }

    leidos = read(df, cadena, cont);

    if (leidos == -1) {
        perror("Error al leer");
        return;
    }

    cadena[leidos] = '\0';
    printf("%s\n", cadena);

}

void Cmd_writestr(char *arg[]){
  int df, escritos;

    if (arg[0] == NULL || arg[1] == NULL) {
        printf("Uso: writestr <df> <cadena>\n");
        return;
    }

    df = atoi(arg[0]);

    escritos = write(df, arg[1], strlen(arg[1]));

    if (escritos == -1) {
        perror("Error al escribir");
        return;
    }

    printf("Bytes escritos: %d\n", escritos);

}

void Cmd_help (char *arg[])
{ 

  static struct HELP C[]={ 
   {"fin", "Finaliza sesión en el Shell"},
   {"exit", "Finaliza sesión en el Shell"},
   {"bye", "Finaliza sesión en el Shell"},
   {"quit", "Finaliza la sesión en el Shell"},
   {"date", "MUestra la fecha actual. -d sólo para la fecha y -t sólo para la hora"},
   {"pid", "Muestra el valor actual del pid. -p para el pid del proceso padre"},
   {"pwd", "Muestra el path del directorio actual"},
   {"chdir","Cambia de directorio al seleccionado"},
   {"authors","Muestra los nombre y logins de los autores de la práctica. -l sólo para los logins y -n sólo para los nombres"},
   {"sysinfo", "Muestra información del sistema"},
   {"help", "Muestra una explicación del comando que se le introduzca como argumento"},
   {"open", "Muestra los nombre y logins de los autores de la práctica"},
   {"close", "Muestra los nombre y logins de los autores de la práctica. -f para cerrar descriptores en mapeo activo"},
   {"listopen", "Muestra la lista de los ficheros abiertos"},
   {"dup", "Duplica el descriptor y lo añade a la lista de ficheros abiertos"},
   {"lseek", "Se posiciona en el offset indicado"},
   {"readstr", "Lee cont bytes de un descriptor y los muestra por pantalla"},
   {"writestr", "Escribe str en el fichero abierto descrito por df"},
   {"makefile", "Crea un fichero de nombre pasado con el argumento"},
   {"makedir", "Crea un directorio de nombre pasado con el argumento"},
   {"delete", "Elimina objetos de sistema de ficheros que se pasen por argumentos, excepto directorios con contenido"},
   {"deltree", "Elimina objetos de sistema de ficheros, incluyendo directorios con contenido"},
   {"listfile", "Da información sobre los archivos pasados por argumentos, inicialmente solo sus nombre y tamañp. -acc añade el último acceso. -long añade fecha de creación (o acceso), número de links, modo, dueño y grupo. -link añade ficheros a dónde apunta"},
   {"list", "Igual que listfile, sólamente que los directorios muestra su contenido y no el objeto en sí. -hid Enlista también fichero no visibles. -reca los directorios son Enlistados recursivamente antes del listado de directorio. --recb igual que --reca, pero antes del listado de directorio"},
  };

  if(arg[0] == NULL){
      for (int i = 0; C[i].nombre != NULL; i++){ /*recorro el array hasta el NULL*/
        printf("%s\n", C[i].nombre);
      }
  }
  
  else{
    for (int i = 0; C[i].nombre != NULL; i++){
      if(strcmp(arg[0], C[i].nombre) == 0){
        printf("%s\n", C[i].descripcion);
      }
    }
  }
}

void Cmd_chdir (char * arg[])
{
   if (arg[0]==NULL){
      MostrarDirActual();
   }
   
   else if (chdir(arg[0]) == -1){
      perror("Imposible cambiar directorio");
   }
}

void Cmd_open (char *arg[])
{
    int i, df, mode = 0; 
    
    if (arg[0] == NULL) { /*no hay parametro*/
        printf("descriptor: 0, offset: (  )-> entrada estandar O_RDWR\n");
        printf("descriptor: 1, offset: (  )-> salida estandar O_RDWR\n");
        printf("descriptor: 2, offset: (  )-> error estandar O_RDWR\n");
        
      for(i = 3; i < MAXLIST; i++){
          
          if(fileList[i].nombre == NULL){
            printf("descriptor: %d, offset: (  )-> no usado", i);
          }
            
          else{
            printf("descriptor: %d, offset: (  )-> %s", fileList[i].df, fileList[i].nombre);
          }

          printf("\n");
      }
      return;
    } 
  
    for (i = 1; arg[i] != NULL; i++){
      if (!strcmp(arg[i],"cr")) mode |= O_CREAT;
      else if (!strcmp(arg[i],"ex")) mode |= O_EXCL;
      else if (!strcmp(arg[i],"ro")) mode |= O_RDONLY; 
      else if (!strcmp(arg[i],"wo")) mode |= O_WRONLY;
      else if (!strcmp(arg[i],"rw")) mode |= O_RDWR;
      else if (!strcmp(arg[i],"ap")) mode |= O_APPEND;
      else if (!strcmp(arg[i],"tr")) mode |= O_TRUNC; 
      else break;
    }

    if ((df = open(arg[0], mode, 0777)) == -1){
        perror ("Imposible abrir fichero");
    }

    else{

        fileList[df].df = df;
        fileList[df].nombre = strdup(arg[0]);
  
        printf ("Anadida entrada %d a la tabla ficheros abiertos\n", df);
    }
}

void Cmd_close (char *arg[])
{ 
  int df, i;
    
  if (arg[0] == NULL || (df = atoi(arg[0])) < 0) { 
       
      printf("descriptor: 0, offset: (  )-> entrada estandar O_RDWR\n");
      printf("descriptor: 1, offset: (  )-> salida estandar O_RDWR\n");
      printf("descriptor: 2, offset: (  )-> error estandar O_RDWR\n");
        
    for(i = 3; i < MAXLIST; i++){
          
      if(fileList[i].nombre == NULL){
        printf("descriptor: %d, offset: (  )-> no usado", i);
      }
            
      else{
        printf("descriptor: %d, offset: (  )-> %s", fileList[i].df, fileList[i].nombre);
      }

      printf("\n");
    }

    return;
  } 
  
    if (close(df) == -1){
        perror("Imposible cerrar descriptor\n");
    }
    
    else{
      free(fileList[df].nombre);
      fileList[df].nombre = NULL;

      printf("Eliminada entrada %d a la tabla ficheros abiertos\n", df);
    }
}

void Cmd_listopen(char *arg[])
{

   int i, df, mode = 0; 
    
  printf("descriptor: 0, offset: (  )-> entrada estandar O_RDWR\n");
  printf("descriptor: 1, offset: (  )-> salida estandar O_RDWR\n");
  printf("descriptor: 2, offset: (  )-> error estandar O_RDWR\n");
        
  for(i = 3; i < MAXLIST; i++){
          
    if(fileList[i].nombre == NULL){
      printf("descriptor: %d, offset: (  )-> no usado", i);
    }
            
    else{
      printf("descriptor: %d, offset: (  )-> %s", fileList[i].df, fileList[i].nombre);
    }

    printf("\n");
  }
  return;
     
}

void Cmd_makedir(char *arg[]){
  int i, mode = 0;

  for (i = 1; arg[i] != NULL; i++)
      if (!strcmp(arg[i],"cr")) mode |= O_CREAT;
      else if (!strcmp(arg[i],"ex")) mode |= O_EXCL;
      else if (!strcmp(arg[i],"ro")) mode |= O_RDONLY; 
      else if (!strcmp(arg[i],"wo")) mode |= O_WRONLY;
      else if (!strcmp(arg[i],"rw")) mode |= O_RDWR;
      else if (!strcmp(arg[i],"ap")) mode |= O_APPEND;
      else if (!strcmp(arg[i],"tr")) mode |= O_TRUNC; 
      else break;

  if(mkdir(arg[0], mode) == -1){
    perror("Imposible crear directorio\n");
  }
}

void Cmd_delete(char * arg[])
{

  struct stat s;
  int i;

  for(int i = 0; arg[i] != NULL; i++){

    if(lstat(arg[i], &s) == -1){
      perror("Imposible identificar objeto\n");
    }

    char tipo = LetraTF(s.st_mode);

    switch(tipo){
      case '-':
        if(remove(arg[i]) == -1){
          perror("Imposible eliminar archivo\n");
        }
        break;
    
      case 'l':
        if(unlink(arg[i]) == -1){
          perror("Imposible eliminar link\n");
        }
        break;

      case 'd':
        if(rmdir(arg[i]) == -1){
          perror("Imposible eliminar directorio\n");
        }
        break;
    }

  }
}

void borrarRecursivo(char *p)
{
    struct stat s;
    struct dirent *e;
    char hijo[MAXPATH];

    if (lstat(p, &s) == -1) {
        perror(p);
        return;
    }

    DIR *d = opendir(p);
    if (d == NULL) {
        perror("Imposible abrir directorio");
        return;
    }

    while ((e = readdir(d)) != NULL) {
      borrarRecursivo(hijo);
    }

    closedir(d);

    if (rmdir(p) == -1){
        perror("No ha sido posible borrar");
    }
}

void Cmd_deltree(char *arg[])
{

  struct stat s;
  int i;

  for(int i = 0; arg[i] != NULL; i++){

    if(lstat(arg[i], &s) == -1){
      perror("Imposible identificar objeto\n");
    }

    char tipo = LetraTF(s.st_mode);

    switch(tipo){
      case '-':
        if(remove(arg[i]) == -1){
          perror("Imposible eliminar archivo\n");
        }
        break;
    
      case 'l':
        if(unlink(arg[i]) == -1){
          perror("Imposible eliminar link\n");
        }
        break;

      case 'd':
        borrarRecursivo(arg[i]);
      }
    }
}

void Cmd_listfile(char *arg[])
{

  int i;
  char date[MAXDATE];
  char path[MAXPATH];
  struct stat s;
  struct tm tm;
  ssize_t p;


  if(strcmp(arg[0], "-acc") != 0  && strcmp(arg[0], "-long") != 0){
         
    for(i = 0; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);

      printf("\n");
    }
  }
  
  if(strcmp(arg[0], "-acc") == 0){
         
    for(i = 1; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      localtime_r(&s.st_atime, &tm);
      strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &tm);

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", date);
      
      printf("\n");
    }
  }
   
  if(strcmp(arg[0], "-long") == 0){
         
    for(i = 1; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      localtime_r(&s.st_ctime, &tm);
      strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &tm);

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", date);
      printf("%lu ", s.st_nlink);
      printf("%s ", ConvierteModo3(s.st_mode));
      printf("%d ", s.st_uid);
      printf("%d ", s.st_gid);

      printf("\n");
    }
  }

  if(strcmp(arg[0], "-link") == 0){
         
    for(i = 1; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      p = readlink(arg[i], path, sizeof(path) - 1);
      
      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", path);
      
      printf("\n");
    }
  }
}

void list(char *arg[])
{
  int i;
  char date[MAXDATE];
  char path[MAXPATH];
  struct stat s;
  struct tm tm;
  ssize_t p;


  if(strcmp(arg[0], "-acc") != 0  && strcmp(arg[0], "-long") != 0){
         
    for(i = 0; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);

      printf("\n");
    }
  }
  
  if(strcmp(arg[0], "-acc") == 0){
         
    for(i = 1; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      localtime_r(&s.st_atime, &tm);
      strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &tm);

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", date);
      
      printf("\n");
    }
  }
   
  if(strcmp(arg[0], "-long") == 0){
         
    for(i = 1; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      localtime_r(&s.st_ctime, &tm);
      strftime(date, sizeof(date), "%Y-%m-%d %H:%M:%S", &tm);

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", date);
      printf("%lu ", s.st_nlink);
      printf("%s ", ConvierteModo3(s.st_mode));
      printf("%d ", s.st_uid);
      printf("%d ", s.st_gid);

      printf("\n");
    }
  }

  if(strcmp(arg[0], "-link") == 0){
         
    for(i = 1; arg[i] != NULL; i++){

      if(lstat(arg[i], &s) == -1){
        perror("Error al obtener tipo\n");
      }

      p = readlink(arg[i], path, sizeof(path) - 1);
      
      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", path);
      
      printf("\n");
    }
  }
}

/**************************SHELL**************************/

static struct COMANDO C[]={ /*no declaro dimension, que la coge de la inicializacion*/
   {"fin",Cmd_fin},
   {"exit",Cmd_fin},
   {"bye",Cmd_fin},
   {"quit", Cmd_fin},
   {"date", Cmd_date},
   {"pid",Cmd_pid},
   {"pwd", Cmd_pwd},
   {"chdir",Cmd_chdir},
   {"authors",Cmd_authors},
   {"sysinfo", Cmd_sysinfo},
   {"help", Cmd_help},
   {"chdir", Cmd_chdir},
   {"exec",Cmd_exec},
   {"pplano",Cmd_pplano},
   {"splano",Cmd_splano},
   {"open", Cmd_open},
   {"close", Cmd_close},
   {"listopen", Cmd_listopen},
   {"dup", Cmd_dup},
   {"lseek", Cmd_lseek},
   {"readstr", Cmd_readstr},
   {"writestr", Cmd_writestr},
   {"makefile", Cmd_makefile},
   {"makedir", Cmd_makedir},
   {"delete", Cmd_delete},
   {"deltree", Cmd_deltree},
   {"listfile", Cmd_listfile},
   {NULL,NULL},             /*NULL marca el final del array*/
  };

void DecidirComando (char *tr[])
{
  int i;

  if (tr[0] == NULL) return; /*por si luego quito lode TrocearCadena ==0*/
  for (i = 0; C[i].nombre != NULL; i++) /*recorro el array hasta el NULL*/
    if (!strcmp(C[i].nombre, tr[0])){
        (*C[i].funcion)(tr + 1);
	    return; /*si es algun cmd_ ya no miro mas*/
    }
  Proceso(tr,0); /*si no es un comando, es un  ejecutable externo*/
}

int TrocearCadena(char * cadena, char * trozos[])
{
  int i = 1;
  if ((trozos[0] = strtok(cadena," \n\t")) == NULL)
      return 0;
  while ((trozos[i] = strtok(NULL," \n\t")) != NULL)
     i++;
  return i;
}	

void ProcesarEntrada(char * entrada)
{
   char *tr[MAXENTRADA/2];
   if (TrocearCadena(entrada,tr) == 0) /*no hay nada*/
	return;
   DecidirComando(tr);
}



int main(int argc, char *argv[], char *ent[])
{
   char entrada[MAXENTRADA];

   while (1){
      printf ("-> ");
      fgets(entrada, MAXENTRADA, stdin);
      ProcesarEntrada(entrada);
   }
}

