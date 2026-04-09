#pragma once

#include <optional>
#include <string>
#include <vector>

struct InfoInterfaz {
  std::string nombre;
  std::string ip;
  std::string netmask;
  std::string description;
  bool up = false;
};

struct InfoOSPF {
  std::string router_id;
  std::string neighbor_ip;
  std::string state;
  std::string interfaz;
};

struct NetworkEntry {
  std::string network;
  std::string wildcard;
  int area;
};

struct OSPFConfig {
  std::string router_id;
  std::string process_id;
  std::vector<NetworkEntry> networks;
  std::vector<std::string> passive_interfaces;
  bool active = false;
};

struct InfoRoute {
  std::string destino;
  std::string netmask;
  std::string via;
  std::string interfaz;
  std::string protocolo;
};

struct ConfigSnapshot { // Para mostrar las configuraciones
  std::string texto;
};

class RouterCore {
public:
  std::string hostname = "Router";
  std::string version = "Router Sistemas Operativos 2.0";

  std::vector<InfoInterfaz> interfaces;
  std::vector<InfoOSPF> ospf_neighbors;
  std::vector<InfoRoute> rutas;
  OSPFConfig ospf_config;

  ConfigSnapshot running_config;
  std::optional<ConfigSnapshot>
      startup_config; // No es obligatorio tener una startup-conflict

  InfoInterfaz *get_interfaz(const std::string &nombre);
  InfoRoute *set_route(std::string destino, std::string netmask,
                       std::string via, std::string interfaz,
                       std::string protocolo);

  std::string password = "";
  bool login_local = false;
  bool enable_secret = false;

  // Helpers
  void init_default_state();
  void generar_running_config();
  void actualizar_running_config();
  void recalcular_rutas_connected();

  void process_password(const std::string &pwd, bool hashear);

private:
  std::string calcular_red(const std::string &ip, const std::string &mask);
};