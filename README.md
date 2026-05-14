# Emulador de Red: Router & PC (C++20)

Este proyecto es una simulación integral de una red corporativa, permitiendo la interacción entre instancias de Routers y PCs mediante comunicación real por sockets UDP. Emula el comportamiento de dispositivos Cisco con una CLI jerárquica, protocolos de red dinámicos y persistencia de configuración.

## Características Principales

- **Networking Real**: Comunicación basada en sockets UDP para simular enlaces físicos.
- **Seguridad MD5**: Implementación real del algoritmo MD5 para proteger el acceso al modo privilegiado (`enable secret`).
- **Protocolo DHCP**: Flujo DORA completo (Discover, Offer, Request, ACK) para asignación dinámica de IPs.
- **Diagnóstico Avanzado**: Soporte para `ping` y `traceroute` con manejo de mensajes ICMP (Echo, Time Exceeded, Destination Unreachable).
- **Persistencia**: Capacidad de guardar (`write`) y cargar configuraciones lógicas directamente en los archivos de topología.
- **Enrutamiento**: Soporte para rutas estáticas e interconexión entre múltiples routers.

## Estructura del Proyecto

```text
Router_Sistemas_Operativos/
├── router/             # Componentes del Router
│   ├── include/        # Headers (Core, CLI, Network, MD5)
│   └── src/            # Implementación (Core, CLI, Network, MD5)
├── pc/                 # Componentes de la PC (Host)
│   └── src/            # Cliente DHCP, lógica ICMP y CLI de PC
├── Makefile            # Compilación unificada
├── topo_corerouter.txt # Topología de red corporativa final
└── topo_R1.txt / R2    # Topología multi-router
```

## Compilación y Ejecución

### 1. Compilar

```bash
make all
```

Esto generará los binarios `router_exe` y `pc_exe`.

### 2. Ejecutar Escenario Corporativo (CoreRouter + 2 LANs)

Abre 3 terminales y ejecuta:

**Terminal 1 (Router):**

```bash
./router_exe CoreRouter topo_corerouter.txt
```

**Terminal 2 (PC Ventas):**

```bash
./pc_exe PC_Ventas topo_pc_ventas.txt
```

**Terminal 3 (PC Soporte):**

```bash
./pc_exe PC_Soporte topo_pc_soporte.txt
```

## Comandos Destacados

### En el Router

- `enable` / `configure terminal`: Navegación por modos de configuración.
- `enable secret <password>`: Configura contraseña con hash MD5.
- `ip dhcp pool <nombre>`: Configuración de servidores DHCP.
- `copy running-config startup-config` (o `write`): Persistencia a disco.
- `show ip interface brief` / `show ip route`: Diagnóstico de interfaces y rutas.

### En la PC

- `show ip interface brief`: Verifica la IP asignada por DHCP.
- `ping <IP>`: Prueba conectividad básica.
- `traceroute <IP>`: Rastrea los saltos (routers) hasta el destino.

## Tecnologías Utilizadas

- **Lenguaje**: C++20 con hilos POSIX (`std::thread`).
- **Comunicación**: Sockets UDP (Berkeley API).
- **Criptografía**: Implementación propia del algoritmo MD5.
- **Diseño**: Árboles N-arios (Tries) para parsing de comandos con soporte de abreviaturas.
