# TicTacToe

This project is an online Tic-Tac-Toe game with a graphical user interface (GUI) built using the
__[FLTK library](https://www.fltk.org/doc-1.3/index.html)__ . It allows users to play the classic Tic-Tac-Toe game
against each other over a network.

## Building

1. __Clone the Repository:__

~~~
git clone --recurse-submodules --remote-submodules git@github.com:ChoiZez/TicTacToe.git
~~~

2. __Build and install the FLTK Library__

__Build:__

~~~
mkdir buildFLTK
cd buildFLTK
cmake ../fltk
cmake --build .
~~~

__Install:__

~~~
sudo cmake --install .
~~~

3. __Build the Project__

In the root folder:

~~~
mkdir build
cd build
cmake ..
cmake --build .
~~~

Now files will be in the `build/src` folder

## Running

Go to the `/build/src`

### Server

Run server with `./server`

### Client

Run client with `./client`

__Please make sure that you start the server before the client.__

## Usage

### Server

Just run the server and be happy :)

### Client

You should register and login before you are allowed to play. Just follow the messages.