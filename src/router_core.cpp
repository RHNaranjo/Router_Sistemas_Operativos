#include "../include/router_core.hpp"
#include <sstream>

//FUNCIONES PARA REALIZAR DESPUÉS
InfoInterfaz* RouterCore::get_interfaz(std::string& nombre){
    (void)nombre;
    InfoInterfaz* interfaz;
    return interfaz;
}

InfoRoute* RouterCore::set_route(std::string, std::string, std::string, std::string, std::string){
    InfoRoute* route;
    return route;
}


void RouterCore::init_default_state(){
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
}

void RouterCore::generar_running_config(){
    std::ostringstream oss;
    
    oss << "version " << version << std::endl;
    oss << "hostname " << hostname << std::endl;
    oss << std::endl;
    
    //Interfaces
    for(const auto& interfaz : interfaces) {
        oss << "interface: " << interfaz.nombre << std::endl;
        if(!interfaz.description.empty())
            oss << " description " << interfaz.description << std::endl;
        
        if(!interfaz.ip.empty())
            oss << " ip address " << interfaz.ip << std::endl;
        
        if(interfaz.up)
            oss << " no shutdown" << std::endl;
        else
            oss << " shutdown" << std::endl;
        
        oss << std::endl;
    }
    
    //OSPF por implementar
    oss << "!" << std::endl;
    oss << "end" << std::endl;
}

void RouterCore::actualizar_running_config(){
    return;
}

void RouterCore::process_password(std::string, bool&){
    return;
}