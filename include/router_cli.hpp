#include <string>
#include <vector>
#include <memory> //Manejar problemas de memoria
#include <functional> //Funciones lambda

#pragma once

//Clase con los modos de CLI del Router
enum class CliMode {
    USER_EXEC, 
    PRIVILEGED_EXEC,
    GLOBAL_CONFIG, 
    INTERFACE_CONFIG, //Interfaces
    ROUTER_OSPF_CONFIG //OSPF
};


struct CommandContexto {
    CliMode modo; //Qué comandos se pueden utilizar dependiendo del modo
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
    const CommandNodo* match_command(const std::vector<std::string>& tokens,
                                    std::vector<const CommandNodo*>& path,
                                    std::string& error) const;
};
