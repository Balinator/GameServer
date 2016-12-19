#pragma once
#include<list>
#include<vector>
#include<sstream>
#include<windows.h>
#include<stdio.h>
#include<mutex>
#include<string>

#include"Lock.h"

using namespace std;

class SysThread
{
public:
	SysThread(SOCKET *s, list<SysThread*>& l, mutex& m);
	virtual ~SysThread();
	virtual bool start(void);
	virtual bool stop(unsigned int timeout = 0);
	inline volatile bool& isRunning(void)
	{
		return m_bRunning;
	}
	inline volatile bool& isExited(void)
	{
		return m_bExited;
	}

	static const unsigned int INFINIT_WAIT;
protected:
	virtual void run(void);
	/*Ezt a metodust a származtatott osztályban felül kell írni.
	Ide kell beírni az utasítás szekvenciát amit a szálunk végre kell hajtson*/	
private: //static
	static const int buflen;

	static const char BREAK_CHAR;//elvalaszto karakter
	static const char BREAK_CHAR_MSG_START;//elvalaszto karakter eleje
	static const char BREAK_CHAR_MSG_STOP;//elvalaszto karakter vege

	//tipus konstatnsok
	static const string TYPE_LOGIN;
	static const string TYPE_DISCONNECT;
	static const string TYPE_MSG;
	
private:
	friend DWORD WINAPI runStub(LPVOID mthread);

	volatile bool m_bRunning;
	volatile bool m_bExited;
	
	HANDLE m_thread;

	//socket, lista, mutex amit parameterkent atadtam
	SOCKET *m_soc;
	list<SysThread*> &m_list;
	mutex &m_listMutex;

	//fontos valtozok
	volatile bool m_bNeeded;
	volatile bool m_bIsLoggedIn;
	char* m_OldMsg;
	list<string> m_Msg;
	//olvaso funkciok
	void readMsg(char* buf, const int buflen);
	bool getNext(string& buf);
	//kuldes funkciok
	bool sendToClient(const char* client, const char* msg);//nem
	void sendToAll(const char* msg);
	//ellenorzes funkciok
	bool chackExit(char* data);
	//ezek kozul a kezdeti
	void chackStart(char* data);
	bool chackLogin(char* data);
	//ezek kozul a futasi
	void chackRun(char* data);
	bool chackMsg(char* data);
};
