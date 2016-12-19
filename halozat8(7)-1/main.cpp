#include<stdio.h>
#include<time.h>
#include<vector>
#include<list>
#include<mutex>

#include"winsock2.h"
#include"SysThread.h"
#include"Lock.h"

//mutexek konnyebb kezelesere
#define synchronized(M)  for(Lock M##_lock = M; M##_lock; M##_lock.setUnlock())

//fugveny a lekapcsolt szerverek tredjeinek lezárására
bool deleteStopped(SysThread* th) {
	if (th->isExited()) {
		th->~SysThread();
		return true;
	}
	return false;
}

int main() {

	printf("Szerver Indulasa\n");

	WSAData wsa;
	SOCKET soc;
	sockaddr_in adr;
	const int buflen = 1024;
	char buf[buflen];
	int port = 10013;

	int result = WSAStartup(MAKEWORD(2, 2), &wsa);

	printf("WSA inicializalasa\n");

	if (result != NO_ERROR) {
		printf("Hiba a WSAStartup(...) parancsnal!\n");
		system("pause");
		return 1;
	}

	printf("Szerver socket inicializalasa\n");

	soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (soc == INVALID_SOCKET) {
		printf("Hiba a socket() parancsnal! Hiba: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	printf("Szerver cim inicializalasa\n");

	adr.sin_addr.s_addr = inet_addr("127.0.0.1");
	adr.sin_family = AF_INET;
	adr.sin_port = htons(port);

	printf("Bindeles\n");
	if (::bind(soc, (SOCKADDR*)&adr, sizeof(adr)) == SOCKET_ERROR) {
		printf("Hiba a bind() parancsnal! Hiba: %ld\n", WSAGetLastError());
		closesocket(soc);
		WSACleanup();
		system("pause");
		return 1;
	}

	printf("Fogadas inicializalasa\n");

	int maxLisenings = 2;

	if (listen(soc, maxLisenings) == SOCKET_ERROR) {
		printf("Hiba a listen() parancsnal! Hiba: %ld\n", WSAGetLastError());
		closesocket(soc);
		WSACleanup();
		system("pause");
		return 1;
	}

	SOCKET *acceptSoc;
	SysThread *thread;
	int count = 2;
	list<SysThread*> threadList;
	mutex listMutex;

	do {
		printf("Csatlakozasra varakozas\n");

		acceptSoc = new SOCKET(accept(soc, NULL, NULL));
		if (*acceptSoc == INVALID_SOCKET) {
			printf("Hiba az accept() parancsnal! Hiba: %ld\n", WSAGetLastError());
			closesocket(soc);
			WSACleanup();
			system("pause");
			return 1;
		}

		//atadodik a socket, a tredek listaja, es a lista mutex-e
		thread = new SysThread(acceptSoc,threadList,listMutex);
		threadList.push_back(thread);
		printf("Sikeres csatlakozas(%s)\n", acceptSoc);
		thread->start();
		//
		
		synchronized(listMutex) {
			threadList.remove_if(deleteStopped);//toroljuk a megallitott tredeket
		}
	} while (threadList.size() > 0);


	closesocket(soc);
	::WSACleanup();

	printf("Program bezarasa\n");

	system("pause");

	return 0;
}