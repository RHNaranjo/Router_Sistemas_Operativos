#include "../include/router_core.hpp"
#include "../include/md5.hpp"
#include "../include/network_engine.hpp"
#include "../include/packet.hpp"
#include "../include/router_cli.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

// Método estático para expandir abreviaturas comunes de interfaces Cisco
std::string RouterCore::expandir_nombre_interfaz(const std::string &nombre) {
  if (nombre.rfind("Gig", 0) == 0 || nombre.rfind("gig", 0) == 0) {
    std::size_t pos = nombre.find_first_of("0123456789");
    if (pos != std::string::npos)
      return "GigabitEthernet" + nombre.substr(pos);
  }
  if (nombre.rfind("Se", 0) == 0 || nombre.rfind("se", 0) == 0) {
    std::size_t pos = nombre.find_first_of("0123456789");
    if (pos != std::string::npos)
      return "Serial" + nombre.substr(pos);
  }
  return nombre; // Sin cambios si no coincide con ninguna abreviatura
}

// Buscar interfaz por nombre expandiendo abreviaturas
InfoInterfaz *RouterCore::get_interfaz(const std::string &nombre) {
  std::string nombre_expandido = expandir_nombre_interfaz(nombre);
  for (auto &intf : interfaces) {
    if (intf.nombre == nombre_expandido)
      return &intf;
  }
  return nullptr; // Interfaz no encontrada
}

// Agregar una nueva ruta a la tabla de ruteo
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

  // GigabitEthernet0/1
  InfoInterfaz gig01;
  gig01.nombre = "GigabitEthernet0/1";
  gig01.ip = "";
  gig01.netmask = "";
  gig01.up = false;
  interfaces.push_back(gig01);

  // GigabitEthernet0/2
  InfoInterfaz gig02;
  gig02.nombre = "GigabitEthernet0/2";
  gig02.ip = "";
  gig02.netmask = "";
  gig02.up = false;
  interfaces.push_back(gig02);

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

  // OSPF
  ospf_config.active = false;
  ospf_config.process_id = "";
  ospf_config.router_id = "";
  ospf_config.networks.clear();
  ospf_config.passive_interfaces.clear();
}

// Generar configuración actual en formato de comandos Cisco
void RouterCore::generar_running_config() {
  std::ostringstream oss;

  oss << "version " << version << std::endl;
  oss << "hostname " << hostname << std::endl;
  oss << std::endl;

  // Seguridad
  if (enable_secret)
    oss << "enable secret 5 " << password << std::endl;

  if (!password.empty() && !enable_secret) {
    oss << "!" << std::endl;
    oss << "line console 0" << std::endl;
    oss << " password " << password << std::endl;
    if (login_local)
      oss << " login local" << std::endl;
    oss << "exit" << std::endl;
    oss << "!" << std::endl;
  }

  // Interfaces
  for (const auto &interfaz : interfaces) {
    oss << "interface " << interfaz.nombre << std::endl;
    if (!interfaz.description.empty())
      oss << " description " << interfaz.description << std::endl;

    if (!interfaz.ip.empty())
      oss << " ip address " << interfaz.ip << " " << interfaz.netmask
          << std::endl;

    if (interfaz.up)
      oss << " no shutdown" << std::endl;
    else
      oss << " shutdown" << std::endl;

    oss << "exit" << std::endl;
    oss << "!" << std::endl;
  }

  // DHCP
  for (const auto &pool : dhcp_pools) {
    oss << "ip dhcp pool " << pool.nombre << std::endl;
    if (!pool.red.empty())
      oss << " network " << pool.red << " " << pool.mascara << std::endl;
    if (!pool.gateway.empty())
      oss << " default-router " << pool.gateway << std::endl;
    oss << "exit" << std::endl;
    oss << "!" << std::endl;
  }

  // OSPF
  if (ospf_config.active) {
    oss << "router ospf " << ospf_config.process_id << std::endl;
    if (!ospf_config.router_id.empty())
      oss << " router-id " << ospf_config.router_id << std::endl;

    for (const auto &net : ospf_config.networks) {
      oss << " network " << net.network << " " << net.wildcard << " area "
          << net.area << std::endl;
    }
    for (const auto &p_intf : ospf_config.passive_interfaces) {
      oss << " passive-interface " << p_intf << std::endl;
    }
    oss << "exit" << std::endl;
    oss << "!" << std::endl;
  }

  oss << "end" << std::endl;

  // Asignar el texto generado al running_config
  running_config.texto = oss.str();
}

// Guardar configuración manteniendo las líneas de topología originales
void RouterCore::save_to_file(const std::string &filename) {
  std::vector<std::string> topology_lines;

  // 1. Leer el archivo original para extraer las líneas de topología
  {
    std::ifstream infile(filename);
    if (infile.is_open()) {
      std::string line;
      while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string first_token;
        if (!(ss >> first_token))
          continue;

        // Si el primer token es el nombre del router, es una línea de topología
        if (first_token == hostname) {
          topology_lines.push_back(line);
        }
      }
      infile.close();
    }
  }

  // 2. Generar la nueva configuración lógica
  generar_running_config();

  // 3. Escribir el archivo final combinando ambos
  std::ofstream outfile(filename);
  if (outfile.is_open()) {
    outfile << "# Topología física" << std::endl;
    for (const auto &topo : topology_lines) {
      outfile << topo << std::endl;
    }
    outfile << std::endl << "# Configuración lógica" << std::endl;
    outfile << running_config.texto;
    outfile.close();
  }
}

// Cargar y ejecutar comandos desde un archivo de configuración
// Cargar y ejecutar comandos desde un archivo de configuración
void RouterCore::load_from_file(const std::string &filename, RouterCLI &cli) {
  std::ifstream file(filename);
  if (!file.is_open())
    return;

  // Guardar modo actual y forzar GLOBAL_CONFIG para la carga
  CliMode modo_previo = cli.modo_actual;
  cli.modo_actual = CliMode::GLOBAL_CONFIG;

  std::string linea;
  std::string error;
  while (std::getline(file, linea)) {
    if (linea.empty() || linea[0] == '!' || linea.find("version") == 0 ||
        linea.find("#") == 0)
      continue;

    // Eliminar espacios al inicio
    size_t first = linea.find_first_not_of(" \t");
    if (first != std::string::npos) {
      linea = linea.substr(first);
    }

    if (linea == "end")
      break;

    CommandContexto contexto = cli.crear_contexto();
    const auto &arbol = cli.obtener_arbol_de_modo(cli.modo_actual);
    arbol.ejecutar_linea(contexto, linea, error);
  }

  // Restaurar modo inicial (usualmente USER_EXEC)
  cli.modo_actual = modo_previo;
}

void RouterCore::actualizar_running_config() { generar_running_config(); }

// Procesar y hashear contraseña si es necesario
void RouterCore::process_password(const std::string &pwd, bool hashear) {
  if (hashear) {
    password = md5(pwd);
    enable_secret = true;
  } else {
    password = pwd;
    enable_secret = false;
  }
}

// Procesar paquetes entrantes, ruteo, DHCP e ICMP
void RouterCore::handle_incoming_packet(const std::string &iface,
                                        const SimulatedPacket &pkt) {
  // 1. Detectar si el paquete es para este router (IP propia o Broadcast)
  bool es_para_mi = false;
  bool es_broadcast = (std::string(pkt.dst_ip) == "255.255.255.255");

  for (const auto &intf : interfaces) {
    if (intf.up && (intf.ip == pkt.dst_ip || es_broadcast)) {
      es_para_mi = true;
      break;
    }
  }

  if (es_para_mi) {
    // Si es ICMP (Ping), respondemos automáticamente (Echo Reply)
    if (pkt.protocol == 1) {
      if (std::string(pkt.payload).find("ECHO_REQUEST") != std::string::npos) {
        SimulatedPacket reply;
        reply.protocol = 1;
        std::strncpy(reply.src_ip, pkt.dst_ip, 16);
        std::strncpy(reply.dst_ip, pkt.src_ip, 16);
        std::strncpy(reply.payload, "ECHO_REPLY", 1024);
        reply.payload_len = std::strlen(reply.payload);

        if (net_engine) {
          net_engine->send_packet(iface, reply);
        }
      } else if (std::string(pkt.payload).find("ECHO_REPLY") !=
                 std::string::npos) {
        std::cout << "\n[ICMP] Reply from " << pkt.src_ip
                  << ": bytes=" << pkt.payload_len << " TTL=" << (int)pkt.ttl
                  << std::endl;
      }
    }

    // Si es DHCP (protocolo 67)
    if (pkt.protocol == 67) {
      std::string payload(pkt.payload);

      // DISCOVER -> OFFER
      if (payload.find("DHCP_DISCOVER") != std::string::npos) {
        // Buscar un pool que coincida con la red de la interfaz que recibió el
        // paquete
        InfoInterfaz *intf_ptr = get_interfaz(iface);
        if (!intf_ptr)
          return;

        for (auto &pool : dhcp_pools) {
          if (calcular_red(intf_ptr->ip, intf_ptr->netmask) == pool.red) {
            std::string offered_ip = asignar_ip(pool);
            if (offered_ip.empty())
              return;

            SimulatedPacket offer;
            offer.protocol = 67;
            std::strncpy(offer.src_ip, intf_ptr->ip.c_str(), 16);
            std::strncpy(offer.dst_ip, "255.255.255.255", 16);
            std::string msg = "DHCP_OFFER IP:" + offered_ip +
                              " MASK:" + pool.mascara + " GW:" + pool.gateway;
            std::strncpy(offer.payload, msg.c_str(), 1024);
            offer.payload_len = msg.length();

            if (net_engine)
              net_engine->send_packet(iface, offer);
            break;
          }
        }
      }
      // REQUEST -> ACK
      else if (payload.find("DHCP_REQUEST") != std::string::npos) {
        // Extraer la IP solicitada del payload (formato DHCP_REQUEST IP:A.B.C.D
        // CLIENT:Name)
        std::string requested_ip, client_name;
        size_t ip_pos = payload.find("IP:");
        size_t client_pos = payload.find("CLIENT:");

        if (ip_pos != std::string::npos && client_pos != std::string::npos) {
          requested_ip = payload.substr(ip_pos + 3, payload.find(" ", ip_pos) -
                                                        (ip_pos + 3));
          client_name = payload.substr(client_pos + 7);

          InfoInterfaz *intf_ptr = get_interfaz(iface);
          for (auto &pool : dhcp_pools) {
            if (calcular_red(intf_ptr->ip, intf_ptr->netmask) == pool.red) {
              pool.asignaciones[client_name] = requested_ip;

              SimulatedPacket ack;
              ack.protocol = 67;
              std::strncpy(ack.src_ip, intf_ptr->ip.c_str(), 16);
              std::strncpy(ack.dst_ip, "255.255.255.255", 16);
              std::string msg = "DHCP_ACK IP:" + requested_ip;
              std::strncpy(ack.payload, msg.c_str(), 1024);
              ack.payload_len = msg.length();

              if (net_engine)
                net_engine->send_packet(iface, ack);
              std::cout << "[DHCP] IP " << requested_ip << " asignada a "
                        << client_name << std::endl;
              break;
            }
          }
        }
      }
    }
    return;
  }

  // 2. Si no es para mí, intentamos el reenvío (Forwarding)
  InfoRoute *ruta = find_route(pkt.dst_ip);
  if (ruta) {
    if (net_engine) {
      // Verificar TTL antes de reenviar
      if (pkt.ttl <= 1) {
        // Enviar ICMP Time Exceeded
        SimulatedPacket error_pkt;
        error_pkt.protocol = 1;
        std::strncpy(error_pkt.src_ip, interfaces[0].ip.c_str(), 16);
        std::strncpy(error_pkt.dst_ip, pkt.src_ip, 16);
        std::strncpy(error_pkt.payload, "TIME_EXCEEDED", 1024);
        error_pkt.payload_len = 13;
        net_engine->send_packet(iface, error_pkt);
        return;
      }

      SimulatedPacket forwarded_pkt = pkt;
      forwarded_pkt.ttl--;
      net_engine->send_packet(ruta->interfaz, forwarded_pkt);
      return;
    }
  }

  // 3. Si no hay ruta, enviar ICMP Destination Unreachable
  if (net_engine &&
      std::string(pkt.payload).find("DHCP") == std::string::npos) {
    SimulatedPacket error_pkt;
    error_pkt.protocol = 1;
    std::strncpy(error_pkt.src_ip, interfaces[0].ip.c_str(), 16);
    std::strncpy(error_pkt.dst_ip, pkt.src_ip, 16);
    std::strncpy(error_pkt.payload, "DEST_UNREACHABLE", 1024);
    error_pkt.payload_len = 16;
    net_engine->send_packet(iface, error_pkt);
  }

  std::cout << "\n[Router] Drop: No hay ruta hacia " << pkt.dst_ip << std::endl;
}

InfoRoute *RouterCore::find_route(const std::string &dest_ip) {
  // Búsqueda simple de ruta (Longest Prefix Match simplificado)
  for (auto &ruta : rutas) {
    if (calcular_red(dest_ip, ruta.netmask) == ruta.destino) {
      return &ruta;
    }
  }
  return nullptr;
}

// Lógica de descubrimiento de rutas directamente conectadas
void RouterCore::recalcular_rutas_connected() {
  // 1. Eliminar rutas previas de tipo "C" (Connected)
  auto it = rutas.begin();
  while (it != rutas.end()) {
    if (it->protocolo == "C")
      it = rutas.erase(it);
    else
      ++it;
  }

  // 2. Para cada interfaz activa con IP, calcular su red y añadir ruta
  for (const auto &intf : interfaces) {
    if (intf.up && !intf.ip.empty() && !intf.netmask.empty()) {
      std::string red = calcular_red(intf.ip, intf.netmask);
      set_route(red, intf.netmask, "directly connected", intf.nombre, "C");
    }
  }
}

// Utilidad para calcular la dirección de red (A.B.C.D & M.M.M.M)
std::string RouterCore::calcular_red(const std::string &ip,
                                     const std::string &mask) {
  unsigned int i1, i2, i3, i4;
  unsigned int m1, m2, m3, m4;

  // Analizar las cadenas de texto para encontrar los cuatro octetos
  if (sscanf(ip.c_str(), "%u.%u.%u.%u", &i1, &i2, &i3, &i4) != 4)
    return "0.0.0.0";
  if (sscanf(mask.c_str(), "%u.%u.%u.%u", &m1, &m2, &m3, &m4) != 4)
    return "0.0.0.0";

  std::ostringstream oss;
  oss << (i1 & m1) << "." << (i2 & m2) << "." << (i3 & m3) << "." << (i4 & m4);
  return oss.str();
}

std::string RouterCore::asignar_ip(DHCPPool &pool) {
  // Configuración para DHCP
  unsigned int start[4], end[4];
  if (sscanf(pool.ip_inicio.c_str(), "%u.%u.%u.%u", &start[0], &start[1],
             &start[2], &start[3]) != 4)
    return "";
  if (sscanf(pool.ip_fin.c_str(), "%u.%u.%u.%u", &end[0], &end[1], &end[2],
             &end[3]) != 4)
    return "";

  for (unsigned int i = start[3]; i <= end[3]; ++i) {
    std::ostringstream oss;
    oss << start[0] << "." << start[1] << "." << start[2] << "." << i;
    std::string candidate = oss.str();

    bool asignada = false;
    for (auto const &[client, ip] : pool.asignaciones) {
      if (ip == candidate) {
        asignada = true;
        break;
      }
    }
    if (!asignada)
      return candidate;
  }
  return "";
}