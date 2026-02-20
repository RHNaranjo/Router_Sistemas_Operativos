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
            mensaje_error = "Comando no reconocido: " + t;
            return nullptr;
        } else if(detectados.size() > 1) {
            mensaje_error = "Comando ambiguo: " + t;
            return nullptr;
        }

        actual = detectados[0];
        path.push_back(actual);
    }
    if(!actual->es_hoja){
        mensaje_error = "El comando está incompleto";
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

    mensaje_error = "Comando sin implementación";
    return false;
}

//CLI del router
RouterCLI::RouterCLI() {
    registrar_comandos_user_exec();
    registrar_comandos_priv_exec();
    registrar_comandos_global_cfg();
    registrar_comandos_if_cfg();
    registrar_comandos_ospf_cfg();
}

CommandContexto RouterCLI::crear_contexto() const {
    CommandContexto contexto{modo_actual};
    return contexto;
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
            return "Router>";
        case CliMode::PRIVILEGED_EXEC:
            return "Router#";
        case CliMode::GLOBAL_CONFIG:
            return "Router(config)#";
        case CliMode::INTERFACE_CONFIG:
            return "Router(config-if)#";
        case CliMode::ROUTER_OSPF_CONFIG:
            return "Router(config-router)#";
    }

    //En caso de que no se obtenga el contexto, se empieza en lo más básico
    return "Router>";
}

void RouterCLI::run(){
    std::string linea;
    while(true) {
        std::cout << prompt() << " ";
        if(!std::getline(std::cin, linea))
            break;
        
        CommandContexto contexto = crear_contexto();
        std::string error;

        const auto& arbol = obtener_arbol_de_modo(modo_actual);
        bool ok = arbol.ejecutar_linea(contexto, linea, error);
        if(!ok && !error.empty())
            std::cout << "% " << error << "\n";
    }
}

//Registro de comandos