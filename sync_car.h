#ifndef A_H
#define A_H

struct station {
	int waiting;
	int free_seats;
	int tempassageiro;

};

#endif


void *passenger_thread(void *arg);

void *load_car_thread(void *args);

void _alarm(int seconds, const char *error_str);

void alarm_handler(int foo);

void station_load_car (struct station *station, int count);

void station_wait_for_car(struct station * station);

void station_on_board (struct station *station);

void station_init(struct station *station);
