/*Jose Manuel Iglesias Díaz  Daniel Marrero Sánchez*/
/*jose.iglesias.diaz@udc.es  daniel.marrero.sanchez@udc.es*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ftw.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>
#include <sys/mman.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXENTRADA 2048
#define MAXNOMBRE 1024
#define MAXLIST 20
#define MAXMEMLIST 30
#define MAXDATE 64
#define MAXPATH 1024
#define TAMANO 2048

struct COMANDO
{
  char *nombre;
  void (*funcion)(char **);
};

struct HELP
{
  char *nombre;
  char *descripcion;
};

struct FILE
{
  char *nombre;
  int df;
};

struct MEMBLOCK
{
  void *address;
  int size;
  char *fecha;
  char *tipo;
  key_t cl;
  char *fileName;
  int dfmf;
};

static struct FILE fileList[MAXLIST];

static struct MEMBLOCK memblockList[MAXMEMLIST];

void *CadenatoPointer(char *s)
{
  void *p;
  sscanf(s, "%p", &p);
  return p;
}

void MostrarDirActual()
{
  char dir[MAXNOMBRE];

  if (getcwd(dir, MAXNOMBRE) == NULL)
    perror("Imposible obtener directorio");
  else
    printf("%s\n", dir);
}

int ComprobarSegundoPlano(char *tr[])
{
  int i;
  for (i = 0; tr[i] != NULL; i++)
    if (!strcmp(tr[i], "&"))
    {               /*& indica segund plano*/
      tr[i] = NULL; /*es el ultimo argumento*/
      return i;     /*si solo hay un & no se ejecuta nada en pplano*/
    }
  return 0;
}

void Proceso(char *tr[], int splano)
{
  pid_t pid;
  void Cmd_exec(char **);
  int background = splano || ComprobarSegundoPlano(tr);
  if ((pid = fork()) == -1)
  {
    perror("Imposible crear proceso");
    return;
  }
  if (pid == 0)
  { /*proceso hijo*/
    Cmd_exec(tr);
    exit(255); /*por si falla exec*/
  }
  if (!background)
    waitpid(pid, NULL, 0);
}

int EsDirectorio(char *dir)
{ /*para saber si algo es directorio o no*/
  struct stat s;

  if (lstat(dir, &s) == -1)
  { /*si no puedo acceder: para mi no es directorio*/
    return 0;
  }

  return (S_ISDIR(s.st_mode));
}

char LetraTF(mode_t m)
{
  switch (m & S_IFMT)
  { /*and bit a bit con los bits de formato,0170000 */
  case S_IFSOCK:
    return 's'; /*socket */
  case S_IFLNK:
    return 'l'; /*symbolic link*/
  case S_IFREG:
    return '-'; /* fichero normal*/
  case S_IFBLK:
    return 'b'; /*block device*/
  case S_IFDIR:
    return 'd'; /*directorio */
  case S_IFCHR:
    return 'c'; /*char device*/
  case S_IFIFO:
    return 'p'; /*pipe*/
  default:
    return '?'; /*desconocido, no deberia aparecer*/
  }
}

char *ConvierteModo3(mode_t m)
{
  char *permisos;

  if ((permisos = (char *)malloc(12)) == NULL)
    return NULL;
  strcpy(permisos, "---------- ");

  permisos[0] = LetraTF(m);
  if (m & S_IRUSR)
    permisos[1] = 'r'; /*propietario*/
  if (m & S_IWUSR)
    permisos[2] = 'w';
  if (m & S_IXUSR)
    permisos[3] = 'x';
  if (m & S_IRGRP)
    permisos[4] = 'r'; /*grupo*/
  if (m & S_IWGRP)
    permisos[5] = 'w';
  if (m & S_IXGRP)
    permisos[6] = 'x';
  if (m & S_IROTH)
    permisos[7] = 'r'; /*resto*/
  if (m & S_IWOTH)
    permisos[8] = 'w';
  if (m & S_IXOTH)
    permisos[9] = 'x';
  if (m & S_ISUID)
    permisos[3] = 's'; /*setuid, setgid y stickybit*/
  if (m & S_ISGID)
    permisos[6] = 's';
  if (m & S_ISVTX)
    permisos[9] = 't';

  return permisos;
}

/*********************************************/
/************* COMANDOS DEL SHELL **************/
/*********************************************/

void Cmd_fin(char *arg[]) /*todos los cmd_ comparten prototipo*/
{                         /*reciben los mismos parametros aunque no los usen*/
  exit(0);
}

void Cmd_authors(char *arg[])
{
  if (arg[0] == NULL)
  {
    printf("Jose Manuel Iglesias Díaz y Daniel Marrero Sánchez\n ");
    printf("jose.iglesias.diaz@udc.es  daniel.marrero.sanchez@udc.es\n");
  }
  else if (strcmp(arg[0], "-l") == 0)
  {
    printf("jose.iglesias.diaz@udc.es  daniel.marrero.sanchez@udc.es\n ");
  }
  else if (strcmp(arg[0], "-n") == 0)
  {
    printf("Jose Manuel Iglesias Díaz y Daniel Marrero Sánchez\n");
  }
}

void Cmd_date(char *arg[])
{
  if (arg[0] == NULL)
  {
    time_t ahora = time(NULL);
    printf("Fecha: %.10s\n", ctime(&ahora));
    printf("Hora: %.8s\n", ctime(&ahora) + 11);
  }
  else if (strcmp(arg[0], "-d") == 0)
  {
    time_t ahora = time(NULL);
    printf("Fecha: %.10s\n", ctime(&ahora));
  }
  else if (strcmp(arg[0], "-t") == 0)
  {
    time_t ahora = time(NULL);
    printf("Hora: %.8s\n", ctime(&ahora) + 11);
  }
}

void Cmd_exec(char *arg[])
{
  if (execvp(arg[0], arg) == -1)
    perror("Imposible ejecutar");
}

void Cmd_splano(char *arg[])
{
  Proceso(arg, 1);
}
void Cmd_pplano(char *arg[])
{
  Proceso(arg, 0);
}

void Cmd_pwd(char *arg[])
{
  MostrarDirActual();
}

void Cmd_pid(char *arg[])
{
  if (arg[0] == NULL)
    printf("El pid del proceso es %d\n", (int)getpid());
  else if (!strcmp(arg[0], "-p"))
    printf("El pid del proceso padre es %d\n", (int)getppid());
}

void Cmd_sysinfo(char *arg[])
{
  system("uname -a");
}

void Cmd_dup(char *arg[])
{
  int df, newdf;
  if (arg[0] == NULL)
  {
    printf("Uso: dup <descriptor>\n");
    return;
  }
  df = atoi(arg[0]);

  newdf = dup(df);

  if (newdf == -1)
  {
    perror("Error al duplicar descriptor");
  }
  else
  {
    printf("Descriptor duplicado: %d\n", newdf);
  }
}

void Cmd_lseek(char *arg[])
{
  int df, origen;
  off_t offset, resultado;
  if (arg[0] == NULL || arg[1] == NULL || arg[2] == NULL)
  {
    printf("Uso: lseek <descriptor><offset><SEEK_SET|SEEK_CUR|SEEK_END>\n");
    return;
  }
  df = atoi(arg[0]);
  offset = atol(arg[1]);

  if (strcmp(arg[2], "SEEK_SET") == 0)
  {
    origen = SEEK_SET;
  }
  else if (strcmp(arg[2], "SEEK_CUR") == 0)
  {
    origen = SEEK_CUR;
  }
  else if (strcmp(arg[2], "SEEK_END") == 0)
  {
    origen = SEEK_END;
  }
  else
  {
    printf("Origen no valido");
    return;
  }

  resultado = lseek(df, offset, origen);

  if (resultado == (off_t)-1)
  {
    perror("Error al mover el puntero del descriptor");
  }
  else
  {
    printf("Nueva posicion: %ld\n", (long)resultado);
  }
}

void Cmd_makefile(char *arg[])
{
  FILE *file;

  if (arg[0] == NULL)
  {
    printf("Falta el nombre del archivo\n");
    return;
  }

  file = fopen(arg[0], "wx");

  if (file == NULL)
  {
    perror("Error al crear el archivo");
    return;
  }
  fclose(file);
}

void Cmd_readstr(char *arg[])
{
  int df, cont, leidos;
  char cadena[1024];

  if (arg[0] == NULL || arg[1] == NULL)
  {
    printf("Uso: readstr <df> <cont>\n");
    return;
  }

  df = atoi(arg[0]);
  cont = atoi(arg[1]);

  if (cont > 1023 || cont <= 0)
  {
    printf("Cantidad no valida\n");
    return;
  }

  leidos = read(df, cadena, cont);

  if (leidos == -1)
  {
    perror("Error al leer");
    return;
  }

  cadena[leidos] = '\0';
  printf("%s\n", cadena);
}

void Cmd_writestr(char *arg[])
{
  int df, escritos;

  if (arg[0] == NULL || arg[1] == NULL)
  {
    printf("Uso: writestr <df> <cadena>\n");
    return;
  }

  df = atoi(arg[0]);

  escritos = write(df, arg[1], strlen(arg[1]));

  if (escritos == -1)
  {
    perror("Error al escribir");
    return;
  }

  printf("Bytes escritos: %d\n", escritos);
}

void Cmd_help(char *arg[])
{

  static struct HELP C[] = {
      {"fin", "Finaliza sesión en el Shell"},
      {"exit", "Finaliza sesión en el Shell"},
      {"bye", "Finaliza sesión en el Shell"},
      {"quit", "Finaliza la sesión en el Shell"},
      {"date", "MUestra la fecha actual. -d sólo para la fecha y -t sólo para la hora"},
      {"pid", "Muestra el valor actual del pid. -p para el pid del proceso padre"},
      {"pwd", "Muestra el path del directorio actual"},
      {"chdir", "Cambia de directorio al seleccionado"},
      {"authors", "Muestra los nombre y logins de los autores de la práctica. -l sólo para los logins y -n sólo para los nombres"},
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
      {"allocate", "allocate -malloc|-shared|-createshared|-mmap]..., reserva un bloque de memoria, sin parámetros muestra la lista de bloques reservados.\n-malloc siz: asigna un bloque de tamaño siz\n-createshared: crea y asigna un bloque de clave cl y tamaño siz\n-shared: asigna un bloque de memoria compartida de clave (preexistente cl)\n-mmap: mapea el fichero fil con permisos perm"},
      {"deallocate", "deallocate -malloc|-shared|-delkey|-mmap|-addr]..., desasigna un bloque de memoria, sin parámetros muestra la lista de bloques reservados.\n-malloc siz: desasigna un bloque malloc de tamaño siz\n-shared: desasigna un bloque de clave cl\n-delkey: elimina una clave cl del sistema\n-mmap: desmapea el fichero fil\n-addr: desasigna el bloque de memoria de dirección addr"},
      {"read", "read df addr cont, transfiere cont bytes a la dirección addr del fichero descrito por df"},
      {"write", "write df addr cont, transfiere cont bytes de la dirección addr al fichero descrito por df"},
      {"recurse", "recurse n, ejecuta la función recursiva n veces"}};

  if (arg[0] == NULL)
  {
    for (int i = 0; C[i].nombre != NULL; i++)
    { /*recorro el array hasta el NULL*/
      printf("%s\n", C[i].nombre);
    }
  }

  else
  {
    for (int i = 0; C[i].nombre != NULL; i++)
    {
      if (strcmp(arg[0], C[i].nombre) == 0)
      {
        printf("%s\n", C[i].descripcion);
      }
    }
  }
}

void Cmd_chdir(char *arg[])
{
  if (arg[0] == NULL)
  {
    MostrarDirActual();
  }

  else if (chdir(arg[0]) == -1)
  {
    perror("Imposible cambiar directorio");
  }
}

void Cmd_open(char *arg[])
{
  int i, df, mode = 0;

  if (arg[0] == NULL)
  { /*no hay parametro*/
    printf("descriptor: 0, offset: (  )-> entrada estandar O_RDWR\n");
    printf("descriptor: 1, offset: (  )-> salida estandar O_RDWR\n");
    printf("descriptor: 2, offset: (  )-> error estandar O_RDWR\n");

    for (i = 3; i < MAXLIST; i++)
    {

      if (fileList[i].nombre == NULL)
      {
        printf("descriptor: %d, offset: (  )-> no usado", i);
      }

      else
      {
        printf("descriptor: %d, offset: (  )-> %s", fileList[i].df, fileList[i].nombre);
      }

      printf("\n");
    }
    return;
  }

  for (i = 1; arg[i] != NULL; i++)
  {
    if (!strcmp(arg[i], "cr"))
      mode |= O_CREAT;
    else if (!strcmp(arg[i], "ex"))
      mode |= O_EXCL;
    else if (!strcmp(arg[i], "ro"))
      mode |= O_RDONLY;
    else if (!strcmp(arg[i], "wo"))
      mode |= O_WRONLY;
    else if (!strcmp(arg[i], "rw"))
      mode |= O_RDWR;
    else if (!strcmp(arg[i], "ap"))
      mode |= O_APPEND;
    else if (!strcmp(arg[i], "tr"))
      mode |= O_TRUNC;
    else
      break;
  }

  if ((df = open(arg[0], mode, 0777)) == -1)
  {
    perror("Imposible abrir fichero");
  }

  else
  {

    fileList[df].df = df;
    fileList[df].nombre = strdup(arg[0]);

    printf("Anadida entrada %d a la tabla ficheros abiertos\n", df);
  }
}

void Cmd_close(char *arg[])
{
  int df, i;

  if (arg[0] == NULL || (df = atoi(arg[0])) < 0)
  {

    printf("descriptor: 0, offset: (  )-> entrada estandar O_RDWR\n");
    printf("descriptor: 1, offset: (  )-> salida estandar O_RDWR\n");
    printf("descriptor: 2, offset: (  )-> error estandar O_RDWR\n");

    for (i = 3; i < MAXLIST; i++)
    {

      if (fileList[i].nombre == NULL)
      {
        printf("descriptor: %d, offset: (  )-> no usado", i);
      }

      else
      {
        printf("descriptor: %d, offset: (  )-> %s", fileList[i].df, fileList[i].nombre);
      }

      printf("\n");
    }

    return;
  }

  if (close(df) == -1)
  {
    perror("Imposible cerrar descriptor\n");
  }

  else
  {
    free(fileList[df].nombre);
    fileList[df].nombre = NULL;

    printf("Eliminada entrada %d a la tabla ficheros abiertos\n", df);
  }
}

void Cmd_listopen(char *arg[])
{

  int i;

  printf("descriptor: 0, offset: (  )-> entrada estandar O_RDWR\n");
  printf("descriptor: 1, offset: (  )-> salida estandar O_RDWR\n");
  printf("descriptor: 2, offset: (  )-> error estandar O_RDWR\n");

  for (i = 3; i < MAXLIST; i++)
  {

    if (fileList[i].nombre == NULL)
    {
      printf("descriptor: %d, offset: (  )-> no usado", i);
    }

    else
    {
      printf("descriptor: %d, offset: (  )-> %s", fileList[i].df, fileList[i].nombre);
    }

    printf("\n");
  }
  return;
}

void Cmd_makedir(char *arg[])
{
  int i, mode = 0;

  for (i = 1; arg[i] != NULL; i++)
    if (!strcmp(arg[i], "cr"))
      mode |= O_CREAT;
    else if (!strcmp(arg[i], "ex"))
      mode |= O_EXCL;
    else if (!strcmp(arg[i], "ro"))
      mode |= O_RDONLY;
    else if (!strcmp(arg[i], "wo"))
      mode |= O_WRONLY;
    else if (!strcmp(arg[i], "rw"))
      mode |= O_RDWR;
    else if (!strcmp(arg[i], "ap"))
      mode |= O_APPEND;
    else if (!strcmp(arg[i], "tr"))
      mode |= O_TRUNC;
    else
      break;

  if (mkdir(arg[0], mode) == -1)
  {
    perror("Imposible crear directorio\n");
  }
}

void Cmd_delete(char *arg[])
{

  struct stat s;

  for (int i = 0; arg[i] != NULL; i++)
  {

    if (lstat(arg[i], &s) == -1)
    {
      perror("Imposible identificar objeto\n");
    }

    char tipo = LetraTF(s.st_mode);

    switch (tipo)
    {
    case '-':
      if (remove(arg[i]) == -1)
      {
        perror("Imposible eliminar archivo\n");
      }
      break;

    case 'l':
      if (unlink(arg[i]) == -1)
      {
        perror("Imposible eliminar link\n");
      }
      break;

    case 'd':
      if (rmdir(arg[i]) == -1)
      {
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

  if (lstat(p, &s) == -1)
  {
    perror(p);
    return;
  }

  DIR *d = opendir(p);
  if (d == NULL)
  {
    perror("Imposible abrir directorio");
    return;
  }

  while ((e = readdir(d)) != NULL)
  {
    borrarRecursivo(hijo);
  }

  closedir(d);

  if (rmdir(p) == -1)
  {
    perror("No ha sido posible borrar");
  }
}

void Cmd_deltree(char *arg[])
{

  struct stat s;

  for (int i = 0; arg[i] != NULL; i++)
  {

    if (lstat(arg[i], &s) == -1)
    {
      perror("Imposible identificar objeto\n");
    }

    char tipo = LetraTF(s.st_mode);

    switch (tipo)
    {
    case '-':
      if (remove(arg[i]) == -1)
      {
        perror("Imposible eliminar archivo\n");
      }
      break;

    case 'l':
      if (unlink(arg[i]) == -1)
      {
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

  if (strcmp(arg[0], "-acc") != 0 && strcmp(arg[0], "-long") != 0)
  {

    for (i = 0; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
        perror("Error al obtener tipo\n");
      }

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);

      printf("\n");
    }
  }

  if (strcmp(arg[0], "-acc") == 0)
  {

    for (i = 1; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
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

  if (strcmp(arg[0], "-long") == 0)
  {

    for (i = 1; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
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

  if (strcmp(arg[0], "-link") == 0)
  {

    for (i = 1; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
        perror("Error al obtener tipo\n");
      }

      p = readlink(arg[i], path, sizeof(path) - 1);

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", path);
      printf("%ld", p);

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

  if (strcmp(arg[0], "-acc") != 0 && strcmp(arg[0], "-long") != 0)
  {

    for (i = 0; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
        perror("Error al obtener tipo\n");
      }

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);

      printf("\n");
    }
  }

  if (strcmp(arg[0], "-acc") == 0)
  {

    for (i = 1; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
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

  if (strcmp(arg[0], "-long") == 0)
  {

    for (i = 1; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
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

  if (strcmp(arg[0], "-link") == 0)
  {

    for (i = 1; arg[i] != NULL; i++)
    {

      if (lstat(arg[i], &s) == -1)
      {
        perror("Error al obtener tipo\n");
      }

      p = readlink(arg[i], path, sizeof(path) - 1);

      printf("%s ", arg[i]);
      printf("%ld ", s.st_size);
      printf("%s ", path);
      printf("%ld", p);

      printf("\n");
    }
  }
}

void ImprimirLista(struct MEMBLOCK memblockList[])
{
  printf("****** Lista de bloques asignados para el proceso %d ******\n", getpid());

  for (int i = 0; memblockList[i].address != NULL && i < MAXMEMLIST; i++)
  {
    if (strcmp(memblockList[i].tipo, "malloc") == 0)
    {
      printf("\t%p\t\t%d %.12s %s\n", memblockList[i].address, memblockList[i].size, memblockList[i].fecha, memblockList[i].tipo);
    }
    else if (strcmp(memblockList[i].tipo, "mmap") == 0)
    {
      printf("\t%p\t\t%d %.12s %s\t(descriptor %d)\n", memblockList[i].address, memblockList[i].size, memblockList[i].fecha, memblockList[i].fileName, memblockList[i].dfmf);
    }
    else if (strcmp(memblockList[i].tipo, "shared") == 0)
    {
      printf("\t%p\t\t%d %.12s %s\t(key %d)\n", memblockList[i].address, memblockList[i].size, memblockList[i].fecha, memblockList[i].tipo, memblockList[i].cl);
    }
  }
}

void ImprimirListaMalloc(struct MEMBLOCK memblockList[])
{
  printf("****** Lista de bloques malloc asignados para el proceso %d ******\n", getpid());

  for (int i = 0; memblockList[i].address != NULL && i < MAXMEMLIST; i++)
  {
    if (strcmp(memblockList[i].tipo, "malloc") == 0)
    {
      printf("\t%p\t\t%d %.12s %s\n", memblockList[i].address, memblockList[i].size, memblockList[i].fecha, memblockList[i].tipo);
    }
  }
}

void ImprimirListaMmap(struct MEMBLOCK memblockList[])
{
  printf("****** Lista de bloques mmap asignados para el proceso %d ******\n", getpid());

  for (int i = 0; memblockList[i].address != NULL && i < MAXMEMLIST; i++)
  {
    if (strcmp(memblockList[i].tipo, "mmap") == 0)
    {
      printf("\t%p\t\t%d %.12s %s\t(descriptor %d)\n", memblockList[i].address, memblockList[i].size, memblockList[i].fecha, memblockList[i].fileName, memblockList[i].dfmf);
    }
  }
}

void ImprimirListaShared(struct MEMBLOCK memblockList[])
{
  printf("****** Lista de bloques shared asignados para el proceso %d ******\n", getpid());

  for (int i = 0; memblockList[i].address != NULL && i < MAXMEMLIST; i++)
  {
    if (strcmp(memblockList[i].tipo, "shared") == 0)
    {
      printf("\t%p\t\t%d %.12s %s\t(key %d)\n", memblockList[i].address, memblockList[i].size, memblockList[i].fecha, memblockList[i].tipo, memblockList[i].cl);
    }
  }
}

void insertarNodoMalloc(struct MEMBLOCK memblockList[], void *p, int bytes, int pos)
{
  time_t date = time(NULL);

  memblockList[pos].address = p;
  memblockList[pos].size = bytes;
  memblockList[pos].fecha = strdup(ctime(&date) + 4);
  memblockList[pos].tipo = strdup("malloc");
}

void InsertarNodoMmap(struct MEMBLOCK memblockList[], void *p, off_t s, int df, char *fichero, int pos)
{
  time_t date = time(NULL);

  memblockList[pos].address = p;
  memblockList[pos].size = s;
  memblockList[pos].fecha = strdup(ctime(&date) + 4);
  memblockList[pos].fileName = strdup(fichero);
  memblockList[pos].dfmf = df;
  memblockList[pos].tipo = strdup("mmap");
}

void InsertarNodoShared(struct MEMBLOCK memblockList[], void *p, size_t tam, key_t key, int pos)
{
  time_t date = time(NULL);

  memblockList[pos].address = p;
  memblockList[pos].size = tam;
  memblockList[pos].fecha = strdup(ctime(&date) + 4);
  memblockList[pos].tipo = strdup("shared");
  memblockList[pos].cl = key;
}

void *MapearFichero(char *fichero, int protection, int pos)
{
  int df, map = MAP_PRIVATE, modo = O_RDONLY;
  struct stat s;
  void *p;

  if (protection & PROT_WRITE)
    modo = O_RDWR;
  if (stat(fichero, &s) == -1 || (df = open(fichero, modo)) == -1)
    return NULL;
  if ((p = mmap(NULL, s.st_size, protection, map, df, 0)) == MAP_FAILED)
    return NULL;

  fileList[df].df = df;
  fileList[df].nombre = strdup(fichero);

  InsertarNodoMmap(memblockList, p, s.st_size, df, fichero, pos);
  return p;
}

void *ObtenerMemoriaShmget(key_t clave, size_t tam, int pos)
{
  void *p;
  int aux, id, flags = 0777; /*los 9 bits menos significativos de los flags:permisos*/
  struct shmid_ds s;

  if (tam)                                /*IMPORTANTE: tam distito de 0 indica crear */
    flags = flags | IPC_CREAT | IPC_EXCL; /*cuando no es crear pasamos de tamano 0*/
  if (clave == IPC_PRIVATE)               /*no nos vale*/
  {
    errno = EINVAL;
    return NULL;
  }
  if ((id = shmget(clave, tam, flags)) == -1)
    return (NULL);
  if ((p = shmat(id, NULL, 0)) == (void *)-1)
  {
    aux = errno;
    if (tam)
      shmctl(id, IPC_RMID, NULL);
    errno = aux;
    return (NULL);
  }
  shmctl(id, IPC_STAT, &s);                                     /* si no es crear, necesitamos el tamano, que es s.shm_segsz*/
  InsertarNodoShared(memblockList, p, s.shm_segsz, clave, pos); /* Guardar en la lista   InsertarNodoShared (&L, p, s.shm_segsz, clave); */

  return (p);
}

void do_AllocateMalloc(char *arg[])
{
  int pos, bytes;
  void *p;

  if (arg[1] == NULL)
  {
    ImprimirListaMalloc(memblockList);
  }

  bytes = atoi(arg[1]);

  if (arg[1] != NULL && (p = malloc(bytes)) != NULL)
  {
    printf("Asignados %d bytes en la dirección %p\n", bytes, p);

    for (pos = 0; memblockList[pos].address != NULL; pos++)
    {
    }
    insertarNodoMalloc(memblockList, p, bytes, pos);
  }
}

void do_AllocateMmap(char *arg[])
{
  char *perm;
  void *p;
  int protection = 0, pos;

  if (arg[1] == NULL)
  {
    ImprimirListaMmap(memblockList);
    return;
  }
  if ((perm = arg[2]) != NULL && strlen(perm) < 4)
  {
    if (strchr(perm, 'r') != NULL)
      protection |= PROT_READ;
    if (strchr(perm, 'w') != NULL)
      protection |= PROT_WRITE;
    if (strchr(perm, 'x') != NULL)
      protection |= PROT_EXEC;
  }
  for (pos = 0; memblockList[pos].address != NULL; pos++)
  {
  }
  if ((p = MapearFichero(arg[1], protection, pos)) == NULL)
    perror("Imposible mapear fichero");
  else
    printf("fichero %s mapeado en %p\n", arg[1], p);
}

void do_AllocateCreateShared(char *arg[])
{
  int pos;
  key_t cl;
  size_t tam;
  void *p;

  if (arg[1] == NULL)
  {
    ImprimirListaShared(memblockList);
    return;
  }

  cl = (key_t)strtoul(arg[1], NULL, 10);
  tam = (size_t)strtoul(arg[2], NULL, 10);
  if (tam == 0)
  {
    printf("No se asignan bloques de 0 bytes\n");
    return;
  }
  for (pos = 0; memblockList[pos].address != NULL; pos++)
  {
  }
  if ((p = ObtenerMemoriaShmget(cl, tam, pos)) != NULL)
    printf("Asignados %lu bytes en %p\n", (unsigned long)tam, p);
  else
    printf("Imposible asignar memoria compartida clave %lu:%s\n", (unsigned long)cl, strerror(errno));
}

void do_AllocateShared(char *arg[])
{
  int pos;
  key_t cl;
  void *p;

  if (arg[1] == NULL)
  {
    ImprimirListaShared(memblockList);
    return;
  }

  cl = (key_t)strtoul(arg[1], NULL, 10);

  for (pos = 0; memblockList[pos].address != NULL; pos++)
  {
  }
  if ((p = ObtenerMemoriaShmget(cl, 0, pos)) != NULL)
    printf("Asignada memoria compartida de clave %lu en %p\n", (unsigned long)cl, p);
  else
    printf("Imposible asignar memoria compartida clave %lu:%s\n", (unsigned long)cl, strerror(errno));
}

void Cmd_allocate(char *arg[])
{
  if (arg[0] == NULL)
  {
    ImprimirLista(memblockList);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-malloc") == 0)
  {
    do_AllocateMalloc(arg);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-mmap") == 0)
  {
    do_AllocateMmap(arg);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-createshared") == 0)
  {
    do_AllocateCreateShared(arg);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-shared") == 0)
  {
    do_AllocateShared(arg);
  }
}

void *DireccionNodoShared(struct MEMBLOCK memblockList[], key_t key)
{
  int i;

  for (i = 0; memblockList[i].address != NULL && memblockList[i].cl != key; i++)
  {
  }

  return memblockList[i].address;
}

void eliminarNodoMalloc(struct MEMBLOCK memblockList[], int pos)
{
  free(memblockList[pos].address);
  free(memblockList[pos].fecha);
  free(memblockList[pos].tipo);
}

void eliminarNodoMmap(struct MEMBLOCK memblockList[], int pos)
{
  int df = memblockList[pos].dfmf;

  if (close(df) == -1)
  {
    perror("Imposible cerrar descriptor\n");
  }
  else
  {
    free(fileList[df].nombre);
    fileList[df].nombre = NULL;

    free(memblockList[pos].fecha);
    free(memblockList[pos].fileName);
    free(memblockList[pos].tipo);
  }
}

void eliminarNodoDireccion(struct MEMBLOCK memblockList[], int pos)
{
  free(memblockList[pos].fecha);
  free(memblockList[pos].tipo);
}

void do_DeallocateMalloc(char *arg[])
{
  int i, j, bytes;

  if (arg[1] == NULL)
  {
    ImprimirListaMalloc(memblockList);
  }
  else
  {
    bytes = atoi(arg[1]);

    for (i = 0; memblockList[i].address != NULL; i++)
    {
      if ((memblockList[i].size == bytes) && (strcmp(memblockList[i].tipo, "malloc") == 0))
      {
        break;
      }
    }

    eliminarNodoMalloc(memblockList, i);

    for (j = i; memblockList[j].address != NULL; j++)
    {
      memblockList[j] = memblockList[j + 1];
    }
  }
}

void do_DeallocateMmap(char *arg[])
{
  int i, j;

  if (arg[1] == NULL)
  {
    ImprimirListaMmap(memblockList);
  }
  else
  {
    for (i = 0; memblockList[i].address != NULL; i++)
    {
      if (memblockList[i].fileName != NULL && strcmp(memblockList[i].fileName, arg[1]) == 0)
      {
        break;
      }
    }

    eliminarNodoMmap(memblockList, i);

    for (j = i; memblockList[j].address != NULL; j++)
    {
      memblockList[j] = memblockList[j + 1];
    }
  }
}

void do_DeallocateShared(char *arg[])
{
  int i, j;
  void *p;

  if (arg[1] == NULL)
  {
    ImprimirListaShared(memblockList);
  }
  else
  {
    key_t cl = atoi(arg[1]);

    if ((p = DireccionNodoShared(memblockList, cl)) == NULL)
    {
      printf("No hay bloque de esa clave mapeado en el proceso\n");
      return;
    }

    shmdt(p);

    for (i = 0; memblockList[i].address != NULL; i++)
    {
      if (memblockList[i].tipo != NULL && strcmp(memblockList[i].tipo, "shared") == 0 && memblockList[i].cl == cl)
      {
        break;
      }
    }

    eliminarNodoDireccion(memblockList, i);

    for (j = i; memblockList[j].address != NULL; j++)
    {
      memblockList[j] = memblockList[j + 1];
    }
  }
}

void do_SharedDelKey(char *arg[])
{
  key_t clave;
  int id;
  char *key = arg[1];

  if (key == NULL || (clave = (key_t)strtoul(key, NULL, 10)) == IPC_PRIVATE)
  {
    printf("      delkey necesita clave_valida\n");
    return;
  }
  if ((id = shmget(clave, 0, 0666)) == -1)
  {
    perror("shmget: imposible obtener memoria compartida");
    return;
  }
  if (shmctl(id, IPC_RMID, NULL) == -1)
    perror("shmctl: imposible eliminar memoria compartida\n");
}

void do_DeallocateAddr(char *arg[])
{
  int i, j;
  void *p;

  if (arg[0] == NULL)
  {
    ImprimirLista(memblockList);
  }
  else
  {
    p = CadenatoPointer(arg[0]);

    for (i = 0; memblockList[i].address != NULL && memblockList[i].address != p; i++)
    {
    }

    if (strcmp(memblockList[i].tipo, "malloc") == 0)
    {
      eliminarNodoMalloc(memblockList, i);
    }
    else if (strcmp(memblockList[i].tipo, "mmap") == 0)
    {
      eliminarNodoMmap(memblockList, i);
    }
    else if (strcmp(memblockList[i].tipo, "shared") == 0)
    {
      eliminarNodoDireccion(memblockList, i);
    }

    for (j = i; memblockList[j].address != NULL; j++)
    {
      memblockList[j] = memblockList[j + 1];
    }
  }
}

void Cmd_deallocate(char *arg[])
{
  if (arg[0] == NULL)
  {
    ImprimirLista(memblockList);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-malloc") == 0)
  {
    do_DeallocateMalloc(arg);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-mmap") == 0)
  {
    do_DeallocateMmap(arg);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-shared") == 0)
  {
    do_DeallocateShared(arg);
  }

  else if (arg[0] != NULL && strcmp(arg[0], "-delkey") == 0)
  {
    do_SharedDelKey(arg);
  }

  else
  {
    do_DeallocateAddr(arg);
  }
}

ssize_t LeerFichero(char *f, void *p, size_t cont)
{
  struct stat s;
  ssize_t n;
  int df, aux;

  if (stat(f, &s) == -1 || (df = open(f, O_RDONLY)) == -1)
    return -1;
  if (cont == -1) /* si pasamos -1 como bytes a leer lo leemos entero*/
    cont = s.st_size;
  if ((n = read(df, p, cont)) == -1)
  {
    aux = errno;
    close(df);
    errno = aux;

    printf("Error al leer del descriptor %d a la direccion %p. Bad address.\n", df, p);
    return -1;
  }
  close(df);

  printf("Leidos %lu bytes de la direccion %p\n", n, p);
  return n;
}

void Cmd_read(char *arg[])
{
  char *f = strdup(arg[0]);
  void *p = CadenatoPointer(arg[1]);
  size_t n = atoi(arg[2]);

  LeerFichero(f, p, n);
}

ssize_t EscribirFichero(char *f, void *p, size_t cont)
{
  struct stat s;
  ssize_t n;
  int df, aux;

  if (stat(f, &s) == -1 || (df = open(f, O_WRONLY)) == -1)
    return -1;

  if ((n = write(df, p, cont)) == -1)
  {
    aux = errno;
    close(df);
    errno = aux;

    printf("Error al escribir de la direccion %p al descriptor %d\n", p, df);
    return -1;
  }
  close(df);

  printf("Escritos %lu bytes de la direccion %p\n", n, p);
  return n;
}

void Cmd_write(char *arg[])
{
  char *f = strdup(arg[0]);
  void *p = CadenatoPointer(arg[1]);
  size_t n = atoi(arg[2]);

  EscribirFichero(f, p, n);
}

void Recursiva(int n)
{
  char automatico[TAMANO];
  static char estatico[TAMANO];

  printf("parametro:%3d(%p) array %p, arr estatico %p\n", n, &n, automatico, estatico);

  if (n > 0)
    Recursiva(n - 1);
}

void Cmd_recurse(char *arg[])
{
  int n = atoi(arg[0]);

  Recursiva(n);
}

/**************************SHELL**************************/

static struct COMANDO C[] = {
    /*no declaro dimension, que la coge de la inicializacion*/
    {"fin", Cmd_fin},
    {"exit", Cmd_fin},
    {"bye", Cmd_fin},
    {"quit", Cmd_fin},
    {"date", Cmd_date},
    {"pid", Cmd_pid},
    {"pwd", Cmd_pwd},
    {"chdir", Cmd_chdir},
    {"authors", Cmd_authors},
    {"sysinfo", Cmd_sysinfo},
    {"help", Cmd_help},
    {"chdir", Cmd_chdir},
    {"exec", Cmd_exec},
    {"pplano", Cmd_pplano},
    {"splano", Cmd_splano},
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
    {"allocate", Cmd_allocate},
    {"deallocate", Cmd_deallocate},
    {"read", Cmd_read},
    {"write", Cmd_write},
    {"recurse", Cmd_recurse},
    {NULL, NULL}, /*NULL marca el final del array*/
};

void DecidirComando(char *tr[])
{
  int i;

  if (tr[0] == NULL)
    return;                             /*por si luego quito lode TrocearCadena ==0*/
  for (i = 0; C[i].nombre != NULL; i++) /*recorro el array hasta el NULL*/
    if (!strcmp(C[i].nombre, tr[0]))
    {
      (*C[i].funcion)(tr + 1);
      return; /*si es algun cmd_ ya no miro mas*/
    }
  Proceso(tr, 0); /*si no es un comando, es un  ejecutable externo*/
}

int TrocearCadena(char *cadena, char *trozos[])
{
  int i = 1;
  if ((trozos[0] = strtok(cadena, " \n\t")) == NULL)
    return 0;
  while ((trozos[i] = strtok(NULL, " \n\t")) != NULL)
    i++;
  return i;
}

void ProcesarEntrada(char *entrada)
{
  char *tr[MAXENTRADA / 2];
  if (TrocearCadena(entrada, tr) == 0) /*no hay nada*/
    return;
  DecidirComando(tr);
}

int main(int argc, char *argv[], char *ent[])
{
  char entrada[MAXENTRADA];

  while (1)
  {
    printf("-> ");
    fgets(entrada, MAXENTRADA, stdin);
    ProcesarEntrada(entrada);
  }
}
