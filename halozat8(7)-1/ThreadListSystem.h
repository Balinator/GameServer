#pragma once
#include<list>
#include<string>

#include"SysThread.h"

using namespace std;
class ThreadListSystem
{
private:
	list<SysThread*> lista;
	list<string*> strs;
	ThreadListSystem();
	~ThreadListSystem();
public:
	static ThreadListSystem getInstance();
	static void addListElement(SysThread* th);
	static void removeListElement();
	static void sendToAll(string* str);
};

