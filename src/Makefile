CC= colorgcc
OOPS = -c
OPS = -g -fstack-check -Wall 
THREADS = -pthread
NF = -lnetfilter_queue

CWD=`pwd`
VERSION=$(shell cat version)
BUILD=$(shell cat build)

DEBUG = $(OPS) -DDEBUG

DAEMON = $(OPS) -DDAEMON

all:daemon


queue: queue.c  logger.o  service.o server.o
	$(CC) $(OPS) -o qlb queue.c  service.o server.o logger.o $(THREADS) $(NF) -D'VERSION="$(VERSION)"' -D'BUILD="$(BUILD)"'
	echo `expr $(BUILD) + 1` > build

daemon: queue.c  logger.o service.o server.o
	$(CC) $(OPS) $(DAEMON) -o qlb queue.c service.o server.o logger.o $(THREADS) $(NF) -D'VERSION="$(VERSION)"' -D'BUILD="$(BUILD)"'
	echo `expr $(BUILD) + 1` > build

debug: queue.c syn.o
	$(CC) $(OPS) $(DEBUG) -o queue queue.c

service.o: service.c
	$(CC) $(OOPS) service.c

server.o: server.c
	$(CC) $(OOPS) server.c

logger.o: logger.c
	$(CC) $(OOPS) logger.c

clean:
	rm -rf queue *.o


install:
	mkdir /var/log/qlb
	cp qlbd /etc/init.d/
	ln -s $(CWD)/lbsts.py /usr/bin/lbsts
#	mkdir /usr/share/lbdt-common
#	cp -r lbdt-common /usr/share/
	ln -s $(CWD)/qlb /usr/bin/qlb

uninstall:
	rm -rf /usr/bin/qlb
	rm -rf /usr/bin/lbsts
	rm -rf /var/log/qlb
#	rm -rf /usr/share/lbdt-common
	rm /etc/init.d/qlbd
	rm -rf $(CWD)/lbdt

