#include "../include/network_engine.hpp"
#include "../include/router_cli.hpp"
#include "../include/router_core.hpp"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    std::cout << "Uso: " << argv[0] << " <NOMBRE_ROUTER> <ARCHIVO_TOPOLOGIA>"
              << std::endl;
    std::cout << "Ejemplo: ./router Router1 topology.txt" << std::endl;
    return 1;
  }

  std::string router_name = argv[1];
  std::string topology_file = argv[2];

  // Crear el core del router
  RouterCore core;
  core.hostname = router_name; // Sincronizar nombre
  core.init_default_state();
  core.generar_running_config();

  // Inicializar Motor de Red
  NetworkEngine net(router_name);
  if (!net.load_topology(topology_file)) {
    std::cerr << "Advertencia: No se cargó ninguna interfaz de red para este "
                 "router desde "
              << topology_file << std::endl;
  }

  // Vincular Core con Red
  core.net_engine = &net;
  net.set_on_receive(
      [&core](const std::string &iface, const SimulatedPacket &pkt) {
        core.handle_incoming_packet(iface, pkt);
      });

  // Iniciar recepción
  net.start();

  // Crear la CLI asociada al core
  RouterCLI cli(core);

  // Ejecutar
  cli.run();

  // Detener red antes de salir
  net.stop();

  return 0;
}