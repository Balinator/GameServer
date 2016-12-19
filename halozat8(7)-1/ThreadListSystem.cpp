#include "ThreadListSystem.h"



ThreadListSystem::ThreadListSystem()
{

}


ThreadListSystem::~ThreadListSystem()
{

}

static ThreadListSystem getInstance();
static void addListElement(SysThread* th);
static void removeListElement();
static void sendToAll(string* str);
