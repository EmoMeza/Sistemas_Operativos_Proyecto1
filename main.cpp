#include <iostream>
#include <sstream>
#include <vector>

int main() {
    while (true) {
        // Imprimir el prompt
        std::cout << "ඞ shell ඞ > ";

        // Leer el comando ingresado por el usuario
        std::string line;
        if (!std::getline(std::cin, line)) {
            // Si no hay entrada, saltar la iteración
            continue;
        }
        // Crear un flujo de entrada a partir de la línea leída
        std::istringstream iss(line);

        // Leer el comando y sus argumentos
        std::vector<std::string> args;
        std::string arg;
        while (iss >> arg) {
            args.push_back(arg);
        }

        if(args.empty()){
        }
        else{
            // Verificar si el comando existe en la shell
            std::string command = args[0];
            if (command == "exit") {
                // Salir de la shell
                break;
            } else if (command == "cd") {
                // Cambiar de directorio
                // ...
            } else if (command == "ls") {
                // Listar archivos y directorios
                // ...
            } else {
                // Comando no encontrado
                std::cout << "Comando no encontrado: " << command << std::endl;
            }
        }
    }
    // END: abpxx6d04wxr

    return 0;
}

