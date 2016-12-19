#pragma once
#include<mutex>

using namespace std;
class Lock
{
public:
	//konstruktor, mutex lokkolasa
	Lock(mutex &mutex):m_mutex(mutex),m_locked(true) {
		mutex.lock();
	}

	//destruktor, mutex feloldasa
	~Lock() {
		m_mutex.unlock();
	}

	//zarojel operator, ami visszateriti, h szukseg van-e meg a lokkolasra
	operator bool() const {
		return m_locked;
	}
	//lokk virtualis feloldasa
	void setUnlock() {
		m_locked = false;
	}
private:
	mutex &m_mutex;
	bool m_locked;
};

