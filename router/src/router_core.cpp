#include "../include/router_core.hpp"
#include "../include/network_engine.hpp"
#include "../include/packet.hpp"
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

// Buscar una interfaz por nombre
InfoInterfaz *RouterCore::get_interfaz(const std::string &nombre) {
  std::string nombre_expandido = expandir_nombre_interfaz(nombre);
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

  // OSPF
  ospf_config.active = false;
  ospf_config.process_id = "";
  ospf_config.router_id = "";
  ospf_config.networks.clear();
  ospf_config.passive_interfaces.clear();
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

  // OSPF
  if (ospf_config.active) {
    oss << "!" << std::endl;
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
  }

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
    // Si la interfaz de salida es distinta a la de entrada (evitar loops
    // simples) O simplemente reenviar si es una red conocida
    if (net_engine) {
      // Decrementar TTL
      SimulatedPacket forwarded_pkt = pkt;
      if (forwarded_pkt.ttl > 1) {
        forwarded_pkt.ttl--;
        net_engine->send_packet(ruta->interfaz, forwarded_pkt);
        return;
      }
    }
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