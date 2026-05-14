# Escenario Avanzado: Interconexión Multi-Router (2 Routers + 3 PCs)

Este escenario demuestra la capacidad de interconexión entre routers y la gestión de múltiples subredes distribuidas.

## Estructura de la Red

1. **R1 (Router Central)**: Gateway de la LAN A y enlace hacia R2.
2. **R2 (Router Sucursal)**: Gateway de la LAN B (con dos PCs) y enlace hacia R1.
3. **PC1**: En la LAN A (`192.168.1.0/24`).
4. **PC2 y PC3**: En la LAN B (`172.16.1.0/24`).

---

## Archivos de Configuración

### 1. `topo_R1.txt`
```text
# Topología física
R1 GigabitEthernet0/0 5000 127.0.0.1 6000
R1 GigabitEthernet0/1 5001 127.0.0.1 5002

# Configuración lógica
hostname R1
enable secret cisco123
!
interface GigabitEthernet0/0
 description LAN_A
 ip address 192.168.1.1 255.255.255.0
 no shutdown
exit
!
interface GigabitEthernet0/1
 description ENLACE_R1_R2
 ip address 10.0.0.1 255.255.255.252
 no shutdown
exit
!
ip dhcp pool Pool_A
 network 192.168.1.0 255.255.255.0
 default-router 192.168.1.1
exit
!
# Ruta hacia la LAN B a través de R2
ip route 172.16.1.0 255.255.255.0 10.0.0.2
end
```

### 2. `topo_R2.txt`
```text
# Topología física
R2 GigabitEthernet0/1 5002 127.0.0.1 5001
R2 GigabitEthernet0/0 5003 127.0.0.1 6001
R2 GigabitEthernet0/2 5004 127.0.0.1 6002

# Configuración lógica
hostname R2
enable secret cisco456
!
interface GigabitEthernet0/1
 description ENLACE_R2_R1
 ip address 10.0.0.2 255.255.255.252
 no shutdown
exit
!
interface GigabitEthernet0/0
 description LAN_B_H1
 ip address 172.16.1.1 255.255.255.0
 no shutdown
exit
!
interface GigabitEthernet0/2
 description LAN_B_H2
 ip address 172.16.1.2 255.255.255.0
 no shutdown
exit
!
ip dhcp pool Pool_B
 network 172.16.1.0 255.255.255.0
 default-router 172.16.1.1
exit
!
# Ruta hacia la LAN A a través de R1
ip route 192.168.1.0 255.255.255.0 10.0.0.1
end
```

### 3. `topo_PC1.txt`
```text
PC1 Ethernet0 6000 127.0.0.1 5000
```

### 4. `topo_PC2.txt`
```text
PC2 Ethernet0 6001 127.0.0.1 5003
```

### 5. `topo_PC3.txt`
```text
PC3 Ethernet0 6002 127.0.0.1 5004
```

---

## Ejecución

1. **Terminal 1**: `./router_exe R1 topo_R1.txt`
2. **Terminal 2**: `./router_exe R2 topo_R2.txt`
3. **Terminal 3**: `./pc_exe PC1 topo_PC1.txt`
4. **Terminal 4**: `./pc_exe PC2 topo_PC2.txt`
5. **Terminal 5**: `./pc_exe PC3 topo_PC3.txt`

---

## Nota sobre Seguridad
El comando `enable secret` actualmente almacena la contraseña de forma simulada. Aunque el código está preparado para integración, **no utiliza MD5 real todavía**; se comporta como una cadena de texto plana para propósitos de esta versión del simulador.
