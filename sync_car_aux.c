#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "sync_car.h"

int total_que_partiu = 0;

pthread_mutex_t lock;
pthread_mutex_t lock2;

pthread_cond_t cond;

void station_load_car (struct station *station, int count){//count indica quantos assentos há no vagão

	if (count == 0);

	//printf("Partiram %d até agora.\n", total_que_partiu);

	if (total_que_partiu == 100)
		station->tempassageiro = 0;

	else if ((station->tempassageiro == 1) && (count != 0)){

		station->waiting = 1;
		station->free_seats = count;

		while(1){

			pthread_cond_broadcast(&cond);

			pthread_mutex_lock(&lock);    //eu nao estou conseguindo pegar a lock pela segunda vez

			if(total_que_partiu == 100){
				pthread_mutex_unlock(&lock);
				break;
			}

			if(station->free_seats == 0){
				station->waiting = 0;
				pthread_mutex_unlock(&lock);
				break;
			}
			pthread_mutex_unlock(&lock);
			//pthread_cond_signal(&cond);

			
		}


	}

}

void station_wait_for_car(struct station * station){

/*Quando um/a passageire chega à estação, inicialmente chama a função
station_wait_for_car (struct station *station)
, que não retorna até um vagão está na estação, ou seja, station_load_car está em execução,
e que haja assentos livres no carro para o/a passageire sentar-se.*/
	
	pthread_mutex_lock(&lock);
	station->tempassageiro = 1;

	while(station->waiting == 0)
		pthread_cond_wait(&cond, &lock);

	if(station->free_seats > 0){
		station->free_seats -= 1;
		printf("Entrei\n");

		if (station->free_seats == 0)
			station->waiting = 0;
	}

	pthread_mutex_unlock(&lock);

}

void station_on_board (struct station *station){

/*sua). Uma vez o/a passageire embarque, a seguinte função será chamada
station_on_board (struct station *station)
, indicando ao carro que o/a passageire embarcou.*/

	//printf("Entrei. Agora tem %d vagas\n", station->free_seats);

	pthread_mutex_lock(&lock2);
	total_que_partiu += 1;
	pthread_mutex_unlock(&lock2);
	
}

void station_init(struct station *station){
	station->waiting = 0;
}

//----------------------------------------------------