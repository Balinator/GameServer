#include "SysThread.h"

#define synchronized(M)  for(Lock M_lock = M; M_lock; M_lock.setUnlock())

#define INVALID_HANDLE_VALUE 0
const unsigned int SysThread::INFINIT_WAIT = UINT_MAX;

//altaklam megharatozott konstansok inicializacioja
const int SysThread::buflen = 1024;

const char SysThread::BREAK_CHAR = (char)178;
const char SysThread::BREAK_CHAR_MSG_START = (char)179;
const char SysThread::BREAK_CHAR_MSG_STOP = (char)180;//torolni a globalt

const string SysThread::TYPE_LOGIN = "1";//log
const string SysThread::TYPE_DISCONNECT = "2";//dis
const string SysThread::TYPE_MSG = "3";//send objects, pos, stb

//darabolo funkcio(javabol atirva)
void split(const std::string &s, char delim, std::vector<std::string> &elems) {
	std::stringstream ss;
	ss.str(s);
	std::string item;
	while (std::getline(ss, item, delim)) {
		elems.push_back(item);
	}
}
std::vector<std::string> split(const std::string &s, char delim) {
	std::vector<std::string> elems;
	split(s, delim, elems);
	return elems;
}

//konstruktor, inicializaljuk a bekert parameterekkel a hozzajuk rendelt valtozot
SysThread::SysThread(SOCKET* s, list<SysThread*>& l, mutex& m):m_soc(s),m_list(l),m_listMutex(m)
{
	m_bRunning = false;
	m_bExited = true;
	m_thread = INVALID_HANDLE_VALUE;
}
//destruktor, toroljuk a socketpointert
SysThread::~SysThread()
{
	delete m_soc;
}
//start es stop, ezek a threadet valositjak meg
bool SysThread::start(void)
{
	if (m_bExited)
	{
		m_bExited = false;
		DWORD dw;
		if ((m_thread = CreateThread(NULL, 4096, runStub, this, 0, &dw)) == INVALID_HANDLE_VALUE)
		{
			m_bRunning = false;
			m_bExited = true;
			return false;
		}
	}
	return true;
}
bool SysThread::stop(unsigned int timeout)
{
	m_bRunning = false;
	if (!m_bExited)
	{
		for (unsigned int i = 0; (i <= timeout / 100) || (timeout ==
			INFINIT_WAIT); i++)
		{
			m_bRunning = false;
			if (m_bExited)
			{
				break;
			}
			Sleep(10);
		}
	}
	if (m_thread != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_thread);
		m_thread = INVALID_HANDLE_VALUE;
	}
	return m_bExited;
}
//fo ciklus
void SysThread::run(void)
{
	//valtozok inicializalasa
	m_bNeeded = true;
	m_bIsLoggedIn = false;
	char tempbuf[buflen];
	m_OldMsg = (char*)calloc(buflen, sizeof(char));

	//kezdo ciklus
	while (m_bNeeded && m_bIsLoggedIn == false) {
		readMsg(tempbuf, buflen);//uzenet beolvasasa, darabolva
		while (m_Msg.size() > 0 && m_bNeeded) {//amig van darabolt uzenet
			strcpy(tempbuf, m_Msg.front().c_str());
			m_Msg.pop_front();
			chackStart(tempbuf);//ellenorzes
		}
	}

	//futasi ciklus
	while (m_bNeeded) {
		readMsg(tempbuf, buflen);//uzenet beolvasasa, darabolva
		while(m_Msg.size() > 0 && m_bNeeded) {//amig van darabolt uzenet
			strcpy(tempbuf, m_Msg.front().c_str());
			m_Msg.pop_front();
			chackRun(tempbuf);//ellenorzes
		}
	}
	//felszabaditas es kapcsolat zarasa
	delete m_OldMsg;
	closesocket(*m_soc);
	printf("Kapcsolat bontasa(%s)\n", m_soc);
}
//uzenet beolvasasa es darabolasa
void SysThread::readMsg(char* buf, const int buflen) {
	int len = recv(*m_soc, buf, buflen - 1, 0);//beolvas
	buf[len] = '\0';
	string str = buf;
	while (getNext(str) && m_bNeeded) {}//feldarabol
}
//darabolo fugveny
bool SysThread::getNext(string& buf) {
	if (strlen(m_OldMsg) > 0) {//ha van meg,maradt feluzenet
		string s = m_OldMsg;
		int f = buf.find(BREAK_CHAR_MSG_STOP);
		if (f != string::npos) {
			s.append(buf.substr(0, f + 1));
			m_Msg.push_back(s);
			buf = buf.substr(f + 1);
			strcpy(m_OldMsg,"\0");
		}
	} else {//ha nincs megmaradva uzenet
		int fstop = buf.find(BREAK_CHAR_MSG_STOP);
		if (fstop != string::npos) {
			string s = buf.substr(0, fstop + 1);
			m_Msg.push_back(s);
			buf = buf.substr(fstop + 1);
		} else {
			strcpy(m_OldMsg, buf.c_str());
			buf = "";
		}
	}

	if (buf.length() > 0) {
		return true;
	}
	return false;
}
//kuldo funkciok
void SysThread::sendToAll(const char * msg)
{
	//uzenet elokeszitese (kezdo es vegso karakter beszurasa)
	string s = BREAK_CHAR_MSG_START + msg + BREAK_CHAR_MSG_STOP;
	char* newMsg = new char[buflen];
	char cp[2];
	cp[0] = BREAK_CHAR_MSG_START;
	cp[1] = '\0';
	strcpy(newMsg, cp);
	strcat(newMsg, msg);
	cp[0] = BREAK_CHAR_MSG_STOP;
	strcat(newMsg, cp);
	//kikuldes
	for (Lock m_listMutex_lock = m_listMutex; m_listMutex_lock; m_listMutex_lock.setUnlock()) {	
		for (SysThread *t : m_list) {
			if (t != nullptr && !t->isExited() && t != this) {
				send(*(t->m_soc), newMsg, strlen(newMsg), 0);
			}
		}
	}
	delete newMsg;
}
bool SysThread::sendToClient(const char * client, const char * msg)
{
	//elokeszites
	string s = BREAK_CHAR_MSG_START + msg + BREAK_CHAR_MSG_STOP;
	char* newMsg = new char[buflen];
	char cp[2]; 
	cp[0] = BREAK_CHAR_MSG_START;
	cp[1] = '\0';
	strcpy(newMsg, cp);
	strcat(newMsg, msg);
	cp[0] = BREAK_CHAR_MSG_STOP;
	strcat(newMsg, cp);
	bool ok = false;
	//kuldes
	for (Lock m_listMutex_lock = m_listMutex; m_listMutex_lock; m_listMutex_lock.setUnlock()) {
		for (SysThread *t : m_list) {
			if (t != nullptr && !t->isExited() && t == this) {
				send(*(t->m_soc), newMsg, strlen(newMsg), 0);
				ok = true;
				break;
			}
		}
	}
	delete newMsg;
	return ok;
}

//bejelentkezes utani uzenet ellenorzese
void SysThread::chackRun(char* data) {
	//kezdo es vegso karakter levagasa
	int len = strlen(data) - 2;
	for (int i = 0; i < len; ++i) {
		data[i] = data[i + 1];
	}
	data[len] = '\0';
	if (chackExit(data)) { //kilepes
		return; 
	}else if (chackMsg(data)) {//uzenetkuldes
		return;
	}
}

//bejelentkezes elotti uzenet ellenorzese
void SysThread::chackStart(char* data) {
	//kezdo es vegso karakter levagasa
	int len = strlen(data) - 2;
	for (int i = 0; i < len; ++i) {
		data[i] = data[i + 1];
	}
	data[len] = '\0';
	if (chackExit(data)) {//kilepes
		return;
	}else if (chackLogin(data)) {//bejelentkezes
		return;
	}
}

//kilepes ellenorzese
bool SysThread::chackExit(char* data) {//id,2,x/y,asd
	vector<string> mem = split(data, BREAK_CHAR);
	if (mem.size() == 4) {//ha legalabb 4 darabu a csomag es ezzel kilepest jelez a felhasznalo
		if (mem[1] == TYPE_DISCONNECT) {
			sendToAll(data);//lekuldeni, h kilepett
			m_bNeeded = false;
			return true;
		}
	}
	return false;
}
//bejelentkezes
bool SysThread::chackLogin(char* data) {//id,LOGIN
	vector<string> mem = split(data, BREAK_CHAR);
	if (mem.size() == 2) {//ha 2 darabu a csomag es ezzel a bejelentkezest jelzi a felhasznalo
		if (mem[1] == TYPE_LOGIN) {
			m_bIsLoggedIn = true;
			return true;
		}
	}
	return false;
}
//szoveg ellenorzese
bool SysThread::chackMsg(char* data) {//id,MSG,x/y,asd
	vector<string> mem = split(data, BREAK_CHAR);
	if (mem.size() == 4) {//ha 5 darabu a csomag es ezzel az uzenetkuldest jelzi a felhasznalo
		if (mem[1] == TYPE_MSG) {
			sendToAll(data);
			return true;
		}
	}
	return false;
}

//thread hatterfugvenye
DWORD WINAPI runStub(LPVOID mthread)
{
	SysThread* pThread = static_cast<SysThread* >(mthread);
	pThread->m_bRunning = true;
	pThread->run();
	pThread->m_bRunning = false;
	pThread->m_bExited = true;
	return 0;
}