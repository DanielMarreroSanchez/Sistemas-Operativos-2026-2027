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
#include <errno.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define MAXENTRADA 2048
#define MAXNOMBRE 1024
#define MAXLIST 20
#define MAXDATE 64
#define MAXPATH 1024

#define TAMANO 2048

//Crear: ImprimirListaShared, DireccionNodoShared, EliminarNodoDireccion, InsertarNodoShared, ImprimirListaMmap
/* Estructura para la lista de Memoria Compartida */

struct NodoShared {
    void *p;
    size_t tam;
    key_t clave;
    struct NodoShared *sig;
};

/* Estructura para la lista de Mmap */
struct NodoMmap {
    void *p;
    size_t tam;
    int df;
    char fichero[MAXPATH];
    struct NodoMmap *sig;
};

/* Listas globales del programa */
static struct NodoShared *LShared = NULL;
static struct NodoMmap *LMmap = NULL;

void InsertarNodoShared (void **L, void *p, size_t tam, key_t clave) {
  struct NodoShared *nuevo = (struct NodoShared *) malloc (sizeof(struct NodoShared));
  if (nuevo == NULL) {
    perror ("Imposible asignar memoria para el nodo");
    return;
  }
  nuevo->p = p;
  nuevo->tam = tam;
  nuevo->clave = clave;
  nuevo->sig = (struct NodoShared *) (*L);
  *L = nuevo;
}

void ImprimirListaShared (void **L)
{
  struct NodoShared *actual = (struct NodoShared *) (*L);

  printf ("--- Lista de memoria compartida (shm) ---\n");
  while (actual != NULL) {
    printf ("%p: tam:%lu clave:%lu\n", actual->p, (unsigned long) actual->tam, (unsigned long) actual->clave);
    actual = actual->sig;
  }
}
void * DireccionNodoShared (void **L, key_t clave)
{
  struct NodoShared *actual = (struct NodoShared *) (*L);

  while (actual != NULL) {
    if (actual->clave == clave)
      return actual->p;
    actual = actual->sig;
  }
  return NULL;
}

int EliminarNodoDireccion (void **L, void *p)
{
  struct NodoShared *actual = (struct NodoShared *) (*L);
  struct NodoShared *anterior = NULL;

  while (actual != NULL) {
    if (actual->p == p) {
      if (anterior == NULL)
        *L = actual->sig; /* Era el primer nodo */
      else
        anterior->sig = actual->sig;

      free (actual);
      return 0; /* Exito */
    }
    anterior = actual;
    actual = actual->sig;
  }
  return -1; /* No encontrado */
}

void InsertarNodoMmap (void **L, void *p, size_t tam, int df, char *fichero) {
  struct NodoMmap *nuevo = (struct NodoMmap *) malloc (sizeof(struct NodoMmap));
  if (nuevo == NULL) {
    perror ("Imposible asignar memoria para el nodo");
    return;
  }
  nuevo->p = p;
  nuevo->tam = tam;
  nuevo->df = df;
  strncpy (nuevo->fichero, fichero, MAXPATH - 1);
  nuevo->fichero[MAXPATH - 1] = '\0';
  nuevo->sig = (struct NodoMmap *) (*L);
  *L = nuevo;
}

/* Imprime la lista de ficheros mapeados */
void ImprimirListaMmap (void **L)
{
  struct NodoMmap *actual = (struct NodoMmap *) (*L);

  printf ("--- Lista de ficheros mapeados (mmap) ---\n");
  while (actual != NULL) {
    printf ("%p: tam:%lu df:%d descriptor:%s\n", actual->p, (unsigned long) actual->tam, actual->df, actual->fichero);
    actual = actual->sig;
  }
}

void Recursiva (int n)
{
  char automatico[TAMANO];
  static char estatico[TAMANO];

  printf ("parametro:%3d(%p) array %p, arr estatico %p\n",n,&n,automatico, estatico);

  if (n>0)
    Recursiva(n-1);
}


void LlenarMemoria (void *p, size_t cont, unsigned char byte)
{
  unsigned char *arr=(unsigned char *) p;
  size_t i;

  for (i=0; i<cont;i++)
		arr[i]=byte;
}

void * ObtenerMemoriaShmget (key_t clave, size_t tam)
{
    void * p;
    int aux,id,flags=0777; /*los 9 bits menos significativos de los flags:permisos*/
    struct shmid_ds s;

    if (tam)     /*IMPORTANTE: tam distito de 0 indica crear */
        flags=flags | IPC_CREAT | IPC_EXCL; /*cuando no es crear pasamos de tamano 0*/
    if (clave==IPC_PRIVATE)  /*no nos vale*/
        {errno=EINVAL; return NULL;}
    if ((id=shmget(clave, tam, flags))==-1)
        return (NULL);
    if ((p=shmat(id,NULL,0))==(void*) -1){
        aux=errno;
        if (tam)
             shmctl(id,IPC_RMID,NULL);
        errno=aux;
        return (NULL);
    }
    shmctl (id,IPC_STAT,&s); /* si no es crear, necesitamos el tamano, que es s.shm_segsz*/
    InsertarNodoShared ((void **)&LShared, p, s.shm_segsz, clave);
    return (p);
}
void do_AllocateCreateShared (char *tr[])
{
   key_t cl;
   size_t tam;
   void *p;
   void *L;

   if (tr[0]==NULL || tr[1]==NULL) {
		ImprimirListaShared ((void **)&LShared);
		return;
   }
  
   cl=(key_t)  strtoul(tr[0],NULL,10);
   tam=(size_t) strtoul(tr[1],NULL,10);
   if (tam==0) {
	printf ("No se asignan bloques de 0 bytes\n");
	return;
   }
   if ((p=ObtenerMemoriaShmget(cl,tam))!=NULL)
		printf ("Asignados %lu bytes en %p\n",(unsigned long) tam, p);
   else
		printf ("Imposible asignar memoria compartida clave %lu:%s\n",(unsigned long) cl,strerror(errno));
}

void do_AllocateShared (char *tr[])
{
   key_t cl;
   void *p, *L;

   if (tr[0]==NULL) {
		ImprimirListaShared(&L);
		return;
   }
  
   cl=(key_t)  strtoul(tr[0],NULL,10);

   if ((p=ObtenerMemoriaShmget(cl,0))!=NULL)
		printf ("Asignada memoria compartida de clave %lu en %p\n",(unsigned long) cl, p);
   else
		printf ("Imposible asignar memoria compartida clave %lu:%s\n",(unsigned long) cl,strerror(errno));
}

void doDeallocateShared (key_t cl)
{
    void *p;    
                             /*llamo a la funcion de la lista*/
                                              /*que devuelve la direccion donde*/
if ((p = DireccionNodoShared ((void **)&LShared, cl)) == NULL) {
    printf ("No hay bloque de esa clave mapeado en el proceso\n");
    return;
}
shmdt(p);
if (EliminarNodoDireccion ((void **)&LShared, p) == -1)
    printf ("Imposible quitar de lista: %s\n", strerror(errno));
}


void * MapearFichero (char * fichero, int protection)
{
    int df, map=MAP_PRIVATE,modo=O_RDONLY;
    struct stat s;
    void *p;

    if (protection&PROT_WRITE)
          modo=O_RDWR;
    if (stat(fichero,&s)==-1 || (df=open(fichero, modo))==-1)
          return NULL;
    if ((p=mmap (NULL,s.st_size, protection,map,df,0))==MAP_FAILED)
           return NULL;

}

void do_AllocateMmap(char *arg[])
{ 
     char *perm;
     void *p;
     int protection = 0;
     
     if (arg[0] == NULL) {
            ImprimirListaMmap((void **)&LMmap); 
            return;
     }
     
     if ((perm = arg[1]) != NULL && strlen(perm) < 4) {
            if (strchr(perm, 'r') != NULL) protection |= PROT_READ;
            if (strchr(perm, 'w') != NULL) protection |= PROT_WRITE;
            if (strchr(perm, 'x') != NULL) protection |= PROT_EXEC;
     }
     
     if ((p = MapearFichero(arg[0], protection)) == NULL)
             perror ("Imposible mapear fichero");
     else
             printf ("fichero %s mapeado en %p\n", arg[0], p);
}

void do_SharedDelkey (char *args[])
{
   key_t clave;
   int id;
   char *key=args[0];

   if (key==NULL || (clave=(key_t) strtoul(key,NULL,10))==IPC_PRIVATE){
        printf ("      delkey necesita clave_valida\n");
        return;
   }
   if ((id=shmget(clave,0,0666))==-1){
        perror ("shmget: imposible obtener memoria compartida");
        return;
   }
   if (shmctl(id,IPC_RMID,NULL)==-1)
        perror ("shmctl: imposible eliminar memoria compartida\n");
}

void * CadenatoPointer (char * s)
{
  void *p;
  sscanf(s,"%p",&p);
  return p;
}

ssize_t LeerFichero (char *f, void *p, size_t cont)
{
   struct stat s;
   ssize_t  n;  
   int df,aux;

   if (stat (f,&s)==-1 || (df=open(f,O_RDONLY))==-1)
	return -1;     
   if (cont==-1)   /* si pasamos -1 como bytes a leer lo leemos entero*/
	cont=s.st_size;
   if ((n=read(df,p,cont))==-1){
	aux=errno;
	close(df);
	errno=aux;
	return -1;
   }
   close (df);
   return n;
}

void do_IOReadFile (char *ar[])
{
   void *p;
   size_t cont=-1;  /*si no pasamos tamano se lee entero */
   ssize_t n;
   if (ar[0]==NULL || ar[1]==NULL){
	printf ("faltan parametros\n");
	return;
   }
   p=CadenatoPointer(ar[1]);  /*convertimos de cadena a puntero*/
   if (ar[2]!=NULL)
	cont=(size_t) atoll(ar[2]);

   if ((n=LeerFichero(ar[0],p,cont))==-1)
	perror ("Imposible leer fichero");
   else
	printf ("leidos %lld bytes de %s en %p\n",(long long) n,ar[0],p);
}


void Do_pmap (void) /*sin argumentos*/

{ pid_t pid; /*hace el pmap (o equivalente) del proceso actual*/

char elpid[32];

char *argv[4]={"pmap",elpid,NULL};

sprintf (elpid,"%d", (int) getpid());

if ((pid=fork())==-1){

perror ("Imposible crear proceso");

return;

}

if (pid==0){

if (execvp(argv[0],argv)==-1)

perror("cannot execute pmap (linux, solaris)");

argv[0]="procstat"; argv[1]="vm"; argv[2]=elpid; argv[3]=NULL;

if (execvp(argv[0],argv)==-1)/*No hay pmap, probamos procstat FreeBSD */

perror("cannot execute procstat (FreeBSD)");

argv[0]="procmap",argv[1]=elpid;argv[2]=NULL;

if (execvp(argv[0],argv)==-1) /*probamos procmap OpenBSD*/

perror("cannot execute procmap (OpenBSD)");

argv[0]="vmmap"; argv[1]="-interleave"; argv[2]=elpid;argv[3]=NULL;

if (execvp(argv[0],argv)==-1) /*probamos vmmap Mac-OS*/

perror("cannot execute vmmap (Mac-OS)");

exit(1);

}

waitpid (pid,NULL,0);

} 


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

void memfill (void *addr, size_t cont, unsigned char hcode)
{
  unsigned char *arr = (unsigned char *) addr;
  size_t i;

  for (i = 0; i < cont; i++)
    arr[i] = hcode;
}

void memdump (void *addr, size_t cont)
{
  unsigned char *ptr = (unsigned char *) addr;
  size_t i, j;

  /* Imprime los bytes en bloques de 25 bytes por línea (o 16, común en shells) */
  for (i = 0; i < cont; i += 25) {
    
    /* Imprime la dirección de memoria al inicio de la línea */
    printf ("%p: ", (void *) (ptr + i));

    for (j = 0; j < 25; j++) {
      if (i + j < cont)
        printf ("%02X ", ptr[i + j]);
      else
        printf ("   "); 
    }

    printf (" ");

    for (j = 0; j < 25; j++) {
      if (i + j < cont) {
        unsigned char c = ptr[i + j];
        /* Muestra el carácter si es imprimible; si no, muestra un punto '.' */
        if (c >= 32 && c <= 126)
          printf ("%c", c);
        else
          printf (".");
      }
    }

    printf ("\n");
  }
}

ssize_t EscribirFichero (char *f, void *p, size_t cont, int overwrite)
{
   ssize_t n;  
   int df, aux, flags;

   flags = O_WRONLY | O_CREAT | (overwrite ? O_TRUNC : O_EXCL);

   if ((df = open(f, flags, 0666)) == -1)
      return -1;     

   if ((n = write(df, p, cont)) == -1) {
      aux = errno;
      close(df);
      errno = aux;
      return -1;
   }

   close (df);
   return n;
}

/* Imprime todos los bloques asignados en el shell (Shared y Mmap) */
void ImprimirBloquesMemoria (void)
{
  ImprimirListaShared ((void **)&LShared);
  ImprimirListaMmap ((void **)&LMmap);
}

/* Imprime direcciones de 3 funciones de la shell y 3 de la librería C */
void ImprimirFuncionesMemoria (void)
{
  printf ("--- Funciones del programa ---\n");
  printf ("%p: %s\n", (void *) Cmd_memory, "Cmd_memory");
  printf ("%p: %s\n", (void *) Cmd_memfill, "Cmd_memfill");
  printf ("%p: %s\n", (void *) Cmd_memdump, "Cmd_memdump");

  printf ("--- Funciones de la libreria C ---\n");
  printf ("%p: %s\n", (void *) printf, "printf");
  printf ("%p: %s\n", (void *) malloc, "malloc");
  printf ("%p: %s\n", (void *) open, "open");
}

/* Imprime direcciones de variables automaticas, estaticas y externas */
void ImprimirVariablesMemoria (void)
{
  int auto_var1 = 5;
  int auto_var2 = 15;
  int auto_var3; /* No inicializada */

  printf ("--- Variables automaticas (locales) ---\n");
  printf ("%p: auto_var1 (inicializada)\n", (void *) &auto_var1);
  printf ("%p: auto_var2 (inicializada)\n", (void *) &auto_var2);
  printf ("%p: auto_var3 (no inicializada)\n", (void *) &auto_var3);

  printf ("--- Variables estaticas ---\n");
  printf ("%p: static_var1 (inicializada)\n", (void *) &static_var1);
  printf ("%p: static_var2 (inicializada)\n", (void *) &static_var2);
  printf ("%p: static_var3 (no inicializada)\n", (void *) &static_var3);

  printf ("--- Variables externas (globales) ---\n");
  printf ("%p: global_var1 (inicializada)\n", (void *) &global_var1);
  printf ("%p: global_var2 (inicializada)\n", (void *) &global_var2);
  printf ("%p: global_var3 (no inicializada)\n", (void *) &global_var3);
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


void Cmd_memfill (char *tr[])
{
  void *addr;
  size_t cont = 128;           //Valor por defecto si no se pasa cont (ejemplo: 128 bytes) 
  unsigned char hcode = 'A';   // Valor por defecto: 0x41 ('A')

  if (tr[0] == NULL) {
    printf ("Faltan parametros: memfill addr [cont] [byte]\n");
    return;
  }

  /* Convierte la cadena hexadecimal/decimal a puntero */
  addr = CadenatoPointer (tr[0]);

  if (tr[1] != NULL)
    cont = (size_t) strtoul (tr[1], NULL, 10);

  if (tr[2] != NULL)
    hcode = (unsigned char) strtoul (tr[2], NULL, 16); /* Convierte el valor hex (ej: 0x55 o 55) */

  printf ("Llenando %unsigned long bytes de memoria en %p con el byte 0x%02X\n", (unsigned long) cont, addr, hcode);
  
  memfill (addr, cont, hcode);
}

void Cmd_memdump (char *tr[])
{
  void *addr;
  size_t cont = 25; /* Valor por defecto si no se pasa 'cont' */

  if (tr[0] == NULL) {
    printf ("Faltan parametros: memdump addr [cont]\n");
    return;
  }

  addr = CadenatoPointer (tr[0]);

  if (tr[1] != NULL)
    cont = (size_t) strtoul (tr[1], NULL, 10);

  memdump (addr, cont);
}
void Cmd_io (char *tr[])
{
  void *p;
  size_t cont;
  ssize_t n;

  if (tr[0] == NULL) {
    printf ("uso: io [read|write] ...\n");
    return;
  }

  /* SUBCOMANDO READ: io read f addr cnt */
  if (!strcmp(tr[0], "read")) {
    if (tr[1] == NULL || tr[2] == NULL || tr[3] == NULL) {
      printf ("uso: io read f addr cnt\n");
      return;
    }
    p = CadenatoPointer(tr[2]);
    cont = (size_t) atoll(tr[3]);

    if ((n = LeerFichero(tr[1], p, cont)) == -1)
      perror ("Imposible leer fichero");
    else
      printf ("leidos %lld bytes de %s en %p\n", (long long) n, tr[1], p);
  }

  /* SUBCOMANDO WRITE: io write [-o] f addr cnt */
  else if (!strcmp(tr[0], "write")) {
    int overwrite = 0;
    int pos = 1;

    if (tr[1] != NULL && !strcmp(tr[1], "-o")) {
      overwrite = 1;
      pos = 2;
    }

    if (tr[pos] == NULL || tr[pos+1] == NULL || tr[pos+2] == NULL) {
      printf ("uso: io write [-o] f addr cnt\n");
      return;
    }

    p = CadenatoPointer(tr[pos+1]);
    cont = (size_t) atoll(tr[pos+2]);

    if ((n = EscribirFichero(tr[pos], p, cont, overwrite)) == -1)
      perror ("Imposible escribir fichero");
    else
      printf ("escritos %lld bytes en %s desde %p\n", (long long) n, tr[pos], p);
  }

  else {
    printf ("Uso incorrecto de io: subcomando no valido '%s'\n", tr[0]);
  }
}

void Cmd_memory (char *tr[])
{
  if (tr[0] == NULL || !strcmp (tr[0], "-all")) {
    ImprimirBloquesMemoria ();
    ImprimirFuncionesMemoria ();
    ImprimirVariablesMemoria ();
    return;
  }

  if (!strcmp (tr[0], "-blocks")) {
    ImprimirBloquesMemoria ();
  }
  else if (!strcmp (tr[0], "-funcs")) {
    ImprimirFuncionesMemoria ();
  }
  else if (!strcmp (tr[0], "-vars")) {
    ImprimirVariablesMemoria ();
  }
  else if (!strcmp (tr[0], "-pmap")) {
    Do_pmap (); /* Reutiliza la funcion Do_pmap que ya tienes implementada */
  }
  else {
    printf ("Opcion no valida. Uso: memory [-blocks|-funcs|-vars|-all|-pmap]\n");
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
   {"list", list},
   {"memfill", Cmd_memfill},
   {"memdump", Cmd_memdump},
   {"io", Cmd_io},
   {"memory", Cmd_memory},
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
