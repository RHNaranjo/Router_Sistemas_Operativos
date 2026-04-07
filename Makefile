compile:
	clang++ -std=c++20 -Iinclude src/main.cpp src/router_core.cpp src/router_cli.cpp -o router