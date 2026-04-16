#pragma once

#include <cstdint>
#include <cstring>

/**
 * Estructura que emula un paquete de Capa 3 (IP) simplificado.
 * Se usará para enviar datos entre procesos de router y hosts.
 */
struct SimulatedPacket {
  char header_id[4];    // Identificador de protocolo "ROUT"
  uint8_t version;      // Versión del protocolo simulado
  uint8_t ttl;          // Time to Live
  uint8_t protocol;     // 1 = ICMP (Ping), 89 = OSPF, 0 = Datos
  char src_ip[16];      // IP origen en formato string "A.B.C.D"
  char dst_ip[16];      // IP destino en formato string "A.B.C.D"
  uint16_t payload_len; // Longitud de los datos
  char payload[1024];   // Datos del paquete

  SimulatedPacket() {
    std::memset(this, 0, sizeof(SimulatedPacket));
    std::memcpy(header_id, "ROUT", 4);
    version = 1;
    ttl = 64;
  }
};
