#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>


#define SEATNUM 250
#define TEL 8
#define SEATLOW 1
#define SEATHIGH 5
#define T_LOW 5
#define T_HIGH 10
#define CARDSUCCESS 90
#define SEATCOST 20
#define BILLION  1000000000L;


#define SEAT 10
#define ZONE_A 5
#define ZONE_B 10
#define ZONE_C 10
#define A_SUCCESS 20
#define B_SUCCESS 40
#define C_SUCCESS 40
#define A_SEATCOST 30
#define B_SEATCOST 25
#define C_SEATCOST 20
#define CASH 4
#define T_CASH_LOW 2
#define T_CASH_HIGH 4

int customers;
unsigned int seed;
int *seatArray;
int chance = 0;
int balance = 0;
int id_transaction = 0;
double waitingTime = 0;
double waitingTimeCash = 0;
int currentTelInUse = 0;
int currentCashInUse = 0;
double assistanceTime = 0;


pthread_mutex_t TelCounter;
pthread_mutex_t ticketFinder;
pthread_mutex_t addToBalance;
pthread_mutex_t PrintMutex;
pthread_mutex_t timeMutex;
pthread_cond_t thresholdCond;

//Project 2
pthread_mutex_t CashCounter;
pthread_mutex_t timeCash;
pthread_cond_t thresholdCondCash;


//RANDOM ZONE GENERATOR
//Returns 1 for Zone A, 2 for Zone B and 3 for Zone C
int rndZoneGen(){
	int prob = rand()%100;
	printf("Chance for the zone..%d %%\n", prob);
	
	if(prob >= 60){
		return 3;
	}else if(prob >=20){
		return 2;
	}else{
		return 1;
	}
}

//RANDOM GENERATOR with limits
int rndGen(int low,int high){
    int result ;
    result= ( (rand() % (high+1-low) ) + low);
    return result;
}

//Checks if there are available tickets, returns -1 if not.
int _isFull(int tickets, int rndZone){

	int count = SEATNUM + 1;

	if(rndZone == 1){
		for(int i = 199; i<199 + SEAT*ZONE_A; i++){
			if(seatArray[i] == 0){
				count = i;
				break;
			}
		}
	}else if(rndZone == 2){
		for(int i = 99; i<99 + SEAT*ZONE_B; i++){
			if(seatArray[i] == 0){
				count = i;
				break;
			}		
		}
	}else if(rndZone == 3){
		for(int i = 0; i<SEAT*ZONE_C; i++){
			if(seatArray[i] == 0){
				count = i;
				break;
			}					
		}
	}

	//Returns -1 if not enough tickets available, otherwise
	//Returns a position of the array 
	if(rndZone == 1){
		if((count + tickets) <= 199+SEAT*ZONE_A){
			return count;
		}else{
			return -1;
		}
	}else if(rndZone == 2){
		if((count + tickets) <= 99+SEAT*ZONE_B){
			return count;
		}else{
			return -1;
		}
	}else if(rndZone == 3){
		if((count + tickets) <= SEAT*ZONE_C){
			return count;
		}else{
			return -1;
		}
	}
} 

//MAIN FUNCTION
void *customerServe(void *threadId) {
    
	//SEED
    srand(seed);

    struct timespec start, stop;
    struct timespec start2, stop2;
	struct timespec start3, stop3;

    int rc;

    /* MUTEX 0 for time start*/	
	rc = pthread_mutex_lock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}

    if( clock_gettime( CLOCK_REALTIME, &start) == -1 ) {
      perror( "clock gettime" );
      exit( EXIT_FAILURE );
    }

    rc = pthread_mutex_unlock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}


    /* MUTEX 1 for employee available counter*/	
	rc = pthread_mutex_lock(&TelCounter);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}


	while(currentTelInUse>=TEL){
		printf("Currently waiting for the first available Telephonist, please wait, %d ---\n", currentTelInUse);
        
		rc= pthread_cond_wait(&thresholdCond ,&TelCounter);
		if(rc!=0){
			printf("ERROR: return code from pthread_cond_wait() is %d\n", rc);
			pthread_exit(&rc);
		}
		printf("An available telephonist is found for customer\n");

	}

    currentTelInUse++;

	rc = pthread_mutex_unlock(&TelCounter);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}	

    /* MUTEX 1.2 for time end */	
	rc = pthread_mutex_lock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}

    if( clock_gettime( CLOCK_REALTIME, &stop) == -1 ) {
      perror( "clock gettime" );
      exit( EXIT_FAILURE );
    }

    waitingTime += ( stop.tv_sec - start.tv_sec ) + ( stop.tv_nsec - start.tv_nsec ) / BILLION;

    rc = pthread_mutex_unlock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}

    /* MUTEX 2 time of the whole call start */	
	rc = pthread_mutex_lock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}

    if( clock_gettime( CLOCK_REALTIME, &start2) == -1 ) {
      perror( "clock gettime" );
      exit( EXIT_FAILURE );
    }

    rc = pthread_mutex_unlock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}

    int rndSeats= rndGen(SEATLOW,SEATHIGH); //Number of seats
    int rndZone= rndZoneGen(); //Zone A = 1, Zone B = 2, Zone C = 3
    int rndSec= rndGen(T_LOW,T_HIGH); //Seconds to process the request
	int rndSecCach = rndGen(T_CASH_LOW,T_CASH_LOW); //Secons to process payment
	

    /* MUTEX 3 check availability of tickets*/	
    rc = pthread_mutex_lock(&ticketFinder);
    if (rc != 0) {	
        printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
        pthread_exit(&rc);
    }

    id_transaction++;
    
    int count = _isFull(rndSeats, rndZone);

	if(count != -1){

		/* MUTEX 4 waiting time cashier start*/	
		rc = pthread_mutex_lock(&timeCash);
		if (rc != 0) {	
			printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
			pthread_exit(&rc);
		}

		if( clock_gettime( CLOCK_REALTIME, &start3) == -1 ) {
		  perror( "clock gettime" );
		  exit( EXIT_FAILURE );
		}

		rc = pthread_mutex_unlock(&timeCash);
		if (rc != 0) {	
			printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
			pthread_exit(&rc);
		}


		/* MUTEX 5 cash counter */
        rc = pthread_mutex_lock(&CashCounter);
        if (rc != 0) {	
            printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
            pthread_exit(&rc);
        }

		while(currentCashInUse >= CASH){
			printf("Currently waiting for the first available Cachier, please wait, %d\n", currentCashInUse);
			rc= pthread_cond_wait(&thresholdCondCash ,&CashCounter);
			if(rc!=0){
				printf("ERROR: return code from pthread_cond_wait() is %d\n", rc);
				pthread_exit(&rc);
			}
			printf("An available telephonist is found for customer\n");
		}

		currentCashInUse++;

		rc = pthread_mutex_unlock(&CashCounter);
        if (rc != 0) {	
            printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
            pthread_exit(&rc);
        }
		

		/* MUTEX 4.1  waiting time cashier end */	
		rc = pthread_mutex_lock(&timeCash);
		if (rc != 0) {	
			printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
			pthread_exit(&rc);
		}

		if( clock_gettime( CLOCK_REALTIME, &stop3) == -1 ) {
		  perror( "clock gettime" );
		  exit( EXIT_FAILURE );
		}

		waitingTimeCash += rndSecCach + ( stop3.tv_sec - start3.tv_sec ) + ( stop3.tv_nsec - start3.tv_nsec ) / BILLION;

		rc = pthread_mutex_unlock(&timeCash);
		if (rc != 0) {	
			printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
			pthread_exit(&rc);
		}

		//PAYMENT 
        chance = rand()% 100;
        printf("Chance for the payment.. %d %%\n", chance);

        if (chance >= CARDSUCCESS){
            printf("Your payment failed! Sorry for the inconvenience..");

            /* MUTEX 6 calculate total time passed if payment is failed */	
            rc = pthread_mutex_lock(&timeMutex);
            if (rc != 0) {	
                printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
                pthread_exit(&rc);
            }

            if( clock_gettime( CLOCK_REALTIME, &stop2) == -1 ) {
            perror( "clock gettime" );
            exit( EXIT_FAILURE );
            }

            assistanceTime += waitingTimeCash + waitingTime + rndSec + ( stop.tv_sec - start.tv_sec ) + ( stop.tv_nsec - start.tv_nsec ) / BILLION;

            rc = pthread_mutex_unlock(&timeMutex);
            if (rc != 0) {	
                printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
                pthread_exit(&rc);
            }

			currentTelInUse--;
			currentCashInUse--;

            rc = pthread_mutex_unlock(&ticketFinder);
			
            pthread_exit(&rc);
        }

		
		for(int i = count; i< count + rndSeats; i++){
			seatArray[i] = id_transaction;
		}

		//currentTelInUse--; 
		//here

    
	//IF NO AVAILABLE SEATS
    } else if(count == -1){
		printf("All seats are reserved, thank you %d customer!:)\n", id_transaction);

        /* MUTEX 7 calculate total time passed if no available seats */	
        rc = pthread_mutex_lock(&timeMutex);
        if (rc != 0) {	
            printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
            pthread_exit(&rc);
        }

        if( clock_gettime( CLOCK_REALTIME, &stop2) == -1 ) {
        perror( "clock gettime" );
        exit( EXIT_FAILURE );
        }

        assistanceTime += waitingTimeCash + waitingTime + rndSec + ( stop2.tv_sec - start2.tv_sec ) + ( stop2.tv_nsec - start2.tv_nsec ) / BILLION;

        rc = pthread_mutex_unlock(&timeMutex);
        if (rc != 0) {	
            printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
            pthread_exit(&rc);
        }

		currentTelInUse--; //here

        rc = pthread_mutex_unlock(&ticketFinder);

        pthread_exit(&rc);
    }


    rc = pthread_mutex_unlock(&ticketFinder);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}

	/* MUTEX cash counter wake up */
	rc = pthread_mutex_lock(&CashCounter);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}
		
	
    if ( currentCashInUse < CASH){
        pthread_cond_signal(&thresholdCondCash);
    }

	currentCashInUse--;

	rc = pthread_mutex_unlock(&CashCounter);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}
	

     /* MUTEX 8 for balance  */
    rc = pthread_mutex_lock(&addToBalance);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}
    
	if(rndZone == 1){
		balance+=rndSeats*A_SEATCOST;
	}else if(rndZone == 2){
		balance+=rndSeats*B_SEATCOST;
	}else if(rndZone == 3){
		balance+=rndSeats*C_SEATCOST;
	}
   
	rc = pthread_mutex_unlock(&addToBalance);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}


    /* MUTEX 9 for prints */	
    rc = pthread_mutex_lock(&PrintMutex);
    if (rc != 0) {	
        printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
        pthread_exit(&rc);
    }

	//PRINTS IF THERE WERE AVAILABLE SEATS
    if( count!=-1 ){
        
		printf("Your Transaction id is :%d\n", id_transaction);
		printf("Your seats are succesfully reserved. We shall now process to payment\n");
		printf("Total cost of the transaction is: %d€\n", rndSeats*SEATCOST);
		printf("Your Seats reserved are: ");
        for(int i = count; i< count + rndSeats; i++){
            printf(" %d ", i );
		}
		printf("\n");
		
    }

	rc = pthread_mutex_unlock(&PrintMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}
    

    /* MUTEX 10 employee counter and wake up condition */
	rc = pthread_mutex_lock(&TelCounter);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}

    if ( currentTelInUse < TEL){
        pthread_cond_signal(&thresholdCond);
    }
	
	currentTelInUse--;

	rc = pthread_mutex_unlock(&TelCounter);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}	
	
    /* MUTEX 11 total time passed end */	
	rc = pthread_mutex_lock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_lock() is %d\n", rc);
		pthread_exit(&rc);
	}

    if( clock_gettime( CLOCK_REALTIME, &stop2) == -1 ) {
      perror( "clock gettime" );
      exit( EXIT_FAILURE );
    }

    assistanceTime += waitingTimeCash + waitingTime + rndSec + ( stop2.tv_sec - start2.tv_sec ) + ( stop2.tv_nsec - start2.tv_nsec ) / BILLION;

    rc = pthread_mutex_unlock(&timeMutex);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_unlock() is %d\n", rc);
		pthread_exit(&rc);
	}


   pthread_exit(threadId);

}



//MAIN
int main(int argc, char *argv[]) {

	int rc;
	
	
	//Checks if user gave the correct input
	if (argc != 3) {
		printf("ERROR: the program should take two arguments, the number of customers to create and the seed number!\n");
		exit(0);
	}
	

	customers = atoi(argv[1]);
	seed = atoi(argv[2]);
	

	
	//Checks if the value is a positive number, otherwise end program
	if (customers < 0) {
		printf("ERROR: the number of customers to run should be a positive number. Current number given %d.\n", customers);
		exit(-1);
	}
	if (seed < 0) {
		printf("ERROR: the number of seed to run should be a positive number. Current number given %d.\n", 8);
		exit(-1);
	}
	
	printf("Main: We will create %d threads for each customer.\n", customers);

	
	seatArray = (int *)malloc(sizeof(int) * SEATNUM);
	//elegxos an apetyxe i malloc
	if (seatArray == NULL) {
		printf("ERROR: Calloc failed not enough memory!\n");
		return -1;
	}
	//ARRAY INITIALIZATION, 
	//All elements are 0, all seats are empty
	for(int i = 0; i < SEATNUM; i++) {
   		seatArray[i]=0;
	}
    
	//CREATE MUTEX
	rc = pthread_mutex_init(&TelCounter, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_mutex_init(&addToBalance, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_mutex_init(&ticketFinder, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_mutex_init(&PrintMutex, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_mutex_init(&timeMutex, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_mutex_init(&CashCounter, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_mutex_init(&timeCash, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc= pthread_cond_init(&thresholdCond, NULL);
	if (rc!=0){
			printf("ERROR: return code from pthread_cond_init() is %d\n", rc);
			exit(-1);
	}

	rc= pthread_cond_init(&thresholdCondCash, NULL);
	if (rc!=0){
			printf("ERROR: return code from pthread_cond_init() is %d\n", rc);
			exit(-1);
	}



    pthread_t *threads = malloc(sizeof(pthread_t) * customers);
	int threadIds[customers];
	if (threads == NULL) {
		printf("ERROR: Failed to allocate threads , not enough memory!\n");
		return -1;
	}

	for (int i = 0; i < customers; i++) {
		threadIds[i] = i + 1;

    		rc = pthread_create(&threads[i], NULL, customerServe, &threadIds[i]);

    		if (rc != 0) {
    			printf("ERROR: return code from pthread_create() is %d\n", rc);
       			exit(-1);
       		}
	}

	void *status;
	for (int i = 0; i < customers; i++) {
		rc = pthread_join(threads[i], &status);
		
		if (rc != 0) {
			printf("ERROR: return code from pthread_join() is %d\n", rc);
			exit(-1);		
		}
		
	}

	//PRINT SEATS
    for (int i = 0; i < SEATNUM; i++) {

		if( seatArray[i] != 0){
			if(i>199 && i<=249){
				printf("Zone A / Seat %d / Costumer %d\n", i+1, *(seatArray + i));

			}else if(i>99 && i<=199){
				printf("Zone B / Seat %d / Costumer %d\n", i+1, *(seatArray + i));

			}else if(i>=0 && i<=99){
				printf("Zone C / Seat %d / Costumer %d\n", i+1, *(seatArray + i));
		
			}
		}
    }

	//PRINT INFO
	printf("\n");
    printf("The balance is: %d euros\n",balance);
    printf("Total transactions: %d\n",id_transaction);
    printf("Total waiting Time until reaching an employee: %f\n",waitingTime/customers/100);
	printf("Total waiting Time until reaching the cashier: %f\n",waitingTimeCash/customers/100);
    printf("Total waiting Time until reaching the end: %f\n",assistanceTime/customers/100);
    
	
	//DESTROY MUTEX
	rc = pthread_mutex_destroy(&TelCounter);
	if (rc != 0) {
   		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
      		exit(-1);
	}

	rc = pthread_mutex_destroy(&addToBalance);
	if (rc != 0) {
   		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
      		exit(-1);
	}

	rc = pthread_mutex_destroy(&ticketFinder);
	if (rc != 0) {
   		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
      		exit(-1);
	}

	rc = pthread_mutex_destroy(&PrintMutex);
	if (rc != 0) {
   		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
      		exit(-1);
	}

	rc = pthread_mutex_destroy(&timeMutex);
	if (rc != 0) {
   		printf("ERROR: return code from pthread_mutex_destroy() is %d\n", rc);
      		exit(-1);
	}

	rc = pthread_mutex_init(&CashCounter, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_mutex_init(&timeCash, NULL);
	if (rc != 0) {	
		printf("ERROR: return code from pthread_mutex_init() is %d\n", rc);
		exit(-1);
	}

	rc = pthread_cond_destroy(&thresholdCond);
	if (rc != 0) {
		printf("ERROR: return code from pthread_cond_destroy() is %d\n", rc);
		exit(-1);		
	}

	rc = pthread_cond_destroy(&thresholdCondCash);
	if (rc != 0) {
		printf("ERROR: return code from pthread_cond_destroy() is %d\n", rc);
		exit(-1);		
	}


	//DELETE MEMORY
	free(threads);
	free(seatArray);


	return 1;
}
