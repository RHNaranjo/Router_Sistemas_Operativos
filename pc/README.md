# Componente PC (Host Simulation)

Este directorio contiene la simulación de un host o computadora conectada a la red, diseñada para interactuar con los routers del sistema.

## Estructura de Archivos

- **src/**
  - `main.cpp`: Implementación de la lógica del host y su interfaz interactiva.

## Funcionalidades del Host

1. **Cliente DHCP Automático**:
   - Al arrancar, la PC inicia el ciclo DORA buscando un servidor DHCP en el enlace local.
   - Configura dinámicamente su dirección IP, máscara y puerta de enlace.
2. **Diagnóstico Interactivo**:
   - **Ping**: Permite probar la conectividad hacia cualquier IP en la red.
   - **Traceroute**: Implementa el rastreo de ruta enviando paquetes con TTL incremental para identificar los saltos del router.
   - **Show IP Interface Brief**: Comando rápido para verificar el estado de la conexión y la IP asignada.
3. **Manejador ICMP**:
   - Responde automáticamente a paquetes `Echo Request`.
   - Procesa mensajes de error de red como `Time Exceeded` y `Destination Unreachable`.

## Funcionamiento
La PC está diseñada para ser "Zero Config" en el lado lógico. Solo requiere un archivo de topología física que defina a qué puerto de qué router está conectada mediante sockets UDP.

## Dependencias
Para mantener la consistencia del protocolo simulado, la PC reutiliza el `NetworkEngine` y las definiciones de `SimulatedPacket` ubicadas en el directorio del router.
