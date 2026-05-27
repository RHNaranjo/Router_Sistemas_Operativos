#include "../../router/include/network_engine.hpp"
#include "../../router/include/packet.hpp"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main(int argc, char *argv[]) {
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
    std::cerr << "Error: No se pudo cargar la topología para " << pc_name
              << std::endl;
    return 1;
  }

  std::string current_ip = "0.0.0.0";

  std::string current_mask = "";

  bool bound = false;

  // Configurar manejador de paquetes entrantes (DHCP e ICMP)
  net.set_on_receive([&](const std::string &iface, const SimulatedPacket &pkt) {
    if (pkt.protocol == 67) { // DHCP
      std::string payload(pkt.payload);

      if (payload.find("DHCP_OFFER") != std::string::npos && !bound) {
        // Extraer IP ofrecida
        size_t ip_pos = payload.find("IP:");

        std::string offered_ip = payload.substr(
            ip_pos + 3, payload.find(" ", ip_pos) - (ip_pos + 3));

        std::cout << "[DHCP] Oferta recibida: " << offered_ip
                  << ". Enviando Request..." << std::endl;

        SimulatedPacket request;

        request.protocol = 67;

        std::strncpy(request.src_ip, "0.0.0.0", 16);
        std::strncpy(request.dst_ip, "255.255.255.255", 16);
        std::string msg =
            "DHCP_REQUEST IP:" + offered_ip + " CLIENT:" + pc_name;
        std::strncpy(request.payload, msg.c_str(), 1024);

        request.payload_len = msg.length();

        net.send_packet(iface, request);
      } else if (payload.find("DHCP_ACK") != std::string::npos && !bound) {
        size_t ip_pos = payload.find("IP:");
        current_ip = payload.substr(ip_pos + 3);
        bound = true;
        std::cout << "[DHCP] ¡Configuración completada! IP asignada: "
                  << current_ip << std::endl;
      }
    } else if (pkt.protocol == 1) { // ICMP
      std::string payload(pkt.payload);
      // Ignorar paquetes ICMP que no son para nosotros
      std::string dst(pkt.dst_ip);
      std::string src(pkt.src_ip);
      bool es_para_mi_icmp =
          (dst == current_ip || dst == "255.255.255.255");
      // Ignorar paquetes que nosotros mismos enviamos (loopback del motor)
      bool es_propio = (src == current_ip);

      if (payload.find("ECHO_REQUEST") != std::string::npos) {
        if (!es_para_mi_icmp || es_propio)
          return;
        std::cout << "\r[ICMP] Echo Request de " << pkt.src_ip
                  << ". Respondiendo..." << std::endl;
        std::cout << pc_name << "> " << std::flush;

        SimulatedPacket reply;
        reply.protocol = 1;

        std::strncpy(reply.src_ip, current_ip.c_str(), 16);
        std::strncpy(reply.dst_ip, pkt.src_ip, 16);
        std::strncpy(reply.payload, "ECHO_REPLY", 1024);

        reply.payload_len = 10;

        net.send_packet(iface, reply);
      } else if (payload.find("ECHO_REPLY") != std::string::npos) {
        std::cout << "\r[ICMP] Reply from " << pkt.src_ip
                  << ": bytes=32 TTL=" << (int)pkt.ttl << std::endl;
        std::cout << pc_name << "> " << std::flush;
      } else if (payload.find("TIME_EXCEEDED") != std::string::npos) {
        std::cout << "\r[ICMP] Time Exceeded from " << pkt.src_ip << std::endl;
        std::cout << pc_name << "> " << std::flush;
      } else if (payload.find("DEST_UNREACHABLE") != std::string::npos) {
        std::cout << "\r[ICMP] Destination Unreachable from " << pkt.src_ip
                  << std::endl;
        std::cout << pc_name << "> " << std::flush;
      }
    }
  });

  // Iniciar motor de red para comunicación física
  net.start();

  // Solicitar IP dinámicamente mediante broadcast DHCP Discover
  std::cout << "[DHCP] Iniciando descubrimiento..." << std::endl;

  SimulatedPacket discover;
  discover.protocol = 67;

  std::strncpy(discover.src_ip, "0.0.0.0", 16);
  std::strncpy(discover.dst_ip, "255.255.255.255", 16);

  std::string discover_msg = "DHCP_DISCOVER CLIENT:" + pc_name;

  std::strncpy(discover.payload, discover_msg.c_str(), 1024);

  discover.payload_len = discover_msg.length();

  // Suponemos que enviamos por la primera interfaz disponible
  net.send_packet("Ethernet0", discover);

  std::cout << "PC iniciada. Esperando eventos... (Ctrl+C para salir)"
            << std::endl;

  std::string linea;
  // Bucle de comandos interactivo para el usuario
  while (true) {
    std::cout << pc_name << "> ";
    if (!std::getline(std::cin, linea))
      break;

    if (linea.empty())
      continue;

    if (linea == "show ip interface brief") {
      std::cout << "\nInterface       IP-Address      Status       Protocol"
                << std::endl;
      printf("%-15s %-15s %-12s %-8s\n", "Ethernet0", current_ip.c_str(),
             bound ? "up" : "down", bound ? "up" : "down");
      continue;
    }

    if (linea == "shutdown") {
      std::cout << "Apagando PC..." << std::endl;
      break;
    }

    if (linea.rfind("ping ", 0) == 0) {
      std::string dest_ip = linea.substr(5);
      if (dest_ip.empty()) {
        std::cout << "Uso: ping <IP>" << std::endl;
        continue;
      }

      if (!bound) {
        std::cout << "ERROR: PC no tiene IP (esperando DHCP...)" << std::endl;
        continue;
      }

      std::cout << "Pinging " << dest_ip
                << " with 32 bytes of data:" << std::endl;
      // Enviar ráfaga de 4 pings ICMP
      for (int i = 0; i < 4; ++i) {
        SimulatedPacket pkt;
        pkt.protocol = 1; // ICMP

        std::strncpy(pkt.src_ip, current_ip.c_str(), 16);
        std::strncpy(pkt.dst_ip, dest_ip.c_str(), 16);
        std::strncpy(pkt.payload, "ECHO_REQUEST", 1024);

        pkt.payload_len = 12;

        if (!net.send_packet("Ethernet0", pkt)) {
          std::cout << "Request timed out (could not send)." << std::endl;
        } else {
          std::this_thread::sleep_for(std::chrono::milliseconds(400));
        }
      }
    } else if (linea.rfind("traceroute ", 0) == 0) {
      std::string dest_ip = linea.substr(11);

      if (dest_ip.empty()) {
        std::cout << "Uso: traceroute <IP>" << std::endl;
        continue;
      }

      if (!bound) {
        std::cout << "ERROR: PC no tiene IP." << std::endl;
        continue;
      }

      std::cout << "Tracing route to " << dest_ip
                << " over a maximum of 30 hops:" << std::endl;
      for (int ttl = 1; ttl <= 30; ++ttl) {
        SimulatedPacket pkt;
        pkt.protocol = 1; // ICMP
        pkt.ttl = ttl;

        std::strncpy(pkt.src_ip, current_ip.c_str(), 16);
        std::strncpy(pkt.dst_ip, dest_ip.c_str(), 16);
        std::strncpy(pkt.payload, "TRACEROUTE", 1024);

        pkt.payload_len = 10;

        std::cout << "  " << ttl << "  ";
        std::cout.flush();

        net.send_packet("Ethernet0", pkt);

        // Esperar respuesta (esto es simplificado, en realidad deberíamos
        // matchear el paquete en on_receive)
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Nota: El log de la respuesta aparecerá vía on_receive
      }
    } else {
      std::cout << "Comando no reconocido: " << linea << std::endl;
    }
  }

  net.stop();
  return 0;
}
