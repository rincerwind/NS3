server_queue: server_queue.o queueLIFO.o queue.h
	gcc -lpthread -W -Wall -o server_queue server_queue.o queueLIFO.o

server_queue.o: queue.h
	gcc -c server_queue.c

queueLIFO.o: queue.h
	gcc -c queueLIFO.c

clean:
	rm -rf *.o