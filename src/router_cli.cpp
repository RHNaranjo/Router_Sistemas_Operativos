#include "../include/router_cli.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

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

    //Recorrer los tokens para buscar qué comando es
    for(const auto& t : tokens){
        std::vector<const CommandNodo*> detectados;

        for(const auto& hijo : actual->children) {
            if(hijo->keyword.rfind(t, 0) == 0) //Detecta si el comando comienza con esa secuencia
                detectados.push_back(hijo.get());
            
            
        }

        // Regresar los errores en caso de que (1) no se haya detectado nada o (2) se hayan detectado más de un posible comando
        if(detectados.empty()) {
            mensaje_error = "ERROR: Comando no reconocido: " + t;
            return nullptr;
        } else if(detectados.size() > 1) {
            mensaje_error = "ERROR: Comando ambiguo: " + t;
            return nullptr;
        }

        actual = detectados[0];
        path.push_back(actual);
    }
    if(!actual->es_hoja){
        mensaje_error = "ERROR: El comando está incompleto";
        return nullptr;
    }
    return actual;
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
    std::cout << "\n=== " << core_.hostname << " Router Sistemas Operativos ===\n";
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
            std::cout << "ERROR: " << error << "\n";
    }
}

// ------- REGISTRO DE COMANDOS --------
void RouterCLI::registrar_comandos_user_exec() {
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

void RouterCLI::registrar_comandos_priv_exec() {
    //Disable
    arbol_user_exec.nuevo_comando(
        {"disable"},
        "Volver a modo user exec.",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_disable(contexto, tokens);
        }
    );
    
    //Configure Terminal
    arbol_user_exec.nuevo_comando(
        {"configure", "terminal"},
        "Entrar a modo de configuración global",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_configure_terminal(contexto, tokens);
        }
    );
    
    //Show version
    arbol_user_exec.nuevo_comando(
        {"show", "version"},
        "Mostrar la versión del router",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_version(contexto, tokens);
        }
    );
    
    //Show running config
    arbol_user_exec.nuevo_comando(
        {"show", "running-config"},
        "Mostrar la configuración en ejecución",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_running_config(contexto, tokens);
        }
    );
    
    //Show startup config
    arbol_user_exec.nuevo_comando(
        {"show", "startup-config"},
        "Mostrar configuración de inicio",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_startup_config(contexto, tokens);
        }
    );
    
    //Show ip interface brief
    arbol_user_exec.nuevo_comando(
        {"show", "ip", "interface", "brief"},
        "Mostrar resumen de las interfaces IP",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_interface_brief(contexto, tokens);
        }
    );
    
    //Show ip ospf neighbor
    arbol_user_exec.nuevo_comando(
        {"show", "ip", "ospf", "neighbor"},
        "Mostrar los vecinos OSPF",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_ospf_neighbor(contexto, tokens);
        }
    );
    
    //Show ip ospf interface
    arbol_user_exec.nuevo_comando(
        {"show", "ip", "ospf", "interface"},
        "Mostrar las interfaces de OSPF",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_ospf_interface(contexto, tokens);
        }
    );
    
    //Show ip route
    arbol_user_exec.nuevo_comando(
        {"show", "ip", "route"},
        "Mostrar la tabla de enrutamiento",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_show_ip_route(contexto, tokens);
        }
    );
    
    //Copy running config startup config
    arbol_user_exec.nuevo_comando(
        {"copy", "running-config", "startup-config"},
        "Guardar la configuración actual",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_copy_running_config_startup_config(contexto, tokens);
        }
    );
    
    //Write
    arbol_user_exec.nuevo_comando(
        {"write"},
        "Guardar la configuración actual",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_write(contexto, tokens);
        }
    );
    
    //Reload
    arbol_user_exec.nuevo_comando(
        {"reload"},
        "Reiniciar el router",
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_reload(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_global_cfg() {
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
            handle_exit(contexto, tokens);
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

void RouterCLI::registrar_comandos_line_cfg() {
    //Password
    arbol_global_cfg.nuevo_comando(
        {"password"},
        "Añadir una contraseña al router", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_password(contexto, tokens);
        }
    );
    
    //Login local
    arbol_global_cfg.nuevo_comando(
        {"login", "local"},
        "Forzar a la autenticación de usuarios", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_login_local(contexto, tokens);
        }
    );
    
    //Exit
    arbol_global_cfg.nuevo_comando(
        {"exit"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_line(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_if_cfg() {
    //Ip address
    arbol_global_cfg.nuevo_comando(
        {"ip", "address"},
        "Configurar dirección IP del router", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_ip_address(contexto, tokens);
        }
    );
    
    //No shutdown
    arbol_global_cfg.nuevo_comando(
        {"no", "shutdown"},
        "Activar la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_no_shutdown(contexto, tokens);
        }
    );
    
    //Description
    arbol_global_cfg.nuevo_comando(
        {"description"},
        "Descripción de la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_description(contexto, tokens);
        }
    );
    
    //Shutdown
    arbol_global_cfg.nuevo_comando(
        {"shutdown"},
        "Desactivar la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_shutdown(contexto, tokens);
        }
    );
    
    //Exit
    arbol_global_cfg.nuevo_comando(
        {"exit"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_if(contexto, tokens);
        }
    );
}

void RouterCLI::registrar_comandos_ospf_cfg() {
    //Network
    arbol_global_cfg.nuevo_comando(
        {"network"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_if(contexto, tokens);
        }
    );
    
    //Router-id
    arbol_global_cfg.nuevo_comando(
        {"router-id"},
        "Registrar el router-id OSPF", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_router_id(contexto, tokens);
        }
    );
    
    //Passive-interface
    arbol_global_cfg.nuevo_comando(
        {"passive-interface"},
        "Detener los paquetes 'hello'", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_passive_interface(contexto, tokens);
        }
    );
    
    //No passive-interface
    arbol_global_cfg.nuevo_comando(
        {"no", "passive-interface"},
        "Activa los paquetes 'hello' en la interfaz", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_no_passive_interface(contexto, tokens);
        }
    );
    
    //Exit
    arbol_global_cfg.nuevo_comando(
        {"exit"},
        "Regresar a modo configuración global", 
        [this](const CommandContexto& contexto, const std::vector<std::string>& tokens){
            handle_exit_ospf(contexto, tokens);
        }
    );
}

