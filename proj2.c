#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <signal.h>
#include <unistd.h>
#include <ctype.h>

#define MUTEX "/xzelen24.proj2_mutex"
#define SEM_bus "/xzelen24.proj2_bus"
#define SEM_boarded "/xzelen24.proj2_boarded"
#define SEM_depart "/xzelen24.proj2_depart"
#define SEM_end "/xzelen24.proj2_end"
#define SEM_Arrival "/xzelen24.proj2_arrival"
#define SEM_Written "/xzelen24.proj2_written"

int MAX( int x,int y)//maximum
{if(x>y) return x;
  else return y;}
int MIN( int x,int y)
  {if(x>y) return y;
  else return x;}

sem_t *mutex=NULL;//semafor pre riadenie pristupu k premennej waiting a rovnaky vyznam ako v little book of semaphores(dalej len kniha)
sem_t *bus=NULL;  //semafor pre prijazd autobusu, vyznam ako v knihe
sem_t *boarded=NULL;//rovnako ako bus
sem_t *semaphore_Arrival=NULL;//signalizacia prichodu autobusu
sem_t *semaphore_depart=NULL;//odchod autobusu, pristup do premmennej action
sem_t *semaphore_end=NULL;//vylucenie pristupu do tej istej premmennej v dvoch usekoch kodu
sem_t *semaphore_written=NULL;//proces moze vypisat, bus dosiel
unsigned int *waiting=NULL;//cakajuce procesy na zastavke
int waitingID=0;
unsigned int *action=NULL;//prevadzany vypis
int actionID=0;
unsigned int *flag=NULL;//priznak chyby
int flagID=0;
FILE *f;

int init()//otvorenie semaforov a zdielanej pamate
  {
    if ((waitingID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
      {
        fprintf(stderr, "Cannot share variabile\n" );
        return -1;
      }

    if ((waiting = shmat(waitingID, NULL, 0)) == NULL)
      {
        fprintf(stderr, "Cannot share variabile\n" );
        return -1;
      }
    if ((flagID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
      {
        fprintf(stderr, "Cannot share variabile\n" );
        return -1;
      }

    if ((flag = shmat(flagID, NULL, 0)) == NULL)
      {
        fprintf(stderr, "Cannot share variabile\n" );
        return -1;
      }
    if ((actionID = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666)) == -1)
      {
        fprintf(stderr, "Cannot share variabile\n" );
        return -1;
      }

    if ((action = shmat(actionID, NULL, 0)) == NULL)
      {
        fprintf(stderr, "Cannot share variabile\n" );
        return -1;
      }
    if((mutex=sem_open(MUTEX, O_CREAT | O_EXCL, 0666, 1))==SEM_FAILED)
      {
      fprintf(stderr, "Cannot open mutex 1\n" );
      return -1;
      }
    if((bus=sem_open(SEM_bus, O_CREAT | O_EXCL, 0666, 0))==SEM_FAILED)
      {
      fprintf(stderr, "Cannot open semaphore 2\n" );
      return -1;
      }
    if((semaphore_Arrival=sem_open(SEM_Arrival, O_CREAT | O_EXCL, 0666, 0))==SEM_FAILED)
      {
      fprintf(stderr, "Cannot open semaphore 3\n" );
      return -1;
    }
    if((semaphore_written=sem_open(SEM_Written, O_CREAT | O_EXCL, 0666, 0))==SEM_FAILED)
      {
      fprintf(stderr, "Cannot open semaphore 7\n" );
      return -1;
    }
    if((semaphore_depart=sem_open(SEM_depart, O_CREAT | O_EXCL, 0666, 0))==SEM_FAILED)
      {
      fprintf(stderr, "Cannot open semaphore 6\n" );
      return -1;
    }
    if((boarded=sem_open(SEM_boarded, O_CREAT | O_EXCL, 0666, 0))==SEM_FAILED)
      {
      fprintf(stderr, "Cannot open semaphore 4\n" );
      return -1;
      }
    if((semaphore_end=sem_open(SEM_end, O_CREAT | O_EXCL, 0666, 1))==SEM_FAILED)
      {
      fprintf(stderr, "Cannot open semaphore 5\n" );
      return -1;
    }
    return 0;
  }

void cleantrash()//upratanie semaforov a ich odlinkovanie ich odkazov + upratanie zdielanej pamate
  {
  munmap(waiting,sizeof(unsigned int));
  munmap(flag,sizeof(unsigned int));
  munmap(action,sizeof(unsigned int));
  sem_close(semaphore_written);
  sem_close(semaphore_end);
  sem_close(boarded);
  sem_close(semaphore_Arrival);
  sem_close(mutex);
  sem_close(semaphore_depart);
  sem_close(bus);
  sem_unlink(SEM_Written);
  sem_unlink(SEM_end);
  sem_unlink(SEM_depart);
  sem_unlink(SEM_bus);
  sem_unlink(SEM_Arrival);
  sem_unlink(SEM_boarded);
  sem_unlink(MUTEX);
  shmctl(flagID, IPC_RMID, NULL);
  shmctl(waitingID, IPC_RMID, NULL);
  shmctl(actionID, IPC_RMID, NULL);
  if(f != NULL) fclose(f);


  }

void proces_rider(int rid)//jedna osoba ,,rider''
  {
  sem_wait(semaphore_end); //cakanie na pristup do premmenj
  fprintf(f,"%d \t: RID %d\t\t : start\n",*action, rid);//vypis
  fflush(f);//strem
  (*action)++;//akcia bola prevedena
  sem_post(semaphore_end);//zapisal som
  if(*flag==1)//kontrola ci sa nic nestalo v inych procesoch napr ci presiel fork
  { sem_post(boarded);
    (*waiting)--;
    sem_post(semaphore_written);
    cleantrash();
    exit(0);
  }
  sem_wait(mutex);//vyznam ako v knihe, cakam na spracovanie
  (*waiting)++;//cakajuci+1
  sem_wait(semaphore_end);//cakam na pristup k premmenj pre vystup
  fprintf(f,"%d \t: RID %d\t\t : enter: %d \n",*action, rid,*waiting);
  fflush(f);//strem
  (*action)++;//akcia
  sem_post(semaphore_end);//vypisal som
  sem_post(mutex);//dolezita cast skoncila
  if(*flag==1)//ci je vsetko ok
  { sem_post(boarded);
    (*waiting)--;
    sem_post(semaphore_written);
    cleantrash();
    exit(0);
  }
  sem_wait(bus);//cakam na prijazd autobusu
  sem_post(boarded);//nastupil som
  sem_wait(semaphore_Arrival);//cakam kym autobus odide
  sem_wait(semaphore_end);//cakam na pristup k premmenj
  fprintf(f,"%d \t: RID %d\t\t : boarding\n",*action, rid);
  fflush(f);
  (*action)++;
  sem_post(semaphore_end);//zapisal som
  sem_post(semaphore_written);//mam zapisane, autobus moze skoncit
  sem_wait(semaphore_depart);//cakam na odchod
  sem_wait(semaphore_end);//pristup k premmennej
  fprintf(f,"%d \t: RID %d\t\t : finish\n",*action, rid);
  fflush(f);
  (*action)++;
  sem_post(semaphore_end);//zapisal som
  cleantrash();
  exit(0);
  }

void process_bus(unsigned argument, int capacity, int delay)//autobus jazdiaci dookola kym nepovozi vsetky procesy
  { unsigned done=0;//urobene procesy
    unsigned ride=1;//jazda v poradi
    unsigned thiscycle=0;//tento cyklus jazd
    sem_wait(semaphore_end);//cakanie na semafor pre zapis
    fprintf(f,"%d \t: BUS \t\t : start\n",*action);
    if((*flag)==1){
      cleantrash();
      exit(1);
      sem_post(semaphore_end);
      }
    fflush(f);
    (*action)++;
    sem_post(semaphore_end);
    while((argument!=done) && ((*flag)!=1))//jazdim kym mam nespracovanych alebo nebol flag
    {
      sem_wait(mutex);//cakam na pristup k zdrojom
      sem_wait(semaphore_end);
      fprintf(f,"%d \t: BUS \t\t : arrival\n",*action);
      fflush(f);
      (*action)++;
      sem_post(semaphore_end);
      int n=MIN(capacity,(int)(*waiting));//stanovenie poctu jazd
      if((*waiting)!=0)//ak ktosi caka vypis
        {
        sem_wait(semaphore_end);
        fprintf(f,"%d \t: BUS\t\t : start boarding: %d\n",*action, (*waiting));
        fflush(f);
        (*action)++;
        sem_post(semaphore_end);
        }
      for(int i=0; i<n; i++)//autobus nabera procesy po jednom
        {
          if(*flag==1)
          {cleantrash();
          exit(1);}
        sem_post(bus);//autobus prisiel
        sem_wait(boarded);//proces nastupil
        sem_post(semaphore_Arrival);//autobus odchadz
        sem_wait(semaphore_written);//proces si zapisal ze nastupuje
        done++;
        thiscycle++;
        }

      if(*waiting!=0)
      {
      *waiting=MAX(((int)(*waiting))-capacity, 0);//novy pocet cakajucich
      sem_wait(semaphore_end);//zapis
      fprintf(f,"%d \t: BUS\t\t : end boarding: %d\n",*action, (*waiting));
      fflush(f);
      (*action)++;
      sem_post(semaphore_end);
      }
      ride++;//presla jazda
      sem_post(mutex);//uvolnenie zdroja
      sem_wait(semaphore_end);//zapis
      fprintf(f,"%d \t: BUS\t\t : depart\n",*action);
      fflush(f);
      (*action)++;
      sem_post(semaphore_end);//zapisane
      if(delay!=0)//simulacia jazdy
        usleep((rand()%(delay+1))*1000);
      sem_wait(semaphore_end);//zapis
      fprintf(f,"%d \t: BUS\t\t : end\n",*action);
      if((*flag)==1){
        cleantrash();
        sem_post(semaphore_end);
        exit(1);
        }
      fflush(f);

      while(thiscycle){
        if((*flag)==1){
          cleantrash();
          exit(1);
          }
        sem_post(semaphore_depart);//odchod autobusu bol... proces sa moze skoncit
        thiscycle--;
      }
      (*action)++;
      sem_post(semaphore_end);
      if((*flag)==1){
        cleantrash();
        exit(1);
        }
    }
  if(*flag==1)
  {cleantrash();
  exit(1);}
  sem_wait(semaphore_end);//zapis
  fprintf(f,"%d \t: BUS\t\t : finish\n",*action);
  fflush(f);
  (*action)++;
  sem_post(semaphore_end);
  cleantrash();
//  printf("new bus\n");
  exit(0);
  }

void gen_riders(int R, int delay)//generovanie procesov
  {int rid=0;
  pid_t RID[R-1];
  for(int i=0; i<R ; i++)//generujem procesy kolko som dostal v zadani
    {
      rid++;
      RID[i]=fork();
      if(RID[i]<0)
        {
        fprintf(stderr, "Fork in function gen_riders failed\n");
        *flag=1;
        for(int q=0; q<i;q++)
          {kill(RID[q],SIGKILL);//ukoncenie podprocesov
            //printf("zabil som proces %d z %d\n",q,i-1 );
          sem_post(semaphore_depart);
          sem_post(boarded);
          sem_post(semaphore_written);
          sem_post(semaphore_end);
          sem_post(semaphore_Arrival);
          sem_post(semaphore_end);
          sem_post(semaphore_Arrival);
          sem_post(semaphore_depart);
          sem_post(semaphore_written);
        }

        cleantrash();
        sem_post(mutex);
        exit(1);
        }
      if (RID[i]==0)
        {
        proces_rider(rid);//samotny proces
        }
      if(delay!=0)
        usleep((rand()%(delay+1))*1000);//simulacia generovania na nahodnu dobu
    }
  for(int i=0; i<R; i++)
  waitpid(RID[i],0,0);//cakanie na ukoncene procesov
  cleantrash();
  exit(0);
  }

int test_args(int argc, char **argv)//testovanie argumentov
  {
    if(argc!=5)//spravny pocet
      {
      fprintf(stderr, "Too much or too few arguments\n" );
      return -1;
      }
    for(int i=1;i<5;i++)//test ci su to cele cisla na vsetkych 5 argumentov, ptm ci su kladne
      {int c=0;
        while(argv[i][c])
          {
            if((isdigit(argv[i][c]) == 0) || argv[i][c]<0)
              {
              fprintf(stderr, "Arguments are not correct\n" );
              return -1;
              }
            c++;
          }
        if((((atoi(argv[i])<=0) && (i==1 || i==2))) || ((i==3 || i==4) &&((atoi(argv[i])>1000))))//testovanie rozsahov zo zadania
        {
          fprintf(stderr, "Arguments are not correct\n" );
          return -1;
        }
      }
    return 0;
  }

int main(int argc, char **argv)
  {
  if((f=fopen("proj2.out","w")) == NULL)//subor na zapis
    {
    fprintf(stderr,"file error\n");
    return 1;
    }
  setbuf(stdout, NULL);//nastavenie buffera
  if(test_args(argc, argv)==-1)//test argumentov
    {
    return 1;
    }
  pid_t BUS,RIDERS;//proces bus a rider
  if(init()!=0)//otvorenie semaforov
    {
    cleantrash();
    return 1;
    }
  *action=1;//nastavenie premmennej ktora bude na vystup
  *flag=0;//nastavenie flagu na 0

  if((BUS=fork()) <0)//vytvorenie procesu bus, handle error
    {
    fprintf(stderr, "Fork bus did not succeed\n");
    cleantrash();
    return 1;
    }
  if (BUS==0)
    {
    process_bus(atoi(argv[1]), atoi(argv[2]), atoi(argv[4]));
    }
  if (BUS > 0)
    {
    if((RIDERS=fork())<0)//vytvorenie procesu rider generator ktory potom vytvori procesy
      {
      fprintf(stderr, "Fork riders did not succeed\n");
      cleantrash();
      return 1;
      }
    if(RIDERS==0)
      {
      gen_riders(atoi(argv[1]), atoi(argv[3]));//generuje procesy
      }
    }
    if((*flag)==1)//kontrola flagu
      {cleantrash();
        return 1;
      }
  waitpid(BUS,0,0);//caka na autobus
  if((*flag)==1)//kontrola flagu
    {cleantrash();
      return 1;
    }
  waitpid(RIDERS,0,0);//caka na cestujucich
  if((*flag)==1)//kontrola flagu
    {cleantrash();
      return 1;
    }
  cleantrash();
  return 0;
}
