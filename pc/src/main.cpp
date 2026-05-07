#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include "../../router/include/network_engine.hpp"
#include "../../router/include/packet.hpp"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Uso: ./pc <NOMBRE_PC> [ARCHIVO_TOPOLOGIA]" << std::endl;
        return 1;
    }

    std::string pc_name = argv[1];
    std::string topology_file = (argc > 2) ? argv[2] : "pc_topology.txt";

    std::cout << "=== Simulación de PC: " << pc_name << " ===" << std::endl;

    // Inicializar Motor de Red (reutilizando el del router por ahora)
    NetworkEngine net(pc_name);
    if (!net.load_topology(topology_file)) {
        std::cerr << "Error: No se pudo cargar la topología para " << pc_name << std::endl;
        return 1;
    }

    net.set_on_receive([](const std::string& iface, const SimulatedPacket& pkt) {
        std::cout << "\n[PC] Paquete recibido en " << iface << " desde " << pkt.src_ip << std::endl;
        // Aquí irá la lógica de respuesta DHCP / Ping
    });

    net.start();

    std::cout << "PC iniciada. Esperando eventos... (Ctrl+C para salir)" << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    net.stop();
    return 0;
}
