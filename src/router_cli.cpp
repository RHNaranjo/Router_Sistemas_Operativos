#include "../include/router_cli.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <thread> //Para simular ping
#include <chrono> //Para simular ping

//Buscar o crear un nuevo comando
void ArbolComandos::nuevo_comando(const std::vector<std::string>& keywords, const std::string& help, CommandHandler handler){
    //Obtener el nodo raíz
    CommandNodo* actual = raiz.get();

    for (const auto& kw : keywords) {
        //Buscar el hijo con la misma palabra clave
        CommandNodo* encontrado = nullptr;
        
        //Revisa cada uno de los hijos del nodo actual
        for(auto& hijo : actual->children) {
            if(hijo->keyword == kw) {
                encontrado = hijo.get();
                break;
            }
        }

        if(!encontrado) {
            //Crear el nuevo nodo en caso de que no se haya encontrado
            auto nodo = std::make_unique<CommandNodo>();
            nodo->keyword = kw;
            nodo->help = ""; //Esto se rellenará en las hojas de los nodos
            
            //Construyendo el nuevo nodo al final
            actual->children.emplace_back(std::move(nodo));

            //Puntero al último nodo hijo
            encontrado = actual->children.back().get();
        }
        actual = encontrado;
    }

    actual->es_hoja = true;
    actual->help = help;
    actual->handler = std::move(handler);

}

//Tokenizar los comandos
std::vector<std::string> ArbolComandos::tokenize(const std::string& linea){
    //Leer las cadena como flujo de entrada
    std::istringstream iss(linea);
    //Vector para guardar tokens
    std::vector<std::string> tokens;
    std::string tok;
    
    //Recorre cada palabra de la frase y la añade al vector de tokens
    while(iss >> tok)
        tokens.push_back(tok);
    
    return tokens;
}

//Permitir abreviaturas (sh == show / en == enable)
const CommandNodo* ArbolComandos::detectar_comando(const std::vector<std::string>& tokens, std::vector<const CommandNodo*>& path, std::string& mensaje_error) const {
    //Obtener el nodo raiz
    const CommandNodo* actual = raiz.get();
    std::size_t index = 0;
    
    //Recorrer tokens para avanzar en árbol
    for(; index < tokens.size(); ++index){
        const auto& token = tokens[index];
        std::vector<const CommandNodo*> detectados;
        
        for(const auto& hijo : actual->children)
            if(hijo->keyword.rfind(token, 0) == 0)
                detectados.push_back(hijo.get());
        
        if(detectados.empty()){
            //Si no hay nodo hoja, hay un error de comandos
            if(path.empty()){
                mensaje_error = "Comando no reconocido: " + token;
                return nullptr;
            }
            
            //Si ya hay un comando reconocido, el resto son argumentos
            break;
        }
        
        if(detectados.size() > 1){
            mensaje_error = "Comando ambiguo: " + token;
            return nullptr;
        }
        
        actual = detectados[0];
        path.push_back(actual);
    }
    
    //No se reconoce comando
    if(path.empty()){
        mensaje_error = "Comando no reconocido";
        return nullptr;
    }
    
    //El último nodo del path debe ser una hoja
    if(!path.back()->es_hoja){
        mensaje_error = "El comando está incompleto";
        return nullptr;
    }
    
    return path.back();
}

//Ejecutar el comando
bool ArbolComandos::ejecutar_linea(CommandContexto& contexto, const std::string& linea, std::string& mensaje_error) const {
    auto tokens = tokenize(linea);
    if(tokens.empty())
        return true; //La línea está vacía, pero eso no es error
    
    std::vector<const CommandNodo*> path;
    
    //Detectar el comando
    const CommandNodo* comando = detectar_comando(tokens, path, mensaje_error);
    
    //Si el comando es nullptr
    if (!comando)
        return false;
    
    //Invocar al handler del comando
    if(comando->handler) {
        comando->handler(contexto, tokens);
        return true;
    }
    
    mensaje_error = "ERROR: Comando sin implementación";
    return false;
}



// ------- CLI DEL ROUTER --------
RouterCLI::RouterCLI(RouterCore& core) : core_(core) {
    registrar_comandos_user_exec();
    registrar_comandos_priv_exec();
    registrar_comandos_global_cfg();
    registrar_comandos_line_cfg();
    registrar_comandos_if_cfg();
    registrar_comandos_ospf_cfg();
}

CommandContexto RouterCLI::crear_contexto() const {
    return CommandContexto{
        modo_actual, 
        const_cast<RouterCore*>(&core_) //Eliminar el 'const' para que no haya problema con el formato
    };
}

const ArbolComandos& RouterCLI::obtener_arbol_de_modo(CliMode modo) const {
    switch(modo){
        case CliMode::USER_EXEC:
            return arbol_user_exec;
        
        case CliMode::PRIVILEGED_EXEC:
            return arbol_priv_exec; 
        
        case CliMode::GLOBAL_CONFIG:
            return arbol_global_cfg;
        
        case CliMode::LINE_CONFIG:
            return arbol_line_cfg;
        
        case CliMode::INTERFACE_CONFIG:
            return arbol_if_cfg;
        
        case CliMode::ROUTER_OSPF_CONFIG:
            return arbol_ospf_cfg;
    }

    //En caso de que no se obtenga el contexto, se empieza en lo más básico
    return arbol_user_exec;
}

std::string RouterCLI::prompt() const {
    switch(modo_actual){
        case CliMode::USER_EXEC:
            return core_.hostname + ">";
        
        case CliMode::PRIVILEGED_EXEC:
            return core_.hostname + "#";
        
        case CliMode::GLOBAL_CONFIG:
            return core_.hostname + "(config)#";
        
        case CliMode::LINE_CONFIG:
            return core_.hostname + "(config-line)#";
        
        case CliMode::INTERFACE_CONFIG:
            return core_.hostname + "(config-if)#";
        
        case CliMode::ROUTER_OSPF_CONFIG:
            return core_.hostname + "(config-router)#";
    }

    //En caso de que no se obtenga el contexto, se empieza en lo más básico
    return "Router>";
}

void RouterCLI::run(){
    std::string linea;
    std::cout << "\n=== " << core_.hostname << " Router Sistemas Operativos ===" << std::endl;
    std::cout << "Escribe 'help' para ver los comandos disponibles\n" << std::endl;
    
    while(true) {
        std::cout << prompt() << " ";
        if(!std::getline(std::cin, linea))
            break;
        
        CommandContexto contexto = crear_contexto();
        std::string error;
    
        const auto& arbol = obtener_arbol_de_modo(modo_actual);
        
        bool ok = arbol.ejecutar_linea(contexto, linea, error);
        
        if(!ok && !error.empty())
            std::cout << "ERROR: " << error << std::endl;
    }
}


// ------- REGISTRO DE COMANDOS --------
void RouterCLI::registrar_comandos_user_exec(){
    //Enable
    arbol_user_exec.nuevo_comando(
        {"enable"}, //comando
        "Entrar a modo priv exec.", //descripción
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_enable(contexto, tokens);
        }
    );
    
    //Exit
    arbol_user_exec.nuevo_comando(
        {"exit"},
        "Cerrar sesión",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit(contexto, tokens);
        }
    );
    
    //Ping
    arbol_user_exec.nuevo_comando(
        {"ping"},
        "Enviar ICMP a otra dirección IP",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_ping(contexto, tokens);
        }
    );
    
    //Help
    arbol_user_exec.nuevo_comando(
        {"help"},
        "Mostrar ayuda",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_help(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_priv_exec(){
    //Disable
    arbol_priv_exec.nuevo_comando(
        {"disable"},
        "Volver a modo user exec.",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_disable(contexto, tokens);
        }
    );
    
    //Configure Terminal
    arbol_priv_exec.nuevo_comando(
        {"configure", "terminal"},
        "Entrar a modo de configuración global",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_configure_terminal(contexto, tokens);
        }
    );
    
    //Show version
    arbol_priv_exec.nuevo_comando(
        {"show", "version"},
        "Mostrar la versión del router",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_version(contexto, tokens);
        }
    );
    
    //Show running config
    arbol_priv_exec.nuevo_comando(
        {"show", "running-config"},
        "Mostrar la configuración en ejecución",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_running_config(contexto, tokens);
        }
    );
    
    //Show startup config
    arbol_priv_exec.nuevo_comando(
        {"show", "startup-config"},
        "Mostrar configuración de inicio",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_startup_config(contexto, tokens);
        }
    );
    
    //Show ip interface brief
    arbol_priv_exec.nuevo_comando(
        {"show", "ip", "interface", "brief"},
        "Mostrar resumen de las interfaces IP",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_interface_brief(contexto, tokens);
        }
    );
    
    //Show ip ospf neighbor
    arbol_priv_exec.nuevo_comando(
        {"show", "ip", "ospf", "neighbor"},
        "Mostrar los vecinos OSPF",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_ospf_neighbor(contexto, tokens);
        }
    );
    
    //Show ip ospf interface
    arbol_priv_exec.nuevo_comando(
        {"show", "ip", "ospf", "interface"},
        "Mostrar las interfaces de OSPF",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_ospf_interface(contexto, tokens);
        }
    );
    
    //Show ip route
    arbol_priv_exec.nuevo_comando(
        {"show", "ip", "route"},
        "Mostrar la tabla de enrutamiento",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_route(contexto, tokens);
        }
    );

    //Exit
    arbol_priv_exec.nuevo_comando(
        {"exit"},
        "Regresar a modo user exec",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_disable(contexto, tokens);
        }
    );
    
    //Copy running config startup config
    arbol_priv_exec.nuevo_comando(
        {"copy", "running-config", "startup-config"},
        "Guardar la configuración actual",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_copy_running_config_startup_config(contexto, tokens);
        }
    );
    
    //Write
    arbol_priv_exec.nuevo_comando(
        {"write"},
        "Guardar la configuración actual",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_copy_running_config_startup_config(contexto, tokens);
        }
    );
    
    //Reload
    arbol_priv_exec.nuevo_comando(
        {"reload"},
        "Reiniciar el router",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_reload(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_global_cfg(){
    //Hostname
    arbol_global_cfg.nuevo_comando(
        {"hostname"},
        "Configurar el nombre del router",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_hostname(contexto, tokens);
        }
    );
    
    //Exit
    arbol_global_cfg.nuevo_comando(
        {"exit"},
        "Volver al modo privilegiado",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_global(contexto, tokens);
        }
    );
    
    //End
    arbol_global_cfg.nuevo_comando(
        {"end"},
        "Volver al modo privilegiado",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_end(contexto, tokens);
        }
    );
    
    //Enable secret
    arbol_global_cfg.nuevo_comando(
        {"enable", "secret"},
        "Habilitar hashing con MD5", //Esto se configurará después, jeje
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_hostname(contexto, tokens);
        }
    );
    
    //Line console 0
    arbol_global_cfg.nuevo_comando(
        {"line", "console", "0"},
        "Habilitar configuración de linea", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_line_console_0(contexto, tokens);
        }
    );
    
    //Interface
    arbol_global_cfg.nuevo_comando(
        {"line", "console", "0"},
        "Habilitar configuración de linea", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_line_console_0(contexto, tokens);
        }
    );
    
    //Router OSPF
    arbol_global_cfg.nuevo_comando(
        {"router", "ospf"},
        "Ingresar a la configuración de OPSF", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_router_ospf(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_line_cfg(){
    //Password
    arbol_line_cfg.nuevo_comando(
        {"password"},
        "Añadir una contraseña al router", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_password(contexto, tokens);
        }
    );
    
    //Login local
    arbol_line_cfg.nuevo_comando(
        {"login", "local"},
        "Forzar a la autenticación de usuarios", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_login_local(contexto, tokens);
        }
    );
    
    //Exit
    arbol_line_cfg.nuevo_comando(
        {"exit"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_global_specific(contexto, tokens);
        }
    );
    
    //End
    arbol_line_cfg.nuevo_comando(
        {"end"},
        "Volver al modo privilegiado",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_end(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_if_cfg(){
    //Ip address
    arbol_if_cfg.nuevo_comando(
        {"ip", "address"},
        "Configurar dirección IP del router", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_ip_address(contexto, tokens);
        }
    );
    
    //No shutdown
    arbol_if_cfg.nuevo_comando(
        {"no", "shutdown"},
        "Activar la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_no_shutdown(contexto, tokens);
        }
    );
    
    //Description
    arbol_if_cfg.nuevo_comando(
        {"description"},
        "Descripción de la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_description(contexto, tokens);
        }
    );
    
    //Shutdown
    arbol_if_cfg.nuevo_comando(
        {"shutdown"},
        "Desactivar la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_shutdown(contexto, tokens);
        }
    );
    
    //Exit
    arbol_if_cfg.nuevo_comando(
        {"exit"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_global_specific(contexto, tokens);
        }
    );
    
    //End
    arbol_if_cfg.nuevo_comando(
        {"end"},
        "Volver al modo privilegiado",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_end(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_ospf_cfg(){
    //Network
    arbol_ospf_cfg.nuevo_comando(
        {"network"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_network(contexto, tokens);
        }
    );
    
    //Router-id
    arbol_ospf_cfg.nuevo_comando(
        {"router-id"},
        "Registrar el router-id OSPF", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_router_id(contexto, tokens);
        }
    );
    
    //Passive-interface
    arbol_ospf_cfg.nuevo_comando(
        {"passive-interface"},
        "Detener los paquetes 'hello'", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_passive_interface(contexto, tokens);
        }
    );
    
    //No passive-interface
    arbol_ospf_cfg.nuevo_comando(
        {"no", "passive-interface"},
        "Activa los paquetes 'hello' en la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_no_passive_interface(contexto, tokens);
        }
    );
    
    //Exit
    arbol_ospf_cfg.nuevo_comando(
        {"exit"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_global_specific(contexto, tokens);
        }
    );
    
    //End
    arbol_ospf_cfg.nuevo_comando(
        {"end"},
        "Volver al modo privilegiado",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_end(contexto, tokens);
        }
    );
}


// ------- HANDLERS USER EXEC --------
void RouterCLI::handle_enable(const CommandContexto&, const std::vector<std::string>&){
    //Se cambia de user exec a priv exec
    modo_actual = CliMode::PRIVILEGED_EXEC;
}

void RouterCLI::handle_exit(const CommandContexto&, const std::vector<std::string>&){
    //Cerrando sesión
    std::cout << "\nSaliendo del router..." << std::endl;
    std::exit(0);
}

void RouterCLI::handle_ping(const CommandContexto&, const std::vector<std::string>& tokens){
    //Tokens[0] == "ping"
    //Tokens[1] == "192.168.0.1"
    if(tokens.size() < 2) {
        std::cout << "ERROR: no se incluyó la dirección IP\nFormato: ping <dirección ip>" << std::endl;
        return;
    }
    
    std::cout << "Pinging " << tokens[1] << " (simulación)..." << std::endl;
    for(int i = 0; i < 4; i++){
        std::cout << "Reply from " << tokens[1] << ": time=1ms TTL=64" << std::endl;
        
        //Que aparezca un output cada 0.2 segundos
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }

}

void RouterCLI::handle_help(const CommandContexto&, const std::vector<std::string>&){
    std::cout << "\nComandos disponibles en modo actual:" << std::endl;
    std::cout << "  enable  - Entrar a modo privilegiado" << std::endl;
    std::cout << "  ping    - Enviar echo ICMP" << std::endl;
    std::cout << "  exit    - Salir del router" << std::endl;
}


// ------- HANDLERS PRIV EXEC --------
void RouterCLI::handle_disable(const CommandContexto&, const std::vector<std::string>&){
    modo_actual = CliMode::USER_EXEC;
}

void RouterCLI::handle_configure_terminal(const CommandContexto&, const std::vector<std::string>&){
    modo_actual = CliMode::GLOBAL_CONFIG;
}

void RouterCLI::handle_show_version(const CommandContexto& contexto, const std::vector<std::string>&){
    std::cout << contexto.core->hostname << " uptime is 0 days, 0 hours" << std::endl;
    std::cout << contexto.core->version << std::endl;
}

void RouterCLI::handle_show_running_config(const CommandContexto& contexto, const std::vector<std::string>&){
    if(contexto.core->running_config.texto.empty())
        contexto.core->generar_running_config();
    
    std::cout << "Building configuration..." << std::endl;
    std::cout << contexto.core->running_config.texto << std::endl;
}

void RouterCLI::handle_show_startup_config(const CommandContexto& contexto, const std::vector<std::string>&){
    if(contexto.core->startup_config.has_value()){
        std::cout << "ERROR: No se ha configurado la startup-config" << std::endl;
        return;
    }
    
    std::cout << "Showing startup-config..." << std::endl;
    std::cout << contexto.core->startup_config->texto << std::endl;
}

void RouterCLI::handle_show_ip_interface_brief(const CommandContexto& contexto, const std::vector<std::string>&){
    std::string status, protocolo;
    
    //Headers columnas
    std::cout << "Interface              IP-Address      OK? Method Status                Protocol" << std::endl;
    
    //Imprimir todas las filas
    for (const auto& interfaz : contexto.core->interfaces){
        //Determinar estatus de la interfaz
        status = interfaz.up ? "up" : "administratively down";
        
        //Determinar protocolo de la interfaz
        protocolo = interfaz.up ? "up" : "down";
        
        //Imprimir interfaz en columnas y filas fijas
        printf("%-22s %-15s YES manual %-21s %s\n",
            interfaz.nombre.c_str(), //Numero interfaz
            interfaz.ip.empty() ? "unassigned" : interfaz.ip.c_str(), //Dirección IP de la interfaz
            status.c_str(),
            protocolo.c_str()
        );
    }
}

void RouterCLI::handle_show_ip_ospf_neighbor(const CommandContexto& contexto, const std::vector<std::string>&){
    std::cout << "Neighbor ID     Pri   State            Dead Time   Address         Interface" << std::endl;
    
    //Imprimir cada vecino
    for(const auto& vecino : contexto.core->ospf_neighbors){
        printf("%-15s 1     %-16s 00:00:30    %-15s %s",
            vecino.router_id.c_str(),
            vecino.state.c_str(),
            vecino.neighbor_ip.c_str(),
            vecino.interfaz.c_str()
        );
    }
}

void RouterCLI::handle_show_ip_ospf_interface(const CommandContexto& contexto, const std::vector<std::string>&){
    std::cout << "Por implementar" << std::endl;
}

void RouterCLI::handle_show_ip_route(const CommandContexto& contexto, const std::vector<std::string>&){
    //Codigos de rutas
    std::cout << "Codes: C - connected, O - OSPF, S - static\n" << std::endl;
    
    for(const auto& ruta : contexto.core->rutas){
        std::cout << ruta.protocolo << "    "
                << ruta.destino << "/" << ruta.netmask
                << " via " << ruta.via << ", " << ruta.interfaz << std::endl;
    }
}

void RouterCLI::handle_copy_running_config_startup_config(const CommandContexto& contexto, const std::vector<std::string>&){
    if(contexto.core->running_config.texto.empty())
        contexto.core->generar_running_config();
    
    contexto.core->startup_config = ConfigSnapshot{
        contexto.core->running_config.texto
    };
    
    std::cout << "Building configuration...\n" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "[OK]" << std::endl;
}

void RouterCLI::handle_reload(const CommandContexto& contexto, const std::vector<std::string>&){
    std::cout << "Proceed with reload? [confirm] ";
    std::string linea;
    std::getline(std::cin, linea);

    std::cout << "\nReloading (simulación)..." << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if(contexto.core->startup_config.has_value())
        contexto.core->running_config = *contexto.core->startup_config;
    else {
        contexto.core->init_default_state();
        contexto.core->generar_running_config();
    }
    std::cout << "Reload completo." << std::endl;
}


// ------- HANDLERS GLOBAL CONFIG --------
void RouterCLI::handle_hostname(const CommandContexto& contexto, const std::vector<std::string>& tokens){
    if(tokens.size() < 2){
        std::cout << "ERROR: formato incorrecto.\nFormato: 'hostname <nombre>'";
        return;
    }
    
    contexto.core->hostname = tokens[1];
    std::cout << "Hostname configurado: " << contexto.core->hostname << std::endl;
}

void RouterCLI::handle_enable_secret(const CommandContexto& contexto, const std::vector<std::string>&){
    contexto.core->enable_secret = true;
}

void RouterCLI::handle_line_console_0(const CommandContexto&, const std::vector<std::string>&){
    modo_actual = CliMode::LINE_CONFIG;
}

void RouterCLI::handle_interface(const CommandContexto&, const std::vector<std::string>& tokens){
    if(tokens.size() < 2){
        std::cout << "ERROR: formato incorrecto.\nFormato: interface <nombre>" << std::endl;
        return;
    }
    
    modo_actual = CliMode::INTERFACE_CONFIG;
    interfaz = tokens[1];
}

void RouterCLI::handle_router_ospf(const CommandContexto&, const std::vector<std::string>&tokens){
    if(tokens.size() < 3){
        std::cout << "ERROR: formato incorrecto.\nFormato: router ospf <process-id>" << std::endl;
        return;
    }
    
    modo_actual = CliMode::ROUTER_OSPF_CONFIG;
    ospf_process_id = tokens[2];
}

void RouterCLI::handle_exit_global(const CommandContexto&, const std::vector<std::string>&){
    modo_actual = CliMode::PRIVILEGED_EXEC;
}

void RouterCLI::handle_end(const CommandContexto&, const std::vector<std::string>&){
    modo_actual = CliMode::PRIVILEGED_EXEC;
}


void RouterCLI::handle_exit_global_specific(const CommandContexto&, const std::vector<std::string>&){
    modo_actual = CliMode::GLOBAL_CONFIG;
}


// ------- HANDLERS LINE CONFIG --------
void RouterCLI::handle_password(const CommandContexto& contexto, const std::vector<std::string>& tokens){
    if(tokens.size() < 2){
        std::cout << "ERROR: formato incorrecto.\nFormato: password <PWD>" << std::endl;
        return;
    }
    
    contexto.core->process_password(tokens[1], contexto.core->enable_secret);
    contexto.core->actualizar_running_config();
    std::cout << "Contraseña configurada." << std::endl;
}

void RouterCLI::handle_login_local(const CommandContexto&contexto, const std::vector<std::string>&){
    contexto.core->login_local = true;
    contexto.core->actualizar_running_config();
}


// ------- HANDLERS CONFIG INTERFAZ --------
void RouterCLI::handle_ip_address(const CommandContexto& contexto, const std::vector<std::string>& tokens){
    if(tokens.size() < 4){
        std::cout << "ERROR: formato incorrecto.\nFormato: ip address A.B.C.D M.M.M.M" << std::endl;
        return;
    }
    
    InfoInterfaz* intf = contexto.core->get_interfaz(interfaz);
    intf->ip = tokens[2];
    intf->netmask = tokens[3];
    contexto.core->actualizar_running_config();
}

void RouterCLI::handle_no_shutdown(const CommandContexto& contexto, const std::vector<std::string>&){
    InfoInterfaz* intf = contexto.core->get_interfaz(interfaz);
    intf->up = true;
}

void RouterCLI::handle_description(const CommandContexto& contexto, const std::vector<std::string>& tokens){
    if(tokens.size() < 2){
        std::cout << "ERROR: formato incorrecto.\nFormato: description <DESCRIPCION>" << std::endl;
        return;
    }
    
    //Uniendo todos los tokens para añadirlos a la descripción
    std::string desc;
    for(std::size_t i = 1; i < tokens.size(); i++){
        if(i > 1){
            desc += ' ';
            desc += tokens[i];
        }
    }
    
    InfoInterfaz* intf = contexto.core->get_interfaz(interfaz);
    intf->description = desc;
}

void RouterCLI::handle_shutdown(const CommandContexto& contexto, const std::vector<std::string>&){
    InfoInterfaz* intf = contexto.core->get_interfaz(interfaz);
    intf->up = false;
}


// ------- HANDLERS CONFIG OSPF --------
void RouterCLI::handle_network(const CommandContexto&, const std::vector<std::string>& tokens){
    if(tokens.size() < 6){
        std::cout << "ERROR: formato incorrecto.\nFormato: network A.B.C.D W.W.W.W area N" << std::endl;
        return;
    }
    
    std::cout << "Red agregada a OSPF (simulado)\n";
}

void RouterCLI::handle_router_id(const CommandContexto&, const std::vector<std::string>& tokens){
    if (tokens.size() < 2) {
        std::cout << "ERROR: formato incorrecto.\nFormato: router-id A.B.C.D" << std::endl;
        return;
    }
    
    std::cout << "Router-id configurado: " << tokens[1] << " (simulado)\n";
}

void RouterCLI::handle_passive_interface(const CommandContexto&, const std::vector<std::string>&){
    std::cout << "Comando por implementar" << std::endl;
}

void RouterCLI::handle_no_passive_interface(const CommandContexto&, const std::vector<std::string>&){
    std::cout << "Comando por implementar" << std::endl;
}
