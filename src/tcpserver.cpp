#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <chrono>
#include <random>
#include <thread>
#include <map>
#include <unordered_map>
#include <array>
#include <list>
#include <algorithm>

#include "userData.h"
#include "logger.h"
#include "gameSession.h"

std::mt19937_64 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());

namespace TicTacToeServer {

    Logger logger("server.log");

    class serverSocket {
    private:
        std::unordered_map<std::string, std::string> configData; // container with data from config
        std::map<std::string, userData> db; // DataBase
        std::map<int, std::string> clientLogin; // socketID -> login

        std::vector<gameSession> gameSessions;
        std::vector<bool> isSessionUsed;
        std::list<int> waitingQueue;

        static const int BUFFERSIZE = 1024;

        // Socket vars
        int opt = 1;
        int master_socket{};
        int addrLen;
        int max_clients;
        std::vector<int> client_sockets;
        int new_socket;
        int activity;
        int max_sd;
        ssize_t valread{};
        sockaddr_in address{};
        timeval tv{0, 500000};

        fd_set readfds{};
        fd_set writefds{};

        char buffer[BUFFERSIZE] = {0};

        bool isActive = true; // Socket state

        void createMasterSocket() {
            master_socket = socket(
                    AF_INET,
                    SOCK_STREAM,
                    0);
            if (master_socket == 0) {
                throw std::invalid_argument("Can't create master socket");
            }
            logger.log(Logger::INFO, "Create master socket: OK.");
        }

        void createDescriptor() {
            if (setsockopt(master_socket,
                           SOL_SOCKET,
                           SO_REUSEADDR | SO_REUSEPORT,
                           &opt,
                           sizeof(opt)
            ) < 0) {
                throw std::invalid_argument("Can't create socket descriptor");
            }
            logger.log(Logger::INFO, "Create descriptor: OK.");
        }

        void bindSocket() {
            if (bind(master_socket,
                     (struct sockaddr *) &address,
                     sizeof(address)
            ) < 0) {
                throw std::invalid_argument("Can't bind socket");
            }
            logger.log(Logger::INFO, "Binding: OK.");
        }

        void listenSocket() const {
            if (listen(master_socket, max_clients + 1) < 0) {
                throw std::invalid_argument("Listen error");
            }
            logger.log(Logger::INFO, "Listening: OK.");
        }

        void loadDB() {
            logger.log(Logger::INFO, "Start loading the database.");

            std::ifstream DBFile(".db");

            if (!DBFile.is_open()) {
                throw std::invalid_argument("Can't read database");
            }

            std::string currentLine;
            while (getline(DBFile, currentLine)) {
                std::string login = currentLine.substr(0, currentLine.find(':'));
                std::string password = currentLine.substr(login.size() + 1);
                db.insert(std::make_pair(login, userData(password, false, false)));
            }

            DBFile.close();

            logger.log(Logger::INFO, "End loading the database.");
            std::cout << "Database loaded" << std::endl;
        }

        void saveDB() {
            logger.log(Logger::INFO, "Start saving the database.");

            std::fstream DBFile(".db", std::ios::out | std::ios::trunc);

            if (!DBFile.is_open()) {
                throw std::invalid_argument("Can't save database");
            }

            for (auto const &[login, data]: db) {
                DBFile << login << ':' << data.password << std::endl;
            }

            DBFile.close();

            logger.log(Logger::INFO, "End saving the database.");
        }

        void readCfg() {
            logger.log(Logger::INFO, "Start reading the cfg.");

            std::ifstream cfgFile("server.config");

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

            logger.log(Logger::INFO, "End reading the cfg.");
        }

        void setupCfg() {
            logger.log(Logger::INFO, "Start setup cfg.");
            gameSessions.resize(std::stoul(configData["GAMESESSIONS"]));
            isSessionUsed.resize(std::stoul(configData["GAMESESSIONS"]));

            max_clients = std::stoi(configData["MAXCLIENTS"]);
            client_sockets.resize(max_clients);

            std::cout << "Config loaded" << std::endl;
            logger.log(Logger::INFO, "End setup cfg.");
        }

        void inputThread() {
            std::string command;
            while (true) {
                std::cin >> command;
                logger.log(Logger::INFO, "Entered a command: " + command);
                if (command == "exit") {
                    this->isActive = false;
                    break;
                } else if (command == "queue") {
                    std::cout << waitingQueue.size() << std::endl;
                    for (const auto &el: waitingQueue) {
                        std::cout << el << ' ';
                    }
                    std::cout << std::endl;
                } else if (command == "db") {
                    for (const auto &[login, data]: db) {
                        const auto &[password, isLogged, isPlaying, activeSession] = data;
                        std::cout << login << ' ' << password << ' ' << isLogged << ' ' << isPlaying << ' '
                                  << activeSession
                                  << ' ' << isSessionUsed[activeSession] << std::endl;
                    }
                } else {
                    std::cout << "Unknown command." << std::endl;
                }
            }
        }

        void runInputThread() {
            std::thread th(&serverSocket::inputThread, this);
            th.detach();
        }

        void createSession() {
            for (int i = 0; i < std::stoul(configData["GAMESESSIONS"]); ++i) {
                if (!isSessionUsed[i]) { // looking for free gameSession
                    isSessionUsed[i] = true;
                    logger.log(Logger::DEBUG, "Session " + std::to_string(i) + " in use.");
                    int firstClient = waitingQueue.front();
                    waitingQueue.pop_front();
                    int secondClient = waitingQueue.front();
                    waitingQueue.pop_front();

                    // add to the database as players
                    db[clientLogin[firstClient]].isPlaying = true;
                    db[clientLogin[firstClient]].activeSession = i;
                    db[clientLogin[secondClient]].isPlaying = true;
                    db[clientLogin[secondClient]].activeSession = i;

                    gameSessions[i].restart();
                    std::this_thread::sleep_for(
                            std::chrono::milliseconds(500)); // to prevent double message send

                    for (const auto client: {firstClient, secondClient}) {
                        logger.log(Logger::DEBUG, "Pop " + std::to_string(client) + " from queue");
                        gameSessions[i].addUser(client);
                        sendMessage(client, "restart");
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // prevent double message
                    sendMessage(rng() % 2 == 0 ? firstClient : secondClient, "lock");

                    break;
                }
            }
        }

    public:
        serverSocket() try:
                addrLen(sizeof(address)) {

            logger.log(Logger::INFO, "starting loading cfg.");

            readCfg();

            setupCfg();

            loadDB();

            address = {AF_INET, htons(std::stoul(configData["PORT"])), {inet_addr(configData["HOST"].c_str())}};

            runInputThread(); // async thread for console commands

            createMasterSocket();

            createDescriptor();

            bindSocket();

            listenSocket();

            std::cout << "Waiting for connections..." << std::endl;

            while (this->isActive) { // Socket Loop
                FD_ZERO(&readfds);

                FD_SET(master_socket, &readfds);
                max_sd = master_socket;

                for (int i = 0; i < max_clients; ++i) { // idk
                    int sd = client_sockets[i];
                    if (sd > 0) {
                        FD_SET(sd, &readfds);
                    }
                    if (sd > max_sd) {
                        max_sd = sd;
                    }
                }

                // select with unlock after 500ms ( look &tv )
                activity = select(max_sd + 1, &readfds, &writefds, nullptr, &tv);
                if (activity < 0 && errno != EINTR) {
                    throw std::invalid_argument("Select error");
                }

                if (FD_ISSET(master_socket, &readfds)) { // new connections
                    if ((new_socket = accept(
                            master_socket,
                            (struct sockaddr *) &address,
                            (socklen_t *) &addrLen)) < 0) {
                        throw std::invalid_argument("Accept error");
                    }

                    std::cout << "New connection, socket fd: " << new_socket <<
                              " ip: " << inet_ntoa(address.sin_addr) <<
                              " port: " << ntohs(address.sin_port) << std::endl;
                    logger.log(Logger::DEBUG, "User " + std::to_string(new_socket) + " is connected. IP: " +
                                              inet_ntoa(address.sin_addr) + ", Port: " +
                                              std::to_string(ntohs(address.sin_port)));

                    for (int i = 0; i < max_clients; ++i) {
                        if (client_sockets[i] == 0) {
                            client_sockets[i] = new_socket;
                            std::cout << "Adding to list of sockets as " << i << std::endl;
                            logger.log(Logger::DEBUG, "Adding to list as " + std::to_string(i));
                            break;
                        }
                    }
                }

                if (waitingQueue.size() >= 2) { // Start gameSession when 2 clients are waiting for the game
                    createSession();
                }

                for (int i = 0; i < max_clients; ++i) { // Main request handler
                    int sd = client_sockets[i];
                    if (FD_ISSET(sd, &readfds)) {
                        memset(buffer, 0, BUFFERSIZE);
                        if ((valread = read(sd, buffer, BUFFERSIZE)) == 0) { // if client disconnect
                            getpeername(sd,
                                        (struct sockaddr *) &address,
                                        (socklen_t *) &addrLen);
                            std::cout << "Host disconnected, ip: " << inet_ntoa(address.sin_addr) << " port: "
                                      << ntohs(address.sin_port) << std::endl;
                            logger.log(Logger::DEBUG, "User " + std::to_string(i) + " is disconnected. IP: " +
                                                      inet_ntoa(address.sin_addr) + ", Port: " +
                                                      std::to_string(ntohs(address.sin_port)));
                            close(sd);
                            client_sockets[i] = 0;
                            if (clientLogin.contains(i)) { // free in [idx -> login] map
                                auto &[password, isLogged, isPlaying, activeSession] = db[clientLogin[i]];

                                for (int j = 0;
                                     j < max_clients; ++j) { // send other clients a message about disconnection
                                    if (clientLogin.contains(j) && db[clientLogin[j]].isPlaying &&
                                        db[clientLogin[j]].activeSession == activeSession) {
                                        sendMessage(j, "disconnect");
                                        db[clientLogin[j]].isPlaying = false;
                                    }
                                }

                                isLogged = false;
                                clientLogin.erase(clientLogin.find(i));
                                isSessionUsed[activeSession] = false;
                            }
                            if (auto it = std::find_if(waitingQueue.begin(), waitingQueue.end(),
                                                       [i](int current) { // delete from waiting queue
                                                           return current == i;
                                                       }); it != std::end(waitingQueue)) {
                                waitingQueue.erase(it);
                                logger.log(Logger::DEBUG, "Pop " + std::to_string(*it) + " from queue");
                            }
                        } else { // if got a message
                            std::cout << "msg from client: " << buffer << std::endl;
                            logger.log(Logger::DEBUG,
                                       "Got message: " + std::string(buffer) + " from " + std::to_string(i));
                            std::string command = strtok(buffer, " ");

                            if (command == "log") { // login
                                std::string login = strtok(nullptr, " ");
                                std::string password = strtok(nullptr, " ");

                                if (!db.contains(login)) { // no login in db
                                    sendMessage(i, "404");
                                    continue;
                                }
                                if (db[login].password != password) { // wrong password
                                    sendMessage(i, "401");
                                    continue;
                                }
                                if (db[login].isLogged) { // already logged
                                    sendMessage(i, "405");
                                    continue;
                                }
                                sendMessage(i, "200"); // good login
                                clientLogin.insert(std::make_pair(i, login));
                                db[login].isLogged = true;
                                waitingQueue.push_back(i);
                                logger.log(Logger::INFO, "Pushing " + std::to_string(i) + " to queue");
                            } else if (command == "reg") { // registration
                                std::string login = strtok(nullptr, " ");
                                std::string password = strtok(nullptr, " ");

                                if (db.contains(login)) { // already registered
                                    sendMessage(i, "400");
                                    continue;
                                }

                                db.insert(std::make_pair(login, userData(password, false, false)));
                                sendMessage(i, "200"); // good registration
                            } else if (command == "put") { // inGame requests
                                size_t activeSession = db[clientLogin[i]].activeSession;

                                std::vector<int> usersInSession = gameSessions[activeSession].getUsers();

                                std::string id = strtok(nullptr, " ");

                                for (auto user: usersInSession) {
                                    sendMessage(user, (gameSessions[activeSession].getTurn() ? "X" : "O") +
                                                      id); // send a move to all users in the session
                                }

                                gameSessions[activeSession].setCell(stoull(id)); // setCell in local session
                                if (bool isWon = gameSessions[activeSession].isWon(), isDraw = gameSessions[activeSession].isDraw();
                                        isWon || isDraw) { // if somebody win or draw
                                    std::this_thread::sleep_for(
                                            std::chrono::milliseconds(500)); // prevent double message
                                    for (auto user: usersInSession) {
                                        sendMessage(user, isWon ? "win" : "draw");
                                        db[clientLogin[user]].isPlaying = false;
                                    }
                                    isSessionUsed[activeSession] = false;
                                    logger.log(Logger::DEBUG, "Session " + std::to_string(activeSession) + " is free.");
                                }
                            } else if (command == "again") {
                                waitingQueue.push_back(i);
                            }
                        }
                    }
                }
            }
        } catch (const std::exception &e) {
            std::cerr << e.what();
            logger.log(Logger::ERROR, e.what());
        }

        ~serverSocket() {
            saveDB();

            for (int i = 0; i < max_clients; ++i) {
                send(client_sockets[i], "shutdown", strlen("shutdown"), 0);
                close(client_sockets[i]);
            }
            logger.log(Logger::INFO, "All clients disconnected.");
            // closing the listening socket
            shutdown(master_socket, SHUT_RDWR);
            close(master_socket);
            logger.log(Logger::INFO, "Shutdown.");
        }

        void sendMessage(const int idx, const std::string &message) {
            send(client_sockets[idx], message.c_str(), strlen(message.c_str()), 0);
            logger.log(Logger::DEBUG, "Send " + message + " to " + std::to_string(idx));
        }
    };
}

int main() {
    TicTacToeServer::serverSocket server;
    return 0;
}
