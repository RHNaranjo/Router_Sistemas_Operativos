# Componente Router

Este directorio contiene la implementación central del emulador de Router, diseñado para comportarse como un dispositivo de red profesional.

## Estructura de Archivos

- **include/**
  - `router_core.hpp`: Núcleo lógico (tablas de ruteo, DHCP pools, configuración).
  - `router_cli.hpp`: Motor de CLI jerárquica y árboles de comandos (Tries).
  - `network_engine.hpp`: Abstracción de capa física mediante sockets UDP.
  - `packet.hpp`: Definición del formato binario `SimulatedPacket`.
  - `md5.hpp`: Interfaz para el algoritmo de hashing.
- **src/**
  - `router_core.cpp`: Lógica de forwarding, DHCP, ICMP y persistencia.
  - `router_cli.cpp`: Manejadores de comandos (configuración y diagnóstico).
  - `network_engine.cpp`: Gestión de sockets, hilos de recepción y carga de topología.
  - `md5.cpp`: Implementación del algoritmo MD5.
  - `main.cpp`: Inicialización y orquestación del sistema.

## Funcionalidades Clave

1. **CLI Jerárquica**:
   - Modos: `User`, `Privileged`, `Global Config`, `Interface`, `Line` y `DHCP`.
   - Soporte de abreviaturas (ej: `sh ip int br` -> `show ip interface brief`).
2. **Seguridad Avanzada**:
   - `enable secret`: Almacenamiento de contraseñas mediante hash MD5 real.
   - Validación de acceso al modo privilegiado.
3. **Servicios de Red**:
   - **Servidor DHCP**: Flujo DORA completo para asignar IPs dinámicamente.
   - **Enrutamiento Estático**: Gestión de rutas manuales e interconexión multi-router.
4. **Diagnóstico ICMP**:
   - **Echo (Ping)**: Generación y respuesta de Echo Request/Reply.
   - **Traceroute**: Generación de `Time Exceeded` cuando el TTL llega a 0.
   - **Unreachable**: Notificación cuando no existe una ruta al destino.
5. **Persistencia (Write)**:
   - Capacidad de guardar la configuración lógica en el archivo de topología original, permitiendo la recuperación del estado tras un `reload`.

## Configuración y Topología
El router utiliza archivos de texto para definir sus conexiones físicas y su estado lógico inicial. Ejemplos incluidos: `topo_corerouter.txt`, `topo_R1.txt`.
