all: main

main: main.cpp request.cpp socket.cpp print.cpp cache.cpp response.cpp proxy.cpp
	g++ -g  -o main main.cpp request.cpp socket.cpp print.cpp cache.cpp response.cpp proxy.cpp -lpthread

.PHONY:
	clean
clean:
	rm -rf *.o main