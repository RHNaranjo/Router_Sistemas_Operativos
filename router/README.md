# Router Component

Este directorio contiene la implementación completa del emulador de Router.

## Estructura de Archivos

- **include/**
  - `router_core.hpp`: Definición del núcleo lógico, tablas de ruteo y configuración.
  - `router_cli.hpp`: Estructura del árbol de comandos y modos de consola (Cisco-style).
  - `network_engine.hpp`: Motor de comunicación basado en sockets UDP.
  - `packet.hpp`: Definición del formato binario de los paquetes simulados (`SimulatedPacket`).
- **src/**
  - `router_core.cpp`: Lógica de ruteo, manejo de interfaces y servidor DHCP.
  - `router_cli.cpp`: Implementación de los manejadores de comandos (Ping, DHCP, OSPF, etc.).
  - `network_engine.cpp`: Lógica de envío/recepción de paquetes y carga de topología.
  - `main.cpp`: Punto de entrada que inicializa los componentes.

## Funcionalidades Actuales

1. **CLI Robustecida**:
   - Soporta `User Exec`, `Privileged Exec`, `Global Config`, `Interface Config` y `DHCP Config`.
   - **Validación Preventiva**: El comando `interface` verifica la existencia física de la interfaz antes de entrar al modo de configuración.
   - **Navegación**: Inclusión del comando `end` para retornar rápidamente al modo privilegiado desde submodos.
2. **Servidor DHCP**: Capacidad de crear pools de red y asignar IPs dinámicamente a hosts (PCs).
3. **ICMP (Ping)**: Respuesta automática a paquetes Echo Request y ejecución de pings desde la CLI.
4. **Ruteo**: Tabla de rutas dinámicas y cálculo de redes (Longest Prefix Match).
5. **OSPF**: Estructura base para el protocolo de ruteo dinámico.

## Archivos de Configuración
- `config_router_1.txt` / `config_router_2.txt`: Configuraciones de topología estándar.
- `config_2pcs.txt`: Topología optimizada para el escenario multihost.

