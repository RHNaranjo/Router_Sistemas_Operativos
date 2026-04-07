#include "../include/router_core.hpp"
#include <sstream>

// Buscar una interfaz por nombre, con soporte de abreviaturas Cisco
InfoInterfaz *RouterCore::get_interfaz(const std::string &nombre) {
  // Lambda para expandir abreviaturas comunes de interfaces Cisco
  auto expandir_nombre = [](const std::string &n) -> std::string {
    if (n.rfind("Gig", 0) == 0 || n.rfind("gig", 0) == 0) {
      // Encontrar donde empieza el número (primer dígito)
      std::size_t pos = n.find_first_of("0123456789");
      if (pos != std::string::npos)
        return "GigabitEthernet" + n.substr(pos);
    }
    if (n.rfind("Se", 0) == 0 || n.rfind("se", 0) == 0) {
      std::size_t pos = n.find_first_of("0123456789");
      if (pos != std::string::npos)
        return "Serial" + n.substr(pos);
    }
    return n; // Sin cambios si no coincide con ninguna abreviatura
  };

  std::string nombre_expandido = expandir_nombre(nombre);

  for (auto &intf : interfaces) {
    if (intf.nombre == nombre_expandido)
      return &intf;
  }
  return nullptr; // Interfaz no encontrada
}

// Crear una ruta y agregarla al vector de rutas
InfoRoute *RouterCore::set_route(std::string destino, std::string netmask,
                                 std::string via, std::string interfaz,
                                 std::string protocolo) {
  InfoRoute nueva_ruta;
  nueva_ruta.destino = std::move(destino);
  nueva_ruta.netmask = std::move(netmask);
  nueva_ruta.via = std::move(via);
  nueva_ruta.interfaz = std::move(interfaz);
  nueva_ruta.protocolo = std::move(protocolo);

  rutas.push_back(std::move(nueva_ruta));
  return &rutas.back();
}

void RouterCore::init_default_state() {
  interfaces.clear();

  // GigabitEthernet0/0  (abrev. Gig0/0)
  InfoInterfaz gig00;
  gig00.nombre = "GigabitEthernet0/0";
  gig00.ip = "";
  gig00.netmask = "";
  gig00.up = false;
  interfaces.push_back(gig00);

  // GigabitEthernet0/0/0  (Gig0/0/0)
  InfoInterfaz gig000;
  gig000.nombre = "GigabitEthernet0/0/0";
  gig000.ip = "";
  gig000.netmask = "";
  gig000.up = false;
  interfaces.push_back(gig000);

  // GigabitEthernet0/0/1  (Gig0/0/1)
  InfoInterfaz gig001;
  gig001.nombre = "GigabitEthernet0/0/1";
  gig001.ip = "";
  gig001.netmask = "";
  gig001.up = false;
  interfaces.push_back(gig001);

  // Serial0/0/0  (Se0/0/0)
  InfoInterfaz s000;
  s000.nombre = "Serial0/0/0";
  s000.ip = "";
  s000.netmask = "";
  s000.up = false;
  interfaces.push_back(s000);

  // Serial0/0/1  (Se0/0/1)
  InfoInterfaz s001;
  s001.nombre = "Serial0/0/1";
  s001.ip = "";
  s001.netmask = "";
  s001.up = false;
  interfaces.push_back(s001);

  // Limpiar vecinos y rutas
  ospf_neighbors.clear();
  rutas.clear();
}

void RouterCore::generar_running_config() {
  std::ostringstream oss;

  oss << "version " << version << std::endl;
  oss << "hostname " << hostname << std::endl;
  oss << std::endl;

  // Seguridad
  if (enable_secret)
    oss << "enable secret (hashed)" << std::endl;

  if (!password.empty()) {
    oss << "!" << std::endl;
    oss << "line console 0" << std::endl;
    oss << " password " << password << std::endl;
    if (login_local)
      oss << " login local" << std::endl;
    oss << std::endl;
  }

  // Interfaces
  for (const auto &interfaz : interfaces) {
    oss << "interface: " << interfaz.nombre << std::endl;
    if (!interfaz.description.empty())
      oss << " description " << interfaz.description << std::endl;

    if (!interfaz.ip.empty())
      oss << " ip address " << interfaz.ip << " " << interfaz.netmask
          << std::endl;

    if (interfaz.up)
      oss << " no shutdown" << std::endl;
    else
      oss << " shutdown" << std::endl;

    oss << std::endl;
  }

  // OSPF por implementar
  oss << "!" << std::endl;
  oss << "end" << std::endl;

  // Asignar el texto generado al running_config
  running_config.texto = oss.str();
}

void RouterCore::actualizar_running_config() { generar_running_config(); }

void RouterCore::process_password(const std::string &pwd, bool hashear) {
  if (hashear) {
    // Por ahora se almacena con un marcador simple.
    // En una fase posterior se puede agregar hash MD5 real.
    password = "[hashed]" + pwd;
  } else {
    password = pwd;
  }
}