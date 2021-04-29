CC=g++
CFLAGS=-c -std=c++17 -Wall -Wextra -Wshadow -Wold-style-cast -pipe -I/usr/include/mariadb -isystem deps -fstack-protector \
-O2 -DNDEBUG
# -O0 -DDEBUG -g
# -O0 -DNDEBUG


LDFLAGS=-lmariadbclientpp -lmariadbclient -largon2 \
-laws-cpp-sdk-s3 -laws-cpp-sdk-core -laws-c-event-stream \
-laws-checksums -laws-c-common \
-lcurl -lssl -lcrypto -lpthread -lspdlog \
-O2
# -O0 -g

SOURCES=$(wildcard *.cpp) $(wildcard Server/*.cpp) $(wildcard DB/*.cpp)
DEPS=$(SOURCES:%.cpp=build/%.d)
OBJECTS=$(SOURCES:%.cpp=build/%.o)
EXECUTABLE=server


all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)

build/%.o: %.cpp
	@mkdir -p $(@D)
	$(CC) -MMD $(CFLAGS) $< -o $@

-include $(DEPS)

clean:
	-rm -r build/*

