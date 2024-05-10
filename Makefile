CFLAGS = -std=c++17 -O2 -g -Wall
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

all: VulkanTest

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: run clean

run: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest

re: clean all
