#include "bccounter.h"
#include "mutex.h"

BCCounter::BCCounter()
{
	value = 0;
	mutex = new Mutex;
}

BCCounter::~BCCounter()
{
	delete mutex;
}

void BCCounter::up()
{
	mutex->lock();
	value++;
	printf("BCCounter::up %p %d\n", this, value);
	mutex->unlock();
}

void BCCounter::down()
{
	mutex->lock();
	value--;
	printf("BCCounter::down %p %d\n", this, value);
	mutex->unlock();
}
