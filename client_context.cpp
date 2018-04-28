#include "network.h"

ClientContext::ClientContext()
{
	memset(this, 0, sizeof(OVERLAPPED));
}

ClientContext::~ClientContext()
{
}