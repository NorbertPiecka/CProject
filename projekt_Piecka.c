//PROJEKT Z PRZEDMIOTU SYSTEMY OPERACYJNE
//WYKONAL: NORBERT PIECKA WCY18IY1S1
//PIERWSZA CZESC: KOLEJKI KOMUNIKATOW
//DRUGA CZESC: SYGNAŁY, SEMAFORY, PAMIEC DZIELONA
// S1 SIGCONT -KONIEC
// S2 SIGUSR1 -WSTRZYMANIE
// S3 SIGUSR2 -WZNOWIENIE
// S4 SIGINT -WYSYŁANIE DO RESZTY
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>	 	
#include <unistd.h>	 
#include <wait.h>	
#include <time.h>	 
#include <stdlib.h>	 
#include <string.h>
#include <signal.h>
#include <semaphore.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define ROZ 200
#define FLAG_PIDS 2

int *buf,*pids;
int i=0;

struct msgbuf{
	long mtype;
	char mtext[200];

};

int getRozmiar(char tab[]);
void szyfrowanie(char tab[],int rozmiar);
void obslugaSygnalu1(int signum);

static struct sembuf opr;
void upOperation(int semid, int semnum); 
void downOperation(int semid, int semnum);

void S1(int signo);
void S2(int signo);
void S3(int signo);
void S4(int signo);

int main(){

	int pid1,pid2,pid3,msgid,rozmiar=0,shmid,semid,pidshm;
	//char *buf;
	//KOLEJKA KOMUNIAKTOW//
	
	struct msgbuf message;
	key_t key = ftok(".",'N');
	msgid = msgget(key,IPC_CREAT | 0600);
	
	//SEMAFORY//
	semid = semget(key, 3, IPC_CREAT | 0600);
	if(semid < 0) {
        perror("Blad tworzenia semaforow:");
        exit(1);
    }

    if (semctl(semid, FLAG_PIDS, SETVAL, 0) == -1){
        perror("Nadanie wartosci semaforowi 2");
        semctl(semid, 0, IPC_RMID, 0);
        exit(1);
    }

	//PAMIEC DZIELONA//
	shmid = shmget(key, 2*sizeof(int), IPC_CREAT|0600);
	pidshm = shmget(1233, 3*sizeof(int), IPC_CREAT|0600);
 	if (shmid == -1 || pidshm == -1){
		perror("Blad utworzenia segmentu pamieci wspoldzielonej");
		exit(1);
 	}
 	
	buf = shmat(shmid, NULL, 0);
	pids = shmat(pidshm, NULL, 0);
	
	if (buf == NULL || pids == NULL){
		perror("Blad przylaczenia segmentu pamieci wspoldzielonej");
		exit(1);
	}
	buf[0]=1;//Praca ustawiona na 
	buf[1]=0;//Otrzymany sygnal
	
	////////////////////PROCESY/////////
	if((pid1=fork())==0){
		upOperation(semid, FLAG_PIDS);
		//
		signal(SIGCONT,S1);
		signal(SIGUSR1,S2);
		signal(SIGUSR2,S3);
		signal(SIGINT,S4);
		message.mtype = 2;
		while(1){
			if(buf[0]==1){
			//printf("Proces 1 %d_%d: ",pids[0],buf[0]);
			fgets(message.mtext,ROZ,stdin);
			msgsnd(msgid,&message,sizeof(message.mtext),0);
			usleep(1000);

	}
		}
		return 0;
	}
	pids[0]=pid1;
	if((pid2=fork())==0){
		upOperation(semid, FLAG_PIDS);
		//
		signal(SIGCONT,S1);
		signal(SIGUSR1,S2);
		signal(SIGUSR2,S3);
		signal(SIGINT,S4);
		while(1){
			if(buf[0]==1){
				msgrcv(msgid,&message,sizeof(message.mtext),2,0);
				rozmiar = getRozmiar(message.mtext);
				szyfrowanie(message.mtext,rozmiar);
				//printf("Proces 2 %d_%d: Szyfruje wiadomosc\n",pids[1],buf[0]);
				message.mtype = 3;
				msgsnd(msgid,&message,sizeof(message.mtext),0);
				usleep(100);
		}
		}
		return 0;
	}
	pids[1]=pid2;
	
	if((pid3=fork()) == 0){
		upOperation(semid, FLAG_PIDS);
		signal(SIGCONT,S1);
		signal(SIGUSR1,S2);
		signal(SIGUSR2,S3);
		signal(SIGINT,S4);
		while(1){
			if(buf[0]==1){

		msgrcv(msgid,&message,sizeof(message.mtext),3,0);
		//printf("Proces 3 %d_%d: %s \n",pids[2],buf[0],message.mtext);
		printf("%s",message.mtext);
		usleep(100);
	}
	}
		return 0;
	}
	pids[2]=pid3;
	
	printf("Pidy: \nPROCES 1: %d\nPROCES 2: %d\nPROCES 3: %d\n",pids[0],pids[1],pids[2]);
	downOperation(semid, FLAG_PIDS);
	downOperation(semid, FLAG_PIDS);
	downOperation(semid, FLAG_PIDS);
	wait(0);
	wait(0);
	wait(0);
	printf("Zwolniono zaosoby\n");
	msgctl(msgid, IPC_RMID, NULL);
	shmdt(buf);
	shmdt(pids);
	shmctl(pidshm, IPC_RMID, NULL);
	shmctl(shmid, IPC_RMID, NULL); 
	semctl(semid, 3,IPC_RMID, NULL); 
	return 0;

}

void S1(int signo){	// dla SIGCOUNT -  zakonczenie pracy
	buf[1] = 18;
	for(i=0;i<3;i++){
	if(pids[i]!=getpid()){
	kill(pids[i],SIGINT);
	}
	}
	kill(getpid(),SIGINT);
    }
void S2(int signo){ //dla SIGUSR1 - wstrzymanie pracy
	buf[1] = 10;
	for(i=0;i<3;i++){
	if(pids[i]!=getpid()){
	kill(pids[i],SIGINT);
	}
	}
	kill(getpid(),SIGINT);
   }
void S3(int signo){ //dla SIGUSR2 - wznowienie pracy
	buf[1] = 12;
	for(i=0;i<3;i++){
	if(pids[i]!=getpid()){
	kill(pids[i],SIGINT);
	}
	}
	kill(getpid(),SIGINT);
 }
void S4(int signo){
    if ( buf[1] == 10){
		buf[0] = 0;
    }else if ( buf[1] == 12){
		buf[0] = 1;
	}else {
	kill(getpid(),SIGKILL);
	}
}

int getRozmiar(char tab[]){
	int rozm = 0,i = 0;
	while(tab[i]){
		rozm++;
		i++;
	}
	return rozm;
}
void szyfrowanie(char tab[],int rozmiar){
	int i=0;
	for(i=0;i<rozmiar-1;i++){
	if(tab[i] == ' '){
		tab[i] = tab[i] + 1 + i%26;
	}else{
		if(tab[i] > 120){
			tab[i] = tab[i] - 3 - i%6;
		}else{
		tab[i] = tab[i] + 3 + i%4;
	}
	}
	}
}
void upOperation(int semid, int semnum){ //Fukcja odpowiedzialna za podnoszenie semafora
	opr.sem_num = semnum;
	opr.sem_op = 1;
	opr.sem_flg = 0;
	if (semop(semid, &opr, 1) == -1){
 		perror("Podnoszenie semafora");
		exit(1);
	}
} 

void downOperation(int semid, int semnum){ //Funkcja odpowiedzialna za opuszczanie semafora
 	opr.sem_num = semnum;
	opr.sem_op = -1;
	opr.sem_flg = 0;
 	if (semop(semid, &opr, 1) == -1){
		perror("Opuszczenie semafora");
		exit(1);
 	}
}


