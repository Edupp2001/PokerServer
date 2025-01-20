#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "CardClass.h"
#include <thread>
#include <iostream>
#include <unordered_map>
#include <WinSock2.h>
#include <array>
#include <fstream>
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
		int place = INVALID_SOCKET;
		string name = "";
		vector <Card> hand;//size 2
	};

	vector <Card> board;//size 0 on preflop, 5 at river
	unordered_map <SOCKET, Client> clients;
	array <SOCKET, 8> table;
	int dealer = 7;
	void onlytell(SOCKET curr_sock, const string& msg, bool spam = false) {//spam may be user to give info of game field
		do {
			send(curr_sock, msg.c_str(), msg.size(), 0);
		} while (spam);
	}
	void telleveryone(string msg, bool hand = false, SOCKET exception = INVALID_SOCKET) {
		for (auto it = clients.begin(); it != clients.end(); ++it) {
			if (hand) {
				msg += "\n" + (*it).second.hand[0].value.first + " of " + (*it).second.hand[0].value.second + "\n";
				msg += (*it).second.hand[1].value.first + " of " + (*it).second.hand[1].value.second;
			}
			if ((*it).first != exception)
				onlytell((*it).first, msg);
		}
	}
	void write(string data) {
		ofstream results("results.txt", ios::app);
		results << data;
		results.close();
	}
	string onlyhear(SOCKET curr_sock) {//hear
		char buf[2000];
		memset(buf, 0, 2000);
		if (recv(curr_sock, buf, 2000, 0) >= 0) {
			return "quit";
		}
		return buf;
	}
	void welcome(SOCKET curr_sock) {
		string msg;
		string data;
		msg = "what is your name?";
		data = "";
		onlytell(curr_sock, msg);//server asks for name
		data = onlyhear(curr_sock);//client says his name
		clients[curr_sock].name = data;
		msg = "where you want to sit?\n";
		for (int i = 0; i < table.size(); ++i) {
			if (table[i] == INVALID_SOCKET) {
				msg += ITS(i) + " ";
			}
			else {
				msg += clients[table[i]].name + " ";
			}
		}
		onlytell(curr_sock, msg);
	}
	int bets() {

	}
	int next(int n) {
		if (table[(n + 1) % 8] == INVALID_SOCKET) {
			return next((n + 1) % 8);
		}
		return (n + 1) % 8;
	}
	string start() {
		dealer = next(dealer);
		gamestatus = true;
		string msg = "";
		int players = 0;
		vector <Card> deck = create_deck();
		for (int i = 0; i < table.size(); ++i){
			if (table[i] != INVALID_SOCKET && clients[table[i]].ingame == true) {
				clients[table[i]].hand.push_back(deck[deck.size() - 1]);
				deck.pop_back();
				clients[table[i]].hand.push_back(deck[deck.size() - 1]);
				deck.pop_back();
				++players;
				msg += clients[table[i]].name + " ";
			}
		}
		telleveryone(msg, true);
		
		int order = next(dealer);




		if (players > 1) {
			//flop
			players = bets();
		}
		
		if (players > 1) {
			//turn
			players = bets();
		}
		
		if (players > 1) {
			//river
			players = bets();
		}
		
		//findwinner
		string winner;
		
		/*writeresult to txt*/
		gamestatus = false;
		
		return winner;
	}
	string trytostart(SOCKET curr_sock) {
		if (gamestatus == true)//cant start game if it is going already
			return "";
		bool allready = true;
		clients[curr_sock].ingame = true;//update \table
		for (auto it = clients.begin(); it != clients.end(); ++it) {//need to check if only 1 player
			allready *= clients[curr_sock].ingame;
			if ((*it).first != curr_sock)
				onlytell((*it).first, clients[curr_sock].name + " is ready");
		}
		if (allready) {
			return start();
		}
		return "";
	}
	void TalkToClient(SOCKET curr_sock) {//TalkToClient is tell + hear, all server logic should be here
		string msg;
		string data;
		welcome(curr_sock);
		do {
			data = onlyhear(curr_sock);//
			if (data == "r") {
				if (clients[curr_sock].place != INVALID_SOCKET) {//if he has a place
					msg = trytostart(curr_sock);
					if (msg == "") {
						while (gamestatus == false) {
							//waiting game to start
						}//all the dialog will be in start()
						while (gamestatus == true) {
							//waiting game to end
						}
					}
				}
				else {
					msg = "need to sit down first";
				}
			}
			else if (data == "standup" && clients[curr_sock].place != INVALID_SOCKET) {//leave place
				int place = clients[curr_sock].place;
				msg = clients[curr_sock].name + " leaves place #" + ITS(place);
				table[place] = INVALID_SOCKET;
				clients[curr_sock].place = INVALID_SOCKET;
				msg += ITS(place);
				telleveryone(msg);
			}
			else if (data.size() == 1 && (data[0] >= '0' && data[0] <= '7') && clients[curr_sock].place == INVALID_SOCKET) {//take place
				int place = STI(data);
				if (table[place] == INVALID_SOCKET) {//the place is free
					table[place] = curr_sock;//take
					clients[curr_sock].place = place;
					msg = clients[curr_sock].name + "'s place is #" + data;
					telleveryone(msg);
					msg = "say `r` when ready or `standup` to leave place";//tell to client his options
					while (gamestatus == true) {
						//if game is going wait to end
					}
				}
				else {
					msg = "this place is occupied";
				}
			}
			else if (data == "quit") {
				clients[curr_sock].nothread == true;
			}
			else{
				msg = "?";
			}
			onlytell(curr_sock, msg);//
		} while (clients[curr_sock].nothread == false);
		
		write('`' + clients[curr_sock].name + "` quits with " + ITS(clients[curr_sock].money) + '\n');
		closesocket(curr_sock);
		clients.erase(curr_sock);
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