# Sistemas-Operativos-2026-2027
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <time.h>

#define MAXENTRADA 2048
#define MAXNOMBRE 1024


struct COMANDO{
  char * nombre;
  void (*funcion)(char**);
};

void MostrarDirActual()
{
   char dir[MAXNOMBRE];
   
    if (getcwd(dir,MAXNOMBRE)==NULL)
	perror("Imposible obtener directorio");
    else
       printf ("%s\n",dir);
}

int ComprobarSegundoPlano (char *tr[])
{
    int i;
    for (i=0; tr[i]!=NULL;i++)
        if (!strcmp(tr[i],"&")){ /*& indica segund plano*/ 
            tr[i]=NULL;         /*es el ultimo argumento*/
            return i;       /*si solo hay un & no se ejecuta nada en pplano*/
            }
    return 0;
}


void Proceso (char *tr[], int splano)
{
   pid_t pid;
   void Cmd_exec (char **);
   int background=splano || ComprobarSegundoPlano(tr);
   if ((pid=fork())==-1){
        perror ("Imposible crear proceso");
        return;
        }
  if (pid==0){  /*proceso hijo*/
    Cmd_exec (tr);
    exit(255); /*por si falla exec*/
    }
  if (!background) 
    waitpid(pid,NULL,0);
}

/*********************************************/
/*************COMANDOS DEL SHELL************************/
void Cmd_fin (char * arg[])  /*todos los cmd_ comparten prototipo*/
{                            /*reciben los mismos parametros aunque no los usen*/
    exit(0); //Hace que termine el programa, biblioteca stdlib.h.
}

void Cmd_autores(char *arg[])
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
  if (execvp(arg[0],arg)==-1)
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

void Cmd_chdir (char * arg[])
{
   if (arg[0]==NULL)
      MostrarDirActual();
   else if (chdir(arg[0])==-1)
      perror("Imposible cambiar directorio");
}

void Cmd_pwd(char * arg[])
{
    MostrarDirActual();
}

void Cmd_pid (char * arg[])
{
    if (arg[0]==NULL)
        printf ("El pid del proceso es %d\n",(int) getpid());
    else
        if (!strcmp (arg[0],"-p"))
            printf ("El pid del proceso padre es %d\n",(int) getppid());
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


/**************************SHELL**************************/

static struct COMANDO C[]={ /*no declaro dimension, que la coge de la inicializacion*/
   {"fin",Cmd_fin},
   {"exit",Cmd_fin},
   {"bye",Cmd_fin},
   {"quit",Cmd_fin},
   {"pid",Cmd_pid},
   {"pwd", Cmd_pwd},
   {"chdir",Cmd_chdir},
   {"authors",Cmd_autores},
   {"exec",Cmd_exec},
   {"pplano",Cmd_pplano},
   {"splano",Cmd_splano},
   {"date",Cmd_date},
   {"sysinfo",Cmd_sysinfo},
   {"dup", Cmd_dup},
   {"lseek", Cmd_lseek},
   {"makefile", Cmd_makefile},
   {"readstr", Cmd_readstr},
   {"writestr", Cmd_writestr},
   {NULL,NULL},             /*NULL marca el final del array*/
  };

void DecidirComando (char *tr[])
{
  int i;

  if (tr[0]==NULL) return; /*por si luego quito lode TrocearCadena ==0*/
  for (i=0; C[i].nombre!=NULL; i++) /*recorro el array hasta el NULL*/
    if (!strcmp(C[i].nombre,tr[0])){
        (*C[i].funcion)(tr+1);
	    return; /*si es algun cmd_ ya no miro mas*/
    }
  Proceso(tr,0); /*si no es un comando, es un  ejecutable externo*/
}

int TrocearCadena(char * cadena, char * trozos[])
{
  int i=1;
  if ((trozos[0]=strtok(cadena," \n\t"))==NULL)
      return 0;
  while ((trozos[i]=strtok(NULL," \n\t"))!=NULL)
     i++;
  return i;
}	
void ProcesarEntrada(char * entrada)
{
   char *tr[MAXENTRADA/2];
   if (TrocearCadena(entrada,tr)==0) /*no hay nada*/
	return;
   DecidirComando(tr);
}

int main(int argc, char *argv[], char *ent[])
{
   char entrada[MAXENTRADA];

   while (1){
      printf ("-> ");
      fgets(entrada,MAXENTRADA,stdin);
      ProcesarEntrada(entrada);
   }
}
