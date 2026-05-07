#pragma once

#include "packet.hpp"
#include <atomic>
#include <functional>
#include <map>
#include <netinet/in.h>
#include <string>
#include <thread>
#include <vector>

/**
 * Representa un enlace físico simulado unido a una interfaz
 */
struct InterfaceLink {
  std::string interface_name;
  int local_port;
  std::string remote_ip;
  int remote_port;
  int socket_fd;
  struct sockaddr_in remote_addr;
};

/**
 * Motor de red basado en sockets UDP.
 * Cada interfaz del router se asocia a un puerto UDP local y un destino remoto.
 */
class NetworkEngine {
public:
  using PacketCallback = std::function<void(const std::string &interface_name,
                                            const SimulatedPacket &)>;

  NetworkEngine(const std::string &router_name);
  ~NetworkEngine();

  // Cargar topología desde un archivo de texto plano
  bool load_topology(const std::string &filename);

  // Iniciar hilos de recepción
  void start();

  // Detener motor
  void stop();

  // Enviar paquete por una interfaz específica
  bool send_packet(const std::string &interface_name,
                   const SimulatedPacket &packet);

  // Definir qué hacer cuando llega un paquete
  void set_on_receive(PacketCallback callback);

private:
  std::string router_name_;
  std::map<std::string, InterfaceLink> links_;
  std::vector<std::thread> rx_threads_;
  std::atomic<bool> running_{false};
  PacketCallback on_receive_;

  // Bucle de recepción para cada interfaz
  void rx_loop(InterfaceLink &link);
};
