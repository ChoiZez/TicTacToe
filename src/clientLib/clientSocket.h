#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <iostream>
#include <unordered_map>
#include <fstream>
#include <algorithm>
#include <sstream>

#include "logger.h"

namespace TicTacToe {
    void getGameMessage();

    Logger logger("client.log");

    class ClientSocket {
    private:
        std::unordered_map<std::string, std::string> configData; // container for config data
        static const int BUFFERSIZE = 1024;

        // socket vars
        int client_fd;
        struct sockaddr_in servAddr{};
        char buffer[BUFFERSIZE]{0};

        bool isActive = false; // socket state

        void readCfg() {
            logger.log(Logger::INFO, "Start loading the config.");
            std::ifstream cfgFile("client.config");

            if (!cfgFile.is_open()) {
                throw std::invalid_argument("Can't open cfg.");
            }

            for (std::string currentLine{}; std::getline(cfgFile, currentLine);) {
                if (currentLine.find('=') != std::string::npos) {
                    std::istringstream iss{currentLine};
                    if (std::string id{}, value{}; std::getline(std::getline(iss, id, '='), value)) {
                        std::transform(id.begin(), id.end(), id.begin(), [](unsigned char c) { return toupper(c); });
                        configData[id] = value;
                    }
                }
            }

            cfgFile.close();

            std::cout << "Config loaded" << std::endl;
            logger.log(Logger::INFO, "Config loaded.");
        }

    public:
        ClientSocket() try {
            readCfg();

            client_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (client_fd < 0) {
                throw std::invalid_argument("Socket creation error");
            }
            memset(&servAddr, 0, sizeof(servAddr));
            servAddr.sin_family = AF_INET;
            servAddr.sin_port = htons(std::stoul(configData["PORT"]));
            servAddr.sin_addr.s_addr = INADDR_ANY;

            int status = connect(
                    client_fd,
                    (struct sockaddr *) &servAddr,
                    sizeof(servAddr)
            );
            if (status < 0) {
                throw std::invalid_argument("Connection Failed");
            }
            isActive = true;
            std::cout << "Connected\n";
            logger.log(Logger::INFO, "Connected.");
        } catch (const std::exception &e) {
            std::cerr << e.what();
            logger.log(Logger::ERROR, e.what());
        }

        void sendMessage(const std::string &message) const {
            send(client_fd, message.c_str(), strlen(message.c_str()), 0);
            std::cout << "Send " << message << '\n';
            logger.log(Logger::DEBUG, "Send " + message);
        }

        std::string getMessage() {
            memset(buffer, 0, BUFFERSIZE);
            read(client_fd, buffer, BUFFERSIZE);
            std::cout << "Get " << buffer << std::endl;
            logger.log(Logger::DEBUG, "Get " + std::string(buffer));
            return buffer;
        }

        ~ClientSocket() {
            logger.log(Logger::INFO, "Socket shutdown.");
            close(client_fd);
            isActive = false;
        }

        friend void TicTacToe::getGameMessage();
    };
}
