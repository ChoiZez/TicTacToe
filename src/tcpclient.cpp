#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Secret_Input.H>
#include <FL/Fl_Input.H>
#include <FL/fl_message.H>

#include "clientSocket.h"

#include <vector>
#include <memory>
#include <string>
#include <functional>
#include <thread>
#include <chrono>
#include <regex>

namespace TicTacToe {
    using cb = std::function<void()>;
    ClientSocket socket;

    class GameWindow : public Fl_Window { // Main widow with game
    private:
        const int BUTTON_SIZE = 100;
        int _cells;
        std::vector<std::unique_ptr<Fl_Button>> buttons; // buttons :)
        Fl_Box box{0, 0, 300, 50, "Your Turn"}; // box with info whose turn to move
        bool locked = false;
    public:
        explicit GameWindow(int cells = 3) try: Fl_Window(0, 0, "TicTacToe"), _cells(cells) {
            GenerateBoard();
            this->resize(x(), y(), _cells * BUTTON_SIZE, _cells * BUTTON_SIZE + 150);
            box.position(_cells * BUTTON_SIZE / 2 - 150, 50);
        } catch (std::exception &e) {
            std::cerr << e.what();
            logger.log(Logger::ERROR, e.what());
        }

        ~GameWindow() override {
            for (const auto &button: buttons) {
                delete static_cast<cb *>(button->user_data());
            }
        }

        void restartGame() {
            locked = false;
            logger.log(Logger::DEBUG, "Locked = false");
            setBox();
            for (auto [it, id] = std::tuple(buttons.begin(), 0); it != buttons.end(); ++it, ++id) {
                (*it)->copy_label("");
                (*it)->activate();
                (*it)->align(FL_ALIGN_INSIDE | FL_ALIGN_WRAP | FL_ALIGN_CLIP);
                (*it)->callback(
                        [](Fl_Widget *sender, void *data) { // add an onClick event to send the message to the server
                            auto func = static_cast<std::function<void()> *>(data);
                            (*func)();
                        }, new cb([=, this] {
                            if (!locked) { // send only if your turn
                                socket.sendMessage("put " + std::to_string(id));
                            }
                        }));
            }
        }

        void GenerateBoard() {
            for (int i = 0; i < _cells; ++i) {
                for (int j = 0; j < _cells; ++j) {
                    buttons.emplace_back(
                            std::make_unique<Fl_Button>(BUTTON_SIZE * j, BUTTON_SIZE * i + 150,
                                                        BUTTON_SIZE, BUTTON_SIZE)
                    );
                }
            }
            logger.log(Logger::DEBUG, "Board generated.");
        }

        void setCell(const std::string &str, size_t index) {
            buttons[index]->copy_label(str.c_str()); // puts a char to the button
            buttons[index]->deactivate();

            locked ^= 1; // change the turn locally
            logger.log(Logger::DEBUG, "SetCell " + std::to_string(index) + " to " + str);
        }

        void setBox() { // change box with info
            box.copy_label(locked ? "Opponent Turn" : "Your Turn");
        }

        friend void getGameMessage();

    } gameWindow;

    class RegisterWindow : public Fl_Window { // window for registration
    private:
        Fl_Input loginInput{80, 10, 100, 25, "Login:"};
        Fl_Secret_Input passwordInput{80, 45, 100, 25, "Password:"};
        Fl_Secret_Input passwordConfirmInput{80, 80, 100, 25, "Confirm:"};

        Fl_Button cancelButton{10, 135, 80, 45, "Cancel"};
        Fl_Button registerButton{110, 135, 80, 45, "Register"};

        bool isLoginValid() {
            //const std::regex passwordRegex(R"(^[a-z][^:&.'"\s]{3,14}$)", std::regex_constants::icase);

            const std::string login = std::string(loginInput.value());

            const std::regex firstLetter("(^[a-zA-Z]+)");
            const std::regex badChars(R"([^:&.'"\s]+)");
            const std::regex size(R"([^\W]{4,15})");

            if (!std::regex_match(login.begin(), login.end(), firstLetter)) {
                fl_message("The first character of the login should be a letter!");
                return false;
            }
            if (!std::regex_match(login.begin(), login.end(), badChars)) {
                fl_message("Login should not contain [: & . , ' \" spaces]");
                return false;
            }
            if (!std::regex_match(login.begin(), login.end(), size)) {
                fl_message("The login size should be from 4 to 15");
                return false;
            }

            return true;
        }

        bool isPasswordValid() {
            //const std::regex passwordRegex(R"(^[a-z][^:&.'"\s]{5,15}$)", std::regex_constants::icase);

            const std::string password = std::string(passwordInput.value());

            const std::regex firstLetter("(^[a-zA-Z]+)");
            const std::regex badChars(R"([^:&.'"\s]+)");
            const std::regex size(R"([^\W]{6,16})");

            if (!std::regex_match(password.begin(), password.end(), firstLetter)) {
                fl_message("The first character of the password should be a letter!");
                return false;
            }
            if (!std::regex_match(password.begin(), password.end(), badChars)) {
                fl_message("Password should not contain [: & . , ' \" spaces]");
                return false;
            }
            if (!std::regex_match(password.begin(), password.end(), size)) {
                fl_message("The password size should be from 6 to 16");
                return false;
            }

            return true;
        }

    public:
        RegisterWindow() : Fl_Window(200, 200, "Register") {
            cancelButton.callback([](Fl_Widget *sender, void *data) {
                auto func = static_cast<std::function<void()> *>(data);
                (*func)();
            }, new cb([=, this] {
                this->hide();
            }));

            registerButton.callback([](Fl_Widget *sender, void *data) {
                auto func = static_cast<std::function<void()> *>(data);
                (*func)();
            }, new cb([=, this] { // callbacks to the buttons
                if (std::string(passwordInput.value()) == std::string(passwordConfirmInput.value())) {
                    if (isLoginValid() && isPasswordValid()) {
                        socket.sendMessage(std::string("reg ") + loginInput.value() + " " + passwordInput.value());
                        std::string status = socket.getMessage();
                        if (status == "400") {
                            fl_message("Login already used!");
                        } else if (status == "200") {
                            fl_message("Successfully registered!");
                            this->hide();
                        } else if (status == "shutdown") {
                            fl_message("Server is down.");
                            this->hide();
                        }
                    }
                } else {
                    fl_message("Passwords mismatch! Please try again.");
                }
            }));
        }
    } regWindow;

    class WaitingWindow : public Fl_Window {
    private:
        Fl_Box box{0, 0, 300, 50, "Waiting for an opponent..."};
    public:
        WaitingWindow() : Fl_Window(300, 150, "Waiting") {
            box.align(FL_ALIGN_CENTER | FL_ALIGN_INSIDE | FL_ALIGN_CLIP);
        }
    } waitingWindow;


    void getGameMessage() { // async message handler
        while (socket.isActive) {
            std::string data = socket.getMessage(); // unlock when get a message

            if (data == "restart") {
                gameWindow.restartGame();
                gameWindow.show();
                waitingWindow.hide();
                logger.log(Logger::DEBUG, "Restarting.");
                continue;
            }

            if (data == "lock") { // lock random client
                gameWindow.locked = true;
                gameWindow.setBox();
                logger.log(Logger::DEBUG, "Locked = true.");
                continue;
            }

            if (data == "win" || data == "draw" || data == "disconnect") {
                logger.log(Logger::DEBUG, "End of the game.");
                fl_message(data == "disconnect" ? "Opponent is disconnected, You Win." :
                           data == "draw" ? "Draw" :
                           gameWindow.locked ? "You Win" : "You Lose");
                if (fl_choice("Would you like to play again?", fl_no, fl_yes, nullptr) == 1) {
                    socket.sendMessage("again");
                    waitingWindow.show();
                }
                gameWindow.hide();
                continue;
            }

            if (data == "shutdown") {
                logger.log(Logger::INFO, "Shutdown.");
                fl_message("Server is down.");
                gameWindow.hide();
                waitingWindow.hide();
                continue;
            }

            gameWindow.setCell(std::string{data[0]}, std::stoul(data.substr(1)));
            gameWindow.setBox();
        }
    }

    void runGameMessageThread() {
        auto th = std::thread(&getGameMessage);
        th.detach();
    }

    class LoginWindow : public Fl_Window { // login window :)
    private:
        Fl_Input loginInput{80, 10, 100, 25, "Login:"};
        Fl_Secret_Input passwordInput{80, 45, 100, 25, "Password:"};

        Fl_Button registerButton{10, 100, 80, 45, "Register"};
        Fl_Button loginButton{110, 100, 80, 45, "Login"};

    public:
        LoginWindow() : Fl_Window(200, 160, "Login") {
            loginButton.callback([](Fl_Widget *sender, void *data) {
                auto func = static_cast<std::function<void()> *>(data);
                (*func)();
            }, new cb([=, this] {
                if (std::string login(loginInput.value()), password(passwordInput.value());
                        !login.empty() && !password.empty()) {
                    socket.sendMessage(std::string("log ") + loginInput.value() + " " + passwordInput.value());
                    std::string status = socket.getMessage();
                    if (status == "404") {
                        fl_message("User not found!");
                    } else if (status == "401") {
                        fl_message("Wrong password!");
                    } else if (status == "405") {
                        fl_message("User is already logged in!");
                    } else if (status == "200") {
                        runGameMessageThread();
                        waitingWindow.show();
                        this->hide();
                    } else if (status == "shutdown") {
                        fl_message("Server is down.");
                        this->hide();
                    }
                } else {
                    fl_message("Please enter login and password!");
                }
            }));
            registerButton.callback([](Fl_Widget *sender, void *data) {
                regWindow.show();
            }, this);
        }
    } loginWindow;

}

int main() {
    TicTacToe::loginWindow.show();

    return Fl::run();
}