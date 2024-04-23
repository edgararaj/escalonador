CC = gcc
CFLAGS = -Wall -g -Iinclude -ggdb
LDFLAGS =

all: folders server client

server: bin/orchestrator

client: bin/client

folders:
	@mkdir -p src include obj bin tmp

bin/orchestrator: obj/orchestrator.o obj/scheduler.o obj/status.o obj/mysystem.o obj/mysystem_pipe.o obj/time.o
	$(CC) $(LDFLAGS) $^ -o $@

bin/client: obj/client.o
	$(CC) $(LDFLAGS) $^ -o $@

obj/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f obj/* tmp/* bin/*

format:
# Check if clang is installed
	@command -v clang-format &> /dev/null || echo "[Warning] Please install `clang-format`. Read documentation."
# Run clang-format against the code files
	@find . -name '*.[ch]' -exec clang-format -i {} -i -style=file \; && echo "[Formating]"

check-memory:
# Check if valgrind is installed
	@command -v valgrind &> /dev/null || echo "[Warning] Please install `valgrind`. Read documentation."
# Run valgrind against the executables
	@DEBUG=1 make -s && echo "[Compiling]"
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./bin/orchestrator tmp 3 SJF
#	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose ./bin/client
