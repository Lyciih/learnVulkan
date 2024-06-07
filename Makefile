#CFLAGS = -std=c++17 -O2 -g -Wall
CFLAGS = -std=c++17 -O3 -g -Wall # 有需要載入模型時要用 -03
LDFLAGS = -lvulkan -lX11
#LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

all: VulkanTest

VulkanTest: main.cpp
	g++ $(CFLAGS) -o VulkanTest main.cpp $(LDFLAGS)

.PHONY: run clean

run: VulkanTest
	./VulkanTest

clean:
	rm -f VulkanTest

re: clean all
