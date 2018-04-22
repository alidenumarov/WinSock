#include <iostream>
#include <WS2tcpip.h>
#include <string>
#include <sstream>

#pragma comment (lib, "ws2_32.lib")

using namespace std;
fd_set master;
//This is to determine clients count
int clientCnt = 0;
//To welcome connected clients
string welcomeMsg = "";
//Field to Play { 1, 2, 3 For determine columns | a, b, c for determine rows }
string raw = " - - - \r\n";
string field1 = "|*|*|*| 1\r\n", newField1 = field1;
string field2 = "|*|*|*| 2\r\n", newField2 = field2;
string field3 = "|*|*|*| 3\r\n", newField3 = field3;
string rawCoords = " a b c \r\n\n";

char buf[4096];

// If the game ends, the new field will be shown for both clients/players
void newField(SOCKET s) {
	field1 = newField1;
	field2 = newField2;
	field3 = newField3;
	send(s, "New game\r\n\n", 13 , 0);
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, field1.c_str(), field1.size() + 1, 0);
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, field2.c_str(), field2.size() + 1, 0);
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, field3.c_str(), field3.size() + 1, 0);
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, rawCoords.c_str(), rawCoords.size() + 1, 0);
}

//Rewrite the field, after each command clients will be known about what cell is empty or not
void rewriteFields(SOCKET s) {
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, field1.c_str(), field1.size() + 1, 0);
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, field2.c_str(), field2.size() + 1, 0);
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, field3.c_str(), field3.size() + 1, 0);
	send(s, raw.c_str(), raw.size() + 1, 0);
	send(s, rawCoords.c_str(), rawCoords.size() + 1, 0);
}

//If users enter other command than coordinates of field[a1, c2, a3, b3, etc.] 
//They will be known about this error
void informError(SOCKET fd_arr, char field) {
	ostringstream oss;
	oss << "Choose another! Here is already: '" << field << "'\r\n";
	string strOut = oss.str();
	send(fd_arr, strOut.c_str(), strOut.size(), 0);
}

//Check each time after user enters command,
//whether he/she wins or not
void isWin(SOCKET s1, SOCKET s2) {
	if ((field1[1] == 'O' && field2[1] == 'O' && field3[1] == 'O') ||
		(field1[3] == 'O' && field2[3] == 'O' && field3[3] == 'O') ||
		(field1[5] == 'O' && field2[5] == 'O' && field3[5] == 'O') ||
		(field1[1] == 'O' && field1[3] == 'O' && field1[5] == 'O') ||
		(field2[1] == 'O' && field2[3] == 'O' && field2[5] == 'O') ||
		(field3[1] == 'O' && field3[3] == 'O' && field3[5] == 'O') ||
		(field1[1] == 'O' && field2[3] == 'O' && field3[5] == 'O') ||
		(field1[5] == 'O' && field2[3] == 'O' && field3[1] == 'O')) {
			send(s1, "You Loose!\r\n", 13, 0);
			send(s2, "You Win!\r\n", 11, 0);
			newField(s1);
			newField(s2);
	}
	if ((field1[1] == 'X' && field2[1] == 'X' && field3[1] == 'X') ||
		(field1[3] == 'X' && field2[3] == 'X' && field3[3] == 'X') ||
		(field1[5] == 'X' && field2[5] == 'X' && field3[5] == 'X') ||
		(field1[1] == 'X' && field1[3] == 'X' && field1[5] == 'X') ||
		(field2[1] == 'X' && field2[3] == 'X' && field2[5] == 'X') ||
		(field3[1] == 'X' && field3[3] == 'X' && field3[5] == 'X') ||
		(field1[1] == 'X' && field2[3] == 'X' && field3[5] == 'X') ||
		(field1[5] == 'X' && field2[3] == 'X' && field3[1] == 'X')) {
		send(s1, "You Win!\r\n", 11, 0);
		send(s2, "You Loose!\r\n", 13, 0);
		newField(s1);
		newField(s2);
	}
	if (field1[1] != '*' && field1[3] != '*' && field1[5] != '*' &&
		field2[1] != '*' && field2[3] != '*' && field2[5] != '*' &&
		field3[1] != '*' && field3[3] != '*' && field3[5] != '*' ) {
		send(s1, "No Winner!\r\n", 12, 0);
		send(s2, "No Winner!\r\n", 12, 0);
		newField(s1);
		newField(s2);
	}
}

//Shows whether it is the him/her turn
void turnOf(SOCKET s1, SOCKET s2) {
	send(s1, "Wait your turn!\r\n", 17, 0);
	send(s2, "Your turn!\r\n", 12, 0);
}


void main()
{
	//To determine what user's queue to choose the cell
	bool turnX = false, turnO = false;
	//To Initialze winsock
	WSADATA wsData;
	WORD ver = MAKEWORD(2, 2);
	//Check for error
	int wsOk = WSAStartup(ver, &wsData);
	if (wsOk != 0) {
		cerr << "Can't Initialize winsock! Quitting" << endl;
		return;
	}
	// Create a socket
	SOCKET listening = socket(AF_INET, SOCK_STREAM, 0);
	if (listening == INVALID_SOCKET) {
		cerr << "Can't create a socket! Quitting" << endl;
		return;
	}

	// Bind the ip address and port to a socket
	sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(5);
	hint.sin_addr.S_un.S_addr = INADDR_ANY; 

	bind(listening, (sockaddr*)&hint, sizeof(hint));

	// The socket is for listening/server side
	listen(listening, SOMAXCONN);

	// Create the master file descriptor to work with sockets
	FD_ZERO(&master);

	// Add our first socket that we're working with; the listening socket! SERVER side to listen incoming connections from the client
	FD_SET(listening, &master);

	while (true) {
		// Make a copy of the master file descriptor set, this is IMPORTANT because
		// the call to select() is _DESTRUCTIVE_. The copy only contains the sockets that
		// are accepting inbound connection requests OR messages. 

		// EFor example, we have a server and it's master file descriptor set contains 2 items;
		// the listening socket and four clients. When you pass this set into select(), 
		// only the sockets that are interacting with the server are returned. Let's say
		// only one client is sending a message at that time. The contents of 'copy' will
		// be one socket. 

		// SO WE MAKE A COPY OF THE MASTER LIST TO PASS INTO select() !!!

		fd_set copy = master;

		// See who's talking to server side, determine the client
		int socketCount = select(0, &copy, nullptr, nullptr, nullptr);
		// Loop through all the current connections / potential connect
		for (int i = 0; i < socketCount; i++) {
			// Determine what client is 'talking'/ sending the message
			SOCKET sock = copy.fd_array[i];

			//Check whether it is server socket or not 
			//Do not give ability to connect more than 2 clients
			if (sock == listening && clientCnt < 2) {
				// Accept a new connection
				SOCKET client = accept(listening, nullptr, nullptr);

				// Add the new connection to the list of connected clients
				FD_SET(client, &master);

				//Determinr the clients
				clientCnt++;
				if (clientCnt == 1) {
					welcomeMsg = "Welcome to the 'Tic-Tac-Toe'!\r\n You are player X\r\n";
				} else if (clientCnt == 2) {
					welcomeMsg = "Welcome to the 'Tic-Tac-Toe'!\r\n You are player O\r\n";
				}
				
				// Send a welcome message to the connected client
				send(client, welcomeMsg.c_str(), welcomeMsg.size(), 0);
				rewriteFields(client);
			} else {
				// It's an inbound message 
				char buf[4096];
				ZeroMemory(buf, 4096);

				// Send message to other clients, and definiately NOT the listening socket
				SOCKET playerX = master.fd_array[1];
				SOCKET playerO = master.fd_array[2];

				while (true) {
					for (int i = 0; i < master.fd_count; i++) {
						while (turnO == false) {
							//To make buf empty
							ZeroMemory(buf, 4096);
							recv(playerO, buf, 4096, 0);
							//Check for coordinates
							if (buf[0] == 'a' && buf[1] == '1') {
								//whether it is empty or not
								if (field1[1] != '*') informError(playerO, field1[1]);
								else {
									field1[1] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false;break;
								}
							}
							else if (buf[0] == 'a' && buf[1] == '2') {
								if (field2[1] != '*') informError(playerO, field2[1]);
								else {
									field2[1] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false;  break;
								}
							}
							else if (buf[0] == 'a' && buf[1] == '3') {
								if (field3[1] != '*') informError(playerO, field3[1]);
								else {
									field3[1] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false;  break;
								}
							} else if (buf[0] == 'b' && buf[1] == '1') {
								if (field1[3] != '*') informError(playerO, field1[3]);
								else {
									field1[3] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false; break;
								}
							}
							else if (buf[0] == 'b' && buf[1] == '2') {
								if (field2[3] != '*') informError(playerO, field2[3]);
								else {
									field2[3] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false;  break;
								}
							}
							else if (buf[0] == 'b' && buf[1] == '3') {
								if (field3[3] != '*') informError(playerO, field3[3]);
								else {
									field3[3] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false; break;
								}
							}
							else if (buf[0] == 'c' && buf[1] == '1') {
								if (field1[5] != '*') informError(playerO, field1[5]); 
								else {
									field1[5] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false; break;
								}
							}
							else if (buf[0] == 'c' && buf[1] == '2') {
								if (field2[5] != '*') informError(playerO, field2[5]);
								else {
									field2[5] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false; break;
								}
							}
							else if (buf[0] == 'c' && buf[1] == '3') {
								if (field3[5] != '*') informError(playerO, field3[5]);
								else {
									field3[5] = 'O';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerO, playerX);
									turnO = true; turnX = false; break;
								}
							}
							else if (buf[1] != '\n') {
								ZeroMemory(buf, 4096);
								char strOut[22] = "Invalide command !\r\n";
								send(playerO, strOut, 22, 0);
							}
						}
						
						
						while (turnX == false) {
							ZeroMemory(buf, 4096);
							// Receive message
							recv(playerX, buf, 4096, 0);
							if (buf[0] == 'a' && buf[1] == '1') {
								if (field1[1] != '*') informError(playerX, field1[1]);
								else {
									field1[1] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[0] == 'a' && buf[1] == '2') {
								if (field2[1] != '*') informError(playerX, field2[1]);
								else {
									field2[1] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[0] == 'a' && buf[1] == '3') {
								if (field3[1] != '*') informError(playerX, field3[1]);
								else {
									field3[1] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[0] == 'b' && buf[1] == '1') {
								if (field1[3] != '*') informError(playerX, field1[3]);
								else {
									field1[3] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[0] == 'b' && buf[1] == '2') {
								if (field2[3] != '*') informError(playerX, field2[3]);
								else {
									field2[3] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false;  break;
								}
							}
							else if (buf[0] == 'b' && buf[1] == '3') {
								if (field3[3] != '*') informError(playerX, field3[3]);
								else {
									field3[3] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[0] == 'c' && buf[1] == '1') {
								if (field1[5] != '*') informError(playerX, field1[5]);
								else {
									field1[5] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[0] == 'c' && buf[1] == '2') {
								if (field2[5] != '*') informError(playerX, field2[5]);
								else {
									field2[5] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[0] == 'c' && buf[1] == '3') {
								if (field3[5] != '*') informError(playerX, field3[5]);
								else {
									field3[5] = 'X';
									rewriteFields(playerX); rewriteFields(playerO); isWin(playerX, playerO); turnOf(playerX, playerO);
									turnX = true; turnO = false; break;
								}
							}
							else if (buf[1] != '\n') {
								ZeroMemory(buf, 4096);
								char strOut[22] = "Invalide command !\r\n";
								send(playerX, strOut, 22, 0);
							}
						}
					}
				}
			}
		}
	}

	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	// Message to let users know what's happening.
	// Get the socket number
	// Send the goodbye message
	// Remove it from the master file list and close the socket

	FD_CLR(listening, &master);
	closesocket(listening);
	string msg = "Server is shutting down. Goodbye\r\n";

	while (master.fd_count > 0)
	{
		SOCKET sock = master.fd_array[0];

		send(sock, msg.c_str(), msg.size() + 1, 0);

		FD_CLR(sock, &master);
		closesocket(sock);
	}

	// Cleanup winsock
	WSACleanup();

	system("pause");
}