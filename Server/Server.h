#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "CardClass.h"
#include <thread>
#include <iostream>
#include <unordered_map>
#include <WinSock2.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

class Server {
private:
	SOCKET listner;
	sockaddr_in addr;
	bool status = true;//are we waiting for new clients
	bool gamestatus = false;
	//(Preflop)
	//(Flop)
	//(Turn)
	//(River)
public:
	struct Client {
		bool nothread = true;//new thread for every client
		int money = 950;
		bool ingame = false;
		bool bigblind = false;
		bool smallblind = false;
		string name = "";
		vector <Card> hand;//size 2
	};
	vector <Card> board;//size 0 on preflop, 5 at river
	unordered_map <SOCKET, Client> clients;
	void onlytell(SOCKET curr_sock, string msg, bool spam = false) {//spam may be user to give info of game field
		do {
			send(curr_sock, msg.c_str(), msg.size(), 0);
		} while (spam);
	}

	string onlyhear(SOCKET curr_sock) {//hear
		char buf[2000];
		memset(buf, 0, 2000);
		recv(curr_sock, buf, 2000, 0);
		return buf;
	}
	void welcome(SOCKET curr_sock) {
		string msg;
		string data;
		if (clients[curr_sock].name == "") {//first connect no name yet
			msg = "what is your name?";
			data = "";
			onlytell(curr_sock, msg);//server asks for a name for SOCKET with no name
			data = onlyhear(curr_sock);//cleint says his name
			clients[curr_sock].name = data;
		}
		else {//name is not empty => client reconnected
			msg = "welcome back " + clients[curr_sock].name + "!\n";
			if (!gamestatus) {
				msg += "send 'r' if you are ready\n";//he can join if there is no game yet
			}
			else {
				msg += "the game is in process, wait for end of this round\n";//he has to wait if game is on
			}
			onlytell(curr_sock, msg);
		}
	}
	void start() {
		gamestatus = true;
		int players = 0;
		for (auto it = clients.begin(); it != clients.end(); ++it)
			if ((*it).second.ingame == true)
				++players;
				//give cards - preflop
				//bets
		if (players > 1) {
			//flop
			//bets
		}
		
		if (players > 1) {
			//turn
			//bets
		}
		
		if (players > 1) {
			//river
			//bets
		}
		
		//findwinner
		
		/*writeresult to txt*/
		gamestatus = false;
	}
	void trytostart(SOCKET curr_sock) {
		bool allready = true;
		clients[curr_sock].ingame = true;
		for (auto it = clients.begin(); it != clients.end(); ++it) {
			allready *= clients[curr_sock].ingame;
			if ((*it).first != curr_sock)
				onlytell((*it).first, clients[curr_sock].name + " is ready");
		}
		if (allready) {
			start();
		}
	}
	void TalkToClient(SOCKET curr_sock) {//TalkToClient is tell + hear, all server logic should be here
		string msg;
		string data;
		welcome(curr_sock);
		do {
			data = onlyhear(curr_sock);//
			if (data == "r") {
				trytostart(curr_sock);
				while (gamestatus == false) {
					//waiting game to start
				}//all the dialog will be in start()
				while (gamestatus == true) {
					//waiting game to end
				}
			}
			onlytell(curr_sock, msg);//
			
			//change msg with known data
		} while (data.size());
		this->clients[curr_sock].nothread == true;
		closesocket(curr_sock);
	}

	void createlistener() {
		WSADATA wsd;
		WSAStartup(MAKEWORD(2, 2), &wsd);
		SOCKET listener = socket(AF_INET, SOCK_STREAM, 0); //Создаем слушающий сокет
		if (listener == INVALID_SOCKET)
			cout << "Error with creating socket" << endl; //Ошибка создания сокета
		fd_set list;
		list.fd_array[50] = listener;
		sockaddr_in addr; //Создаем и заполняем переменную для хранения адреса
		addr.sin_family = AF_INET; //Семейство адресов, которые будет обрабатывать наш сервер, у нас это TCP/IP адреса
		addr.sin_port = htons(3128); //Используем функцию htons для перевода номера порта в TCP/IP представление
		addr.sin_addr.s_addr = htonl(INADDR_ANY); //INADDR_ANY означает, что сервер будет принимать запросы с любых IP
		if (SOCKET_ERROR == ::bind(listener, (struct sockaddr*)&addr, sizeof(addr))) //Связываем сокет с адресом
			cout << "Error with binding socket";
		this->listner = listener;
		this->addr = addr;
	}

	void waitforcon() {
		while (this->status) {
			listen(this->listner, 1);
			int len = sizeof(this->addr);
			SOCKET curr_sock = accept(this->listner, (struct sockaddr*)&this->addr, &len);
			string ip = inet_ntoa(addr.sin_addr);
			//0.0.0.0 is listener socket ip
			if (ip != "0.0.0.0" && this->clients[curr_sock].nothread) {
				this->clients[curr_sock].nothread = false;

				thread thr(&Server::TalkToClient, this, curr_sock);
				thr.detach();
			}
		}
	}

	Server() {
		createlistener();
		thread newclient(&Server::waitforcon, this);
		newclient.detach();
	}
	
	~Server() {
		for (auto it = this->clients.begin(); it != this->clients.end(); ++it)
			closesocket((*it).first);
	}
};