#include "../include/router_cli.hpp"
#include "../include/router_core.hpp"
#include <iostream>

int main(){
    //Crear el core del router
    RouterCore core;
    core.init_default_state();
    core.generar_running_config();
    
    //Crear la CLI asociada al core
    RouterCLI cli(core);
    
    //Ejecutar
    cli.run();
    
    return 0;
}