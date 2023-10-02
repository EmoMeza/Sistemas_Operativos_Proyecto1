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
            // Buscar si el comando contiene un pipe
            int pipe_pos = -1;
            for (int i = 0; i < args.size(); i++) {
                if (args[i] == "|") {
                    pipe_pos = i;
                    break;
                }
            }

            if (pipe_pos == -1) {
                // No hay pipe, ejecutar el comando normalmente
                // Crear un nuevo proceso hijo
                pid_t pid = fork();

                if (pid == -1) {
                    // Error al crear el proceso hijo
                    std::cerr << "Error: no se pudo crear el proceso hijo\n";
                }
                else if (pid == 0) {
                    // Este es el proceso hijo
                    // Ejecutar el comando ingresado
                    char** argv = new char*[args.size() + 1];
                    for (int i = 0; i < args.size(); i++) {
                        argv[i] = const_cast<char*>(args[i].c_str());
                    }
                    argv[args.size()] = nullptr;
                    execvp(argv[0], argv);
                    // Si llegamos aquí, hubo un error al ejecutar el comando
                    std::cerr << "Error: no se pudo ejecutar el comando\n";
                    exit(1);
                }
                else {
                    // Este es el proceso padre
                    // Esperar a que el proceso hijo termine de ejecutar el comando
                    int status;
                    waitpid(pid, &status, 0);
                }
            }
            else {
                // Hay un pipe, dividir el comando en dos comandos separados
                std::vector<std::string> args1(args.begin(), args.begin() + pipe_pos);
                std::vector<std::string> args2(args.begin() + pipe_pos + 1, args.end());

                // Crear un pipe
                int fd[2];
                if (pipe(fd) == -1) {
                    std::cerr << "Error: no se pudo crear el pipe\n";
                    continue;
                }

                // Crear el primer proceso hijo
                pid_t pid1 = fork();

                if (pid1 == -1) {
                    // Error al crear el proceso hijo
                    std::cerr << "Error: no se pudo crear el proceso hijo\n";
                }
                else if (pid1 == 0) {
                    // Este es el primer proceso hijo
                    // Redirigir la salida estándar al descriptor de escritura del pipe
                    close(fd[0]);
                    dup2(fd[1], STDOUT_FILENO);
                    close(fd[1]);

                    // Ejecutar el primer comando
                    char** argv = new char*[args1.size() + 1];
                    for (int i = 0; i < args1.size(); i++) {
                        argv[i] = const_cast<char*>(args1[i].c_str());
                    }
                    argv[args1.size()] = nullptr;
                    execvp(argv[0], argv);
                    // Si llegamos aquí, hubo un error al ejecutar el comando
                    std::cerr << "Error: no se pudo ejecutar el comando\n";
                    exit(1);
                }

                // Crear el segundo proceso hijo
                pid_t pid2 = fork();

                if (pid2 == -1) {
                    // Error al crear el proceso hijo
                    std::cerr << "Error: no se pudo crear el proceso hijo\n";
                }
                else if (pid2 == 0) {
                    // Este es el segundo proceso hijo
                    // Redirigir la entrada estándar al descriptor de lectura del pipe
                    close(fd[1]);
                    dup2(fd[0], STDIN_FILENO);
                    close(fd[0]);

                    // Ejecutar el segundo comando
                    char** argv = new char*[args2.size() + 1];
                    for (int i = 0; i < args2.size(); i++) {
                        argv[i] = const_cast<char*>(args2[i].c_str());
                    }
                    argv[args2.size()] = nullptr;
                    execvp(argv[0], argv);
                    // Si llegamos aquí, hubo un error al ejecutar el comando
                    std::cerr << "Error: no se pudo ejecutar el comando\n";
                    exit(1);
                }

                // Este es el proceso padre
                // Cerrar los descriptores de archivo del pipe
                close(fd[0]);
                close(fd[1]);

                // Esperar a que los procesos hijos terminen de ejecutar los comandos
                int status;
                waitpid(pid1, &status, 0);
                waitpid(pid2, &status, 0);
            }
        }
    }
    return 0;
}