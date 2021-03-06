// SPDX-License-Identifier: GPL-3.0
/*
 * This file tests the implementation in sync_car.c.
 *
 * Note that passing these tests doesn't guarantee that your code is correct
 * or meets the specifications given, but hopefully it's at least pretty
 * close.
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>

#include "sync_car.h"

volatile int threads_completed = 0;

//----------------------------------------------------

pthread_mutex_t lock;
pthread_mutex_t lock2;


pthread_cond_t cond;

struct station {
	int waiting;
	int free_seats;

};

void station_load_car (struct station *station, int count){//count indica quantos assentos há no vagão

	if (count == 0);

	else{
		station->waiting = 1; //vagão chegou, com vagas, e está à espera
		station->free_seats = count;

		while(station->free_seats > 0){


			pthread_mutex_lock(&lock2);

			station->free_seats -= 1;

			pthread_mutex_unlock(&lock2);

			printf("Current free seats: %d\n", station->free_seats);

		}

		station->waiting = 0;

	}

}

void station_wait_for_car(struct station * station){

/*Quando um/a passageire chega à estação, inicialmente chama a função
station_wait_for_car (struct station *station)
, que não retorna até um vagão está na estação, ou seja, station_load_car está em execução,
e que haja assentos livres no carro para o/a passageire sentar-se.*/

	if(station->waiting != 0){
		pthread_mutex_lock(&lock);

		station->free_seats -=1;

		pthread_mutex_unlock(&lock);

	}
	else{
			pthread_mutex_lock(&lock);

			while(station->waiting == 0 )
				pthread_cond_wait(&cond, &lock);

			pthread_mutex_unlock(&lock);
		}


}

void station_on_board (struct station *station){

/*sua). Uma vez o/a passageire embarque, a seguinte função será chamada
station_on_board (struct station *station)
, indicando ao carro que o/a passageire embarcou.*/
	
}

void station_init(struct station *station){
	station->waiting = 0;
}

//----------------------------------------------------

void *passenger_thread(void *arg)
{
	struct station *station = (struct station *) arg;
	station_wait_for_car(station); // mutex "- pthread_mutex_init"
	__sync_add_and_fetch(&threads_completed, 1);
	return NULL;
}

struct load_car_args { //vagão
	struct station *station; //vagão presente na estação 
	int free_seats; 
};

volatile int load_car_returned = 0;

void *load_car_thread(void *args)
{
	struct load_car_args *ltargs = (struct load_car_args *) args;
	station_load_car(ltargs->station, ltargs->free_seats);
	load_car_returned = 1;
	return NULL;
}

const char *alarm_error_str;
int alarm_timeout;

void _alarm(int seconds, const char *error_str)
{
	alarm_timeout = seconds;
	alarm_error_str = error_str;
	alarm(seconds);
}

void alarm_handler(int foo)
{
	fprintf(stderr, "Error: Failed to complete after %d seconds. Something's "
                    "wrong, or your system is terribly slow. Possible error hint: [%s]\n",
            alarm_timeout, alarm_error_str);
	exit(1);
}

#ifndef MIN
#define MIN(_x, _y) ((_x) < (_y)) ? (_x) : (_y)
#endif

int main(void)
{

	pthread_mutex_init(&lock, NULL); //inicei mutex
	pthread_cond_init(&cond, NULL);

	struct station station;
	station_init(&station);

	int retorno;

	srandom(getpid() ^ time(NULL));

	signal(SIGALRM, alarm_handler);

	_alarm(1, "station_load_car() did not return immediately when no waiting passengers");
	station_load_car(&station, 0);
	station_load_car(&station, 10);
	_alarm(0, NULL);

	int i;
	const int total_passengers = 100;
	int passengers_left = total_passengers;
	for (i = 0; i < total_passengers; i++) {
		pthread_t tid;
		int ret = pthread_create(&tid, NULL, passenger_thread, &station);
		if (ret != 0) {
			perror("pthread_create");
			exit(1);
		}
	}

	_alarm(2, "station_load_car didn't return immediately when no free seats");
	station_load_car(&station, 0);
	_alarm(0, NULL);

	int total_passengers_boarded = 0;
	const int max_free_seats_per_car = 50;
	int pass = 0;
	while (passengers_left > 0) {
		_alarm(2, "Some more complicated issue appears to have caused passengers "
			"not to board when given the opportunity");

		int free_seats = random() % max_free_seats_per_car;

		printf("car entering station with %d free seats\n", free_seats);
		load_car_returned = 0;
		struct load_car_args args = {&station, free_seats};
		pthread_t lt_tid;
		int ret = pthread_create(&lt_tid, NULL, load_car_thread, &args); //chama um novo vagão

		if (ret != 0) {
			perror("pthread_create");
			exit(1);
		}

		int threads_to_reap = MIN(passengers_left, free_seats);
		int threads_reaped = 0;

		while (threads_reaped < threads_to_reap) {
			if (load_car_returned) {
				fprintf(stderr, "Error: station_load_car ");
				fprintf(stderr, "returned early!\n");
				exit(1);
			}
			if (threads_completed > 0) {
				if ((pass % 2) == 0)
					usleep(random() % 2);
				threads_reaped++;
				station_on_board(&station);
				__sync_sub_and_fetch(&threads_completed, 1);
			}
		}

		for (i = 0; i < 1000; i++) {
			if (i > 50 && load_car_returned)
				break;
			usleep(1000);
		}

		if (!load_car_returned) {
			fprintf(stderr, "Error: station_load_car failed\n");
			exit(1);
		}

		while (threads_completed > 0) {
			threads_reaped++;
			__sync_sub_and_fetch(&threads_completed, 1);
		}

		passengers_left -= threads_reaped;
		total_passengers_boarded += threads_reaped;
		printf("car departed station with %d new passenger(s) ",
		       threads_reaped);
		printf("(expected %d)%s\n", threads_to_reap,
		       (threads_to_reap != threads_reaped) ? " *****" : "");

		if (threads_to_reap != threads_reaped) {
			fprintf(stderr, "Error: Many passengers on this car\n");
			exit(1);
		}
		pass++;
	}

	if (total_passengers_boarded == total_passengers) {
		printf("Looks good!\n");
		retorno = 0;
	} else {
		fprintf(stderr, "Error: expected %d total boarded passengers, "
			, total_passengers);
		fprintf(stderr, "but got %d!\n", total_passengers_boarded);

		retorno = 1;
	}

	return retorno;
}
