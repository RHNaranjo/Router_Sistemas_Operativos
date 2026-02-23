#include <string>
#include <vector>
#include <memory> //Manejar problemas de memoria
#include <functional> //Funciones lambda
#include "router_core.hpp"

#pragma once

//Clase con los modos de CLI del Router
enum class CliMode {
    USER_EXEC, 
    PRIVILEGED_EXEC,
    GLOBAL_CONFIG, 
    LINE_CONFIG,
    INTERFACE_CONFIG, //Interfaces
    ROUTER_OSPF_CONFIG //OSPF
};


struct CommandContexto {
    CliMode modo; //Qué comandos se pueden utilizar dependiendo del modo
    RouterCore* core;
};

using CommandHandler = std::function<void(
    const CommandContexto&, //Estado de CLI
    const std::vector<std::string>& //Comando
)>;

//Cada nodo del árbol de comandos
struct CommandNodo {
    std::string keyword; //'configure' o 'show'
    std::string help;
    
    bool es_hoja = false; //Sólo si es final del comando

    CommandHandler handler = nullptr;

    std::vector<std::unique_ptr<CommandNodo>> children; //Nodos hijos
};

//Clase del árbol donde se seleccionan los comandos
class ArbolComandos {
public:
    //Constructor trivial
    ArbolComandos() = default;

    //Construir los comandos ('show ip route' o 'show ip interface brief')
    void nuevo_comando(const std::vector<std::string>& keywords,
                        const std::string& help,
                        CommandHandler handler);

    //Ejecuta comando
    bool ejecutar_linea(CommandContexto& contexto, 
                        const std::string& linea,
                        std::string& error) const;

private:
    //El nodo raíz se mantiene privado porque sólo la clase tiene acceso a él
        //No es un comando. Todos los comandos son sus hijos
    std::unique_ptr<CommandNodo> raiz = std::make_unique<CommandNodo>();

    //Dividir el comando en palabras individualez
    static std::vector<std::string> tokenize(const std::string& linea);
    
    //Recorre el árbol para coincidir tokens con nodos (para permitir abreviaturas)
        //Esto lo quiero implementar para que 'conf t' se entienda como 'configure terminal', como en un router real
    const CommandNodo* detectar_comando(const std::vector<std::string>& tokens,
                                    std::vector<const CommandNodo*>& path,
                                    std::string& error) const;
};

class RouterCLI {
public:
    RouterCLI();
    
    explicit RouterCLI(RouterCore& core);
    
    //Bucle
    void run();

private:
    //El CLI comienza en user exec
    CliMode modo_actual = CliMode::USER_EXEC;
    
    RouterCore& core_; //Referencia al core (configuración) del router
    
    std::string interfaz, ospf_process_id;
    
    //Comandos de cada tipo de CLI
    ArbolComandos arbol_user_exec;
    ArbolComandos arbol_priv_exec;
    ArbolComandos arbol_global_cfg;
    ArbolComandos arbol_if_cfg;
    ArbolComandos arbol_ospf_cfg;
    
    //Funciones helpers
    CommandContexto crear_contexto() const;
    const ArbolComandos& obtener_arbol_de_modo(CliMode modo) const;
    std::string prompt() const;
    
    //Registrar comandos por modo
    void registrar_comandos_user_exec();
    void registrar_comandos_priv_exec();
    void registrar_comandos_global_cfg();
    void registrar_comandos_line_cfg();
    void registrar_comandos_if_cfg();
    void registrar_comandos_ospf_cfg();
    
    //Handlers USER EXEC
    void handle_enable(const CommandContexto&, const std::vector<std::string>&);
    void handle_exit(const CommandContexto&, const std::vector<std::string>&);
    void handle_ping(const CommandContexto&, const std::vector<std::string>&);
    void handle_help(const CommandContexto&, const std::vector<std::string>&);
    
    //Handlers Priv EXEC
    void handle_disable(const CommandContexto&, const std::vector<std::string>&);
    void handle_configure_terminal(const CommandContexto&, const std::vector<std::string>&);
    void handle_show_version(const CommandContexto&, const std::vector<std::string>&);
    void handle_show_running_config(const CommandContexto&, const std::vector<std::string>&);
    void handle_show_startup_config(const CommandContexto&, const std::vector<std::string>&);
    void handle_show_ip_interface_brief(const CommandContexto&, const std::vector<std::string>&);
    void handle_show_ip_ospf_neighbor(const CommandContexto&, const std::vector<std::string>&);
    void handle_show_ip_ospf_interface(const CommandContexto&, const std::vector<std::string>&);
    void handle_show_ip_route(const CommandContexto&, const std::vector<std::string>&);
    void handle_copy_running_config_startup_config(const CommandContexto&, const std::vector<std::string>&);
    void handle_write(const CommandContexto&, const std::vector<std::string>&);
    void handle_reload(const CommandContexto&, const std::vector<std::string>&);
    
    //Handlers global config
    void handle_hostname(const CommandContexto&, const std::vector<std::string>&);
    void handle_enable_secret(const CommandContexto&, const std::vector<std::string>&);
    void handle_line_console_0(const CommandContexto&, const std::vector<std::string>&);
    void handle_interface(const CommandContexto&, const std::vector<std::string>&);
    void handle_router_ospf(const CommandContexto&, const std::vector<std::string>&);
    void handle_exit_global(const CommandContexto&, const std::vector<std::string>&);
    void handle_end(const CommandContexto&, const std::vector<std::string>&);
    
    void handle_exit_global_specific(const CommandContexto&, const std::vector<std::string>&);
    
    //Handlers line config
    void handle_password(const CommandContexto&, const std::vector<std::string>&);
    void handle_login_local(const CommandContexto&, const std::vector<std::string>&);
    
    //Handlers Config interface
    void handle_ip_address(const CommandContexto&, const std::vector<std::string>&);
    void handle_no_shutdown(const CommandContexto&, const std::vector<std::string>&);
    void handle_description(const CommandContexto&, const std::vector<std::string>&);
    void handle_shutdown(const CommandContexto&, const std::vector<std::string>&);
    
    //Handlers OSPF
    void handle_network(const CommandContexto&, const std::vector<std::string>&);
    void handle_router_id(const CommandContexto&, const std::vector<std::string>&);
    void handle_passive_interface(const CommandContexto&, const std::vector<std::string>&);
    void handle_no_passive_interface(const CommandContexto&, const std::vector<std::string>&);
};
