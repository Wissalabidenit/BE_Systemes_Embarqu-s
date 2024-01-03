#include <sqlite3.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h> 
#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <limits.h>
#include <signal.h> 
#include <time.h>  
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

int rt_init(void);
void init_pwm(void);
int db_insert();
unsigned int db_countrow();
struct meter_param* db_read();
int db_create_table();
sqlite3 *db_open();
 pthread_attr_t attr;
 int ret;      
char kbhit(void);
struct gpiod_line *lineRed;
void init_timer(void);
char kbhit(void);
int fd,cfd,sfd;
u_int32_t T=1000000, c=0,duty=0;
char a[5];
char r[50], t[100];
timer_t timerid;
 struct sigevent sev;
 struct itimerspec trigger;
 pthread_cond_t timer,adc;
pthread_mutex_t lock;
sqlite3 *db1;//cree une base de données 
//****************************Parametre de serveur**********************************************************************// 
struct server_param{
	char   ip_address[16];
	u_int16_t port;
	u_int8_t max_clients;
};
struct meter_param {//structure to hold energy meter parameters
	u_int32_t id;
	u_int32_t adc;
	time_t date;
};
time_t get_date_time(void) {
    return time(NULL); // Ceci renvoie la date/heure actuelle en utilisant la fonction time()
}


struct meter_param m1;
//*******************************THREAD_SERVER*****************************************************************************//

void *server_func(void *s_param) {
    printf("THREAD SERVEUR : IP programmée = %s\n", ((struct server_param *)s_param)->ip_address);
    printf("THREAD SERVEUR : Numéro de port programmé = %u\n", ((struct server_param *)s_param)->port);
    printf("THREAD SERVEUR : Nombre maximal de clients pris en charge dans la file d'attente = %u\n", ((struct server_param *)s_param)->max_clients);

    u_int32_t res, client_len, server_len;
    struct sockaddr_in server_adr, client_adr;

    server_adr.sin_family = AF_INET;
    server_adr.sin_addr.s_addr = inet_addr(((struct server_param *)s_param)->ip_address);
    server_adr.sin_port = ((struct server_param *)s_param)->port;

    server_len = sizeof(server_adr);
    client_len = sizeof(client_adr);

    // Création d'une socket
    int sfd = socket(AF_INET, SOCK_STREAM, 0); // Socket TCP orientée connexion
    if (sfd < 0) {
        perror("THREAD SERVEUR : Impossible d'ouvrir la socket : ");
        goto cl_bas;
    }
    printf("THREAD SERVEUR : Socket créée avec succès\n");

    uint8_t on = 1;
    setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // Attribution d'un nom (paramètres) à la socket
    res = bind(sfd, (struct sockaddr *)&server_adr, server_len);
    if (res < 0) {
        perror("THREAD SERVEUR : Impossible de lier la socket : ");
        goto cl_bas;
    }
    printf("THREAD SERVEUR : Socket liée avec succès\n");

    // Écoute des clients
    res = listen(sfd, ((struct server_param *)s_param)->max_clients);
    if (res < 0) {
        perror("THREAD SERVEUR : Impossible d'écouter les clients : ");
        goto cl_bas;
    }

    while (1) {
        printf("THREAD SERVEUR : La socket écoute les clients maintenant\n");

        int cfd = accept(sfd, (struct sockaddr *)&client_adr, &client_len);
        if (cfd < 0) {
            perror("THREAD SERVEUR : Impossible d'accepter le client : ");
            goto cl_bas;
        }

        printf("******************************* CLIENT CONNECTÉ\n");
        printf("Client connecté avec succès !!\n");
        printf("ADRESSE IP DU CLIENT : %s\n", inet_ntoa(client_adr.sin_addr));
        printf("************************************************\n");


        close(cfd);
    }

    close(sfd);
    pthread_exit("THREAD SERVEUR : Thread terminé avec succès !\n");

cl_bas:
    close(sfd);
    pthread_exit("THREAD SERVEUR : Échec du traitement du serveur !\n");
}


//*******************************Thread_adc*****************************************************************************//
void *thread_adc(void *v) {
	for(;;){
	
	pthread_mutex_lock(&lock);
	pthread_cond_wait(&timer, &lock);
	pthread_mutex_unlock(&lock);
		fd=open("adc5/in_voltage3_raw",O_RDONLY);
		read(fd,&a,sizeof(a));
		c=atoi(a);
		printf( "the value ADC is  %d \n",c);
		m1.id =1;  // Vous pouvez définir l'ID approprié
        m1.adc = c;
        m1.date = get_date_time();
        if (db_insert(db1, "ADC", &m1) == -1) {
            printf("Erreur lors de l'insertion des valeurs ADC dans la base de données.\n");
           
        }
		close(fd);
	pthread_mutex_lock(&lock);
	pthread_cond_signal(&adc);
	pthread_mutex_unlock(&lock);
	
	
  
        

}
	return NULL;
}
//*******************************Thread_pwm*****************************************************************************//
void *thread_pwm(void *v) {
	int fd;
	char str[10];
	while(1)
	{

	pthread_mutex_lock(&lock);
	pthread_cond_wait(&adc,&lock);
	pthread_mutex_unlock(&lock);
	fd=open("pwm/duty_cycle",O_WRONLY);
	duty= (2 * T / 10 ) + (8 * T / 10 * c  >> 12 );
	sprintf(str, "%d",duty);
	printf( "the value duty is %d \n",duty/10000);
	write(fd,str,sizeof(str));
	close(fd);	
	
	
	}
	return NULL;
}


//*******************************Thread_Timer*****************************************************************************//
void Tstimer_thread(union sigval sv) 
{
		pthread_mutex_lock(&lock);
		pthread_cond_signal(&timer);
		pthread_mutex_unlock(&lock);
        
		
        /* Will print "100ms elapsed." */
        puts("100ms elapsed.");
		
        timer_settime(timerid, 0, &trigger, NULL);
}
//*******************************programme main *****************************************************************************//
int main(void) {      
	
	    struct server_param myserver = {"127.0.0.0",1880,5};
		sqlite3 *db_open();
	    db_create_table(db1, "ADC");
		init_timer();
        init_pwm();
        db1 = db_open("ADC"); 

    if (db1 == NULL) {
        printf("Erreur lors de l'ouverture de la base de données.\n");
        return EXIT_FAILURE;
    }

    
    if (db_create_table(db1, "ADC") != 0) {
        printf("Erreur lors de la création de la table.\n");
        sqlite3_close(db1);  
        return EXIT_FAILURE;
    }
     u_int32_t row_to_read = 1; 
    struct meter_param *data = db_read(db1, "ADC", row_to_read); 
    if (data) {
        printf("ID : %u\n", data->id);
        printf("ADC : %u\n", data->adc);
        printf("date : %u\n", data->adc);
        
       
        free(data);
    }

    
        
		pthread_t *t1,*t2,*t3;
		if(rt_init()) exit(0);
		t1 = (pthread_t *) malloc(sizeof(pthread_t));
		t2 = (pthread_t *) malloc(sizeof(pthread_t));
		t3= (pthread_t *) malloc(sizeof(pthread_t));
		
		pthread_create(t1, NULL, thread_adc, NULL);	
		pthread_create(t2, NULL, thread_pwm, NULL);	
		pthread_create(t3, NULL, server_func,&myserver);	
	
        while(kbhit()!='q');
        

       
        timer_delete(timerid);
        free(t1);
        free(t2);
        free(t3);
       sqlite3_close(db1);
        return EXIT_SUCCESS;
}
//*******************************Init_timer*****************************************************************************//
void init_timer(void)
{
       
        memset(&sev, 0, sizeof(struct sigevent));
        memset(&trigger, 0, sizeof(struct itimerspec));
        sev.sigev_notify = SIGEV_THREAD;
        sev.sigev_notify_function = &Tstimer_thread;
        timer_create(CLOCK_REALTIME, &sev, &timerid);

        
        trigger.it_value.tv_sec = 0;
        trigger.it_value.tv_nsec = 100000000;  
        timer_settime(timerid, 0, &trigger, NULL);
        
}


//*******************************init_pwm*****************************************************************************//
void init_pwm(void)
{
	char str[10];
	int fd=open("pwmmux/state",O_WRONLY);
	write(fd,"pwm",sizeof("pwm"));
	close(fd);
	
	fd=open("pwm/period",O_WRONLY);
	sprintf(str, "%d",T);
	write(fd,str,sizeof(str));
	close(fd);	
	
	fd=open("pwm/duty_cycle",O_WRONLY);
	sprintf(str, "%d",(T*2)/10);
	write(fd,str,sizeof(str));
	close(fd);	
	
	fd=open("pwm/enable",O_WRONLY);
	write(fd,"1",sizeof("1"));
	close(fd);	
}
//*********************************************************************************************************************/
//############################################################################################################################


//################################################################################################## HELPING FUNCTIONS : SQLite


//Creation de la base de donné 

sqlite3 *db_open(char *dbname)
{
	sqlite3 *db;
	char dbfile[25];
	int rc;
	
	sprintf(dbfile,"%s.db",dbname);
	
	rc = sqlite3_open(dbfile,&db);
	if(rc){
	     printf("XXXX : Can't open database: %s\n", sqlite3_errmsg(db));
	     return NULL;
	}
    	
	printf("@=> Database \"%s\" created/opened successfully\n",dbfile);
	return db;
}

int db_create_table(sqlite3 *db,char *table_name)
{
	char *zErrMsg = NULL,sql[300];
	int rc;

	
   	sprintf(sql,"CREATE TABLE %s (ID INTEGER  NOT NULL PRIMARY KEY AUTOINCREMENT," \
   	"ADC INTEGER,"\
         "Time TEXT); " ,table_name);

	
	rc = sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
	if(rc!=SQLITE_OK){
		printf("XXXX : SQL error (Table = %s) - %s\n",table_name,zErrMsg);
	    	sqlite3_free(zErrMsg);
		return -1;
	}

	printf("@=> Table \"%s\" created successfully!!\n",table_name);
	return 0;
}
//*********************************************Création d'une fonction pour L'insertion dans de le base de données**********************************************
int db_insert(sqlite3 *db,char *table_name,struct meter_param *m1)
{
	char *zErrMsg = NULL,sql[300];
	int rc;

	sprintf(sql, "INSERT INTO %s (ADC,Time) " "VALUES (%d,%ld);", table_name, m1->adc,m1->date);
	
	rc = sqlite3_exec(db,sql,NULL,NULL,&zErrMsg);
	if( rc != SQLITE_OK ){
		printf("XXXX : SQL error - %s\n", zErrMsg);
	    	sqlite3_free(zErrMsg);
		return -1;
	}

	printf("@=> Database values inserted successfully!!\n");
	return 0;
}
//***********************************************************Fonction pour lire les valeurs dans le base ***********************************
struct meter_param* db_read(sqlite3 *db,char *table_name,unsigned int row)
{
	struct meter_param *x= (struct meter_param*)malloc(sizeof(struct meter_param));
	char sql[100],*zErrMsg = NULL;
	int rc;
	sprintf(sql,"SELECT * FROM %s",table_name);
	rc = sqlite3_exec(db, sql, NULL, &x, &zErrMsg);
	if( rc != SQLITE_OK ){
		printf("XXXX : SQL error - %s\n", zErrMsg);
	    	sqlite3_free(zErrMsg);
		free(x);
		return NULL;
	}

	printf("@=> Values read successfully!!\n");
	return x; 
}


//***********************************************************************Fonction de conter le nombre de ligne dans une table**********************************************************
unsigned int db_countrow(sqlite3 *db,char* table_name)
{
	char sql[100],*zErrMsg = NULL;
	int rc;
	sprintf(sql,"SELECT count(*) FROM %s",table_name);
	rc = sqlite3_exec(db, sql, NULL,NULL, &zErrMsg);
	if( rc != SQLITE_OK ){
		printf("XXXX : SQL error - %s\n", zErrMsg);
	    	sqlite3_free(zErrMsg);
		return 0; 
	}

	
	return 0;

}


int rt_init(void)
{
     
        struct sched_param param;
     
        /* Lock memory */
        if(mlockall(MCL_CURRENT|MCL_FUTURE) == -1) {
                printf("mlockall failed: %m\n");
                exit(-2);
        }
 
        /* Initialize pthread attributes (default values) */
        ret = pthread_attr_init(&attr);
        if (ret) printf("init pthread attributes failed\n");
 
        /* Set a specific stack size  */
        ret = pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN);
        if (ret) printf("pthread setstacksize failed\n");
           
        /* Set scheduler policy and priority of pthread */
        ret = pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
        if (ret) printf("pthread setschedpolicy failed\n");
              
        param.sched_priority = 80;
        ret = pthread_attr_setschedparam(&attr, &param);
        if (ret) printf("pthread setschedparam failed\n");
                
        /* Use scheduling parameters of attr */
        ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
        if (ret) printf("pthread setinheritsched failed\n");
        
        return ret;
        
}
//*****************************************************************************FIN*********************************************************
