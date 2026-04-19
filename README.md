# Router_Sistemas_Operativos

Proyecto final para la materia de sistemas operativos. Emulador de router con capacidades de CLI (estilo Cisco), ruteo dinámico (OSPF) y comunicación real entre instancias mediante sockets UDP.

## Estructura del Proyecto

```
Router_Sistemas_Operativos/
├── include/
│   ├── packet.hpp           # Estructura de paquetes simulados
│   ├── network_engine.hpp   # Motor de red (UDP Sockets)
│   ├── router_core.hpp      # Núcleo lógico y estado
│   └── router_cli.hpp       # Interfaz de línea de comandos
├── src/
│   ├── main.cpp             # Punto de entrada
│   ├── network_engine.cpp   # Implementación de sockets
│   ├── router_core.cpp      # Lógica de ruteo y configuración
│   └── router_cli.cpp       # Manejadores de comandos
├── config_router_1.txt      # Topología para Router 1
├── config_router_2.txt      # Topología para Router 2
├── Makefile
└── README.md
```

## Compilación

El proyecto utiliza C++20 y soporte para hilos (pthread).

```bash
make compile
```

## Ejecución

El programa requiere el nombre del router y su archivo de topología para inicializar los sockets correctamente.

```bash
# Para el Router 1 (Terminal 1)
./router Router1 config_router_1.txt

# Para el Router 2 (Terminal 2)
./router Router2 config_router_2.txt
```

### Configuración de Red Real
El emulador permite interconexión real. Para que dos routers se hablen, configura sus interfaces en la misma subred:

**Router 1:**
```text
enable
configure terminal
interface Gig0/0
ip address 10.0.0.1 255.255.255.0
no shutdown
```

**Router 2:**
```text
enable
configure terminal
interface Gig0/0
ip address 10.0.0.2 255.255.255.0
no shutdown
```

## Comandos Disponibles

### Modo Usuario y Privilegiado
*   `enable` / `disable`: Cambio de privilegios.
*   `ping <IP>`: Envío de paquetes ICMP reales entre instancias.
*   `show ip interface brief`: Resumen de estado de interfaces.
*   `show ip route`: Visualización de la tabla de ruteo.
*   `show running-config`: Configuración actual en memoria.

### Modo Configuración Global
*   `hostname <name>`: Cambiar nombre del router.
*   `interface <name>`: Entrar a modo interfaz.
*   `router ospf <id>`: Entrar a modo OSPF.

### Modo Interfaz
*   `ip address <ip> <mask>`: Asignar dirección IP.
*   `no shutdown`: Activar la interfaz.
*   `description <text>`: Añadir descripción.

