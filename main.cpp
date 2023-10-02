#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>

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

        // Verificar si el comando es "exit"
        if (line == "exit") {
            break;
        }

        // Crear un vector de vectores de strings para almacenar los comandos separados por pipes
        std::vector<std::vector<std::string>> commands;

        // Crear un flujo de entrada a partir de la línea leída
        std::istringstream iss(line);

        // Leer el primer comando y sus argumentos
        std::vector<std::string> args;
        std::string arg;
        while (iss >> arg) {
            if (arg == "|") {
                // Agregar el comando actual al vector de comandos
                commands.push_back(args);
                args.clear();
            } else {
                args.push_back(arg);
            }
        }
        // Agregar el último comando al vector de comandos
        commands.push_back(args);

        // Crear un vector de pipes
        std::vector<int[2]> pipes(commands.size() - 1);

        // Crear un vector de procesos hijos
        std::vector<pid_t> pids(commands.size());

        // Crear un bucle para ejecutar los comandos en cada proceso hijo
        for (int i = 0; i < commands.size(); i++) {
            // Crear un nuevo proceso hijo
            pids[i] = fork();

            if (pids[i] == -1) {
                // Error al crear el proceso hijo
                std::cerr << "Error: no se pudo crear el proceso hijo\n";
                exit(1);
            }
            else if (pids[i] == 0) {
                // Este es el proceso hijo
                if (i > 0) {
                    // Redirigir la entrada estándar al descriptor de lectura del pipe anterior
                    dup2(pipes[i - 1][0], STDIN_FILENO);
                    // Cerrar el descriptor de escritura del pipe anterior
                    close(pipes[i - 1][1]);
                }
                if (i < commands.size() - 1) {
                    // Redirigir la salida estándar al descriptor de escritura del pipe actual
                    dup2(pipes[i][1], STDOUT_FILENO);
                    // Cerrar el descriptor de lectura del pipe actual
                    close(pipes[i][0]);
                }

                // Ejecutar el comando correspondiente
                char** argv = new char*[commands[i].size() + 1];
                for (int j = 0; j < commands[i].size(); j++) {
                    argv[j] = const_cast<char*>(commands[i][j].c_str());
                }
                argv[commands[i].size()] = nullptr;
                execvp(argv[0], argv);
                // Si llegamos aquí, hubo un error al ejecutar el comando
                std::cerr << "Error: no se pudo ejecutar el comando\n";
                perror("execvp");
                delete[] argv; // Evitar leaks de memoria
                exit(1);
            }

            if (i > 0) {
                // Cerrar el descriptor de lectura del pipe anterior en el proceso padre
                close(pipes[i - 1][0]);
            }
            if (i < commands.size() - 1) {
                // Crear un nuevo pipe en el proceso padre
                if (pipe(pipes[i]) == -1) {
                    std::cerr << "Error: no se pudo crear el pipe\n";
                    perror("pipe");
                    continue;
                }
                // Cerrar el descriptor de escritura del pipe actual en el proceso padre
                close(pipes[i][1]);
            }
        }

        // Esperar a que todos los procesos hijos terminen de ejecutar sus comandos
        for (int i = 0; i < commands.size(); i++) {
            int status;
            waitpid(pids[i], &status, 0);
        }
    }
    return 0;
}