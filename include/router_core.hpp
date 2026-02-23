#include <string>
#include <vector>
#include <optional>

#pragma once

struct InfoInterfaz{
    std::string nombre;
    std::string ip;
    std::string netmask;
    std::string description;
    bool up = false;
};

struct InfoOSPF{
    std::string router_id;
    std::string neighbor_ip;
    std::string state;
    std::string interfaz;
};

struct InfoRoute{
    std::string destino;
    std::string netmask;
    std::string via;
    std::string interfaz;
    std::string protocolo;
};

struct ConfigSnapshot{ //Para mostrar las configuraciones
    std::string texto;
};

class RouterCore{
public:
    std::string hostname = "Router";
    std::string version = "Router Sistemas Operativos 0.1";
    
    std::vector<InfoInterfaz> interfaces;
    std::vector<InfoOSPF> ospf_neighbors;
    std::vector<InfoRoute> rutas;
    
    ConfigSnapshot running_config;
    std::optional<ConfigSnapshot> startup_config; //No es obligatorio tener una startup-conflict
    
    InfoInterfaz* get_interfaz(std::string&);
    InfoRoute* set_route(std::string destino, std::string netmask, std::string via, std::string interfaz, std::string protocolo);
    
    std::string password = "";
    bool login_local = false;
    bool enable_secret = false;
    
    //Helpers
    void init_default_state();
    void generar_running_config();
    void actualizar_running_config();
    
    void process_password(std::string, bool&);
};