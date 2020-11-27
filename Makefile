CC = gcc

objects = sync_car-runner.o sync_car_aux.o

sync_car: $(objects)
	$(CC) -pthread -o sync_car $(objects)

sync_car-runner.o: sync_car.h

sync_car_aux.o: sync_car.h

.PHONY: clean
clean:
	rm $(objects) sync_car