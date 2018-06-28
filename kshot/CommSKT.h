#pragma once
#include "commbase.h"
class CommSKT :
	public CommBase
{
public:
	CommSKT(CommManager *manager);
	virtual ~CommSKT(void);
};

