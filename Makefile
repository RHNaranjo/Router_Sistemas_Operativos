CXX = clang++
CXXFLAGS = -std=c++20 -pthread -Irouter/include

ROUTER_SRC = router/src/main.cpp router/src/router_core.cpp router/src/router_cli.cpp router/src/network_engine.cpp router/src/md5.cpp
PC_SRC = pc/src/main.cpp router/src/network_engine.cpp router/src/router_core.cpp router/src/router_cli.cpp router/src/md5.cpp

all: router_bin pc_bin

router_bin:
	$(CXX) $(CXXFLAGS) $(ROUTER_SRC) -o router_exe

pc_bin:
	$(CXX) $(CXXFLAGS) $(PC_SRC) -o pc_exe

clean:
	rm -f router_exe pc_exe
