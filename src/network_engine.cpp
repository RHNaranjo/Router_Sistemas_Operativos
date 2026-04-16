#include "../include/network_engine.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

NetworkEngine::NetworkEngine(const std::string& router_name) : router_name_(router_name) {}

NetworkEngine::~NetworkEngine() {
    stop();
}

bool NetworkEngine::load_topology(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: No se pudo abrir el archivo de topología: " << filename << std::endl;
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string r_name, if_name, rem_ip;
        int loc_port, rem_port;

        // Formato: [Router] [Interface] [LocalPort] [RemoteIP] [RemotePort]
        if (!(ss >> r_name >> if_name >> loc_port >> rem_ip >> rem_port)) continue;

        // Solo cargar si la línea corresponde a este router
        if (r_name != router_name_) continue;

        InterfaceLink link;
        link.interface_name = if_name;
        link.local_port = loc_port;
        link.remote_ip = rem_ip;
        link.remote_port = rem_port;

        // Crear socket UDP
        link.socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (link.socket_fd < 0) {
            perror("Error creando socket");
            continue;
        }

        // Configurar puerto local (Bind)
        struct sockaddr_in loc_addr;
        std::memset(&loc_addr, 0, sizeof(loc_addr));
        loc_addr.sin_family = AF_INET;
        loc_addr.sin_addr.s_addr = INADDR_ANY;
        loc_addr.sin_port = htons(loc_port);

        if (bind(link.socket_fd, (struct sockaddr*)&loc_addr, sizeof(loc_addr)) < 0) {
            perror("Error en bind");
            close(link.socket_fd);
            continue;
        }

        // Configurar dirección remota para envíos rápidos
        std::memset(&link.remote_addr, 0, sizeof(link.remote_addr));
        link.remote_addr.sin_family = AF_INET;
        link.remote_addr.sin_port = htons(rem_port);
        inet_pton(AF_INET, rem_ip.c_str(), &link.remote_addr.sin_addr);

        links_[if_name] = link;
        std::cout << "[NetworkEngine] Interfaz " << if_name << " lista en puerto " << loc_port << " -> " << rem_ip << ":" << rem_port << std::endl;
    }

    return !links_.empty();
}

void NetworkEngine::start() {
    running_ = true;
    for (auto& pair : links_) {
        rx_threads_.emplace_back(&NetworkEngine::rx_loop, this, std::ref(pair.second));
    }
}

void NetworkEngine::stop() {
    running_ = false;
    for (auto& pair : links_) {
        if (pair.second.socket_fd >= 0) {
            close(pair.second.socket_fd);
        }
    }
    for (auto& thread : rx_threads_) {
        if (thread.joinable()) thread.join();
    }
    rx_threads_.clear();
}

bool NetworkEngine::send_packet(const std::string& interface_name, const SimulatedPacket& packet) {
    if (links_.find(interface_name) == links_.end()) return false;

    InterfaceLink& link = links_[interface_name];
    ssize_t sent = sendto(link.socket_fd, &packet, sizeof(packet), 0,
                          (struct sockaddr*)&link.remote_addr, sizeof(link.remote_addr));
    
    return sent == sizeof(packet);
}

void NetworkEngine::set_on_receive(PacketCallback callback) {
    on_receive_ = callback;
}

void NetworkEngine::rx_loop(InterfaceLink& link) {
    SimulatedPacket packet;
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);

    while (running_) {
        ssize_t rec = recvfrom(link.socket_fd, &packet, sizeof(packet), 0,
                               (struct sockaddr*)&sender_addr, &addr_len);
        
        if (rec > 0 && on_receive_) {
            // Verificar integridad básica
            if (std::memcmp(packet.header_id, "ROUT", 4) == 0) {
                on_receive_(link.interface_name, packet);
            }
        } else if (rec < 0 && running_) {
            // El socket se cerró o hubo error
            break;
        }
    }
}
