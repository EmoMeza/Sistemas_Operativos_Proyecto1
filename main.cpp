#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <fstream>
#include <string>

// Implementar el demonio
void start_daemon(int t, int p) {
	pid_t pid = fork(); // Se crea un proceso hijo
	if (pid == 0) {
		umask(0); // Cambiar la máscara de modo de archivo, para que los archivos creados por el demonio tengan todos los permisos
		setsid(); // Nueva sesión
		close(STDIN_FILENO);
		close(STDOUT_FILENO);
		close(STDERR_FILENO);

		openlog("SusDaemon", LOG_PID, LOG_DAEMON); // Iniciar el log
		syslog(LOG_NOTICE, "El diablillo se ha creado"); // Mensaje de inicio

		for (int i = 0; i < p / t; ++i) {
            syslog(LOG_NOTICE, "Diablillo corriendo por la %d vez", i + 1);
			std::ifstream file("/proc/stat"); // Sacando la info desde /proc/stat porque /proc/cpuinfo no tiene la info de procesos (Al menos en WSL Ubuntu)
            // std::ifstream file("/proc/cpuinfo"); //! Descomentar esta línea para probar con /proc/cpuinfo
			std::string line;
			int processes = 0, procs_running = 0, procs_blocked = 0;

			if (file.is_open()) {
				while (std::getline(file, line)) {
					if (line.find("processes") != std::string::npos) {
						std::istringstream iss(line);
						std::string key;
						iss >> key >> processes;
					}
					if (line.find("procs_running") != std::string::npos) {
						std::istringstream iss(line);
						std::string key;
						iss >> key >> procs_running;
					}
					if (line.find("procs_blocked") != std::string::npos) {
						std::istringstream iss(line);
						std::string key;
						iss >> key >> procs_blocked;
					}
				}
				file.close();
			}

			syslog(LOG_NOTICE, "Procesos: %d, Corriendo: %d, Bloqueados: %d", processes, procs_running, procs_blocked);
			sleep(t);
		}
        syslog(LOG_NOTICE, "Diablillito ha terminado");
		closelog(); // Cerrar el log
        exit(0);
	}
}



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

        if (commands[0][0] == "start_daemon") {
            if (commands[0].size() != 3) {
                std::cerr << "Error: start_daemon requiere exactamente 2 argumentos (t y p).\n";
            } else {
                try { // Convertir los argumentos a int
                    int t = std::stoi(commands[0][1]); 
                    int p = std::stoi(commands[0][2]); 

                    if (t <= 0 || p <= 0) {
                        std::cerr << "Error: t y p deben ser números enteros positivos.\n";
                    }  else {
                        if (t > p) {
                            std::cerr << "Error: t debe ser menor o igual a p.\n";
                            continue;
                        }
                        start_daemon(t, p); // Llamar a la función para iniciar el demonio
                        continue;
                    }
                } catch (const std::invalid_argument& e) {
                    std::cerr << "Error: Los argumentos t y p deben ser números enteros.\n";
                } catch (const std::exception& e) {
                    std::cerr << "Error desconocido: " << e.what() << '\n';
                }
            }
            continue;
        }

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