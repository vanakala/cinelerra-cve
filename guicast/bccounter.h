// Counter for memory leakage detection

#include "mutex.inc"

class BCCounter
{
public:
	BCCounter();
	~BCCounter();

	void up();
	void down();

	Mutex *mutex;
	int value;
};
