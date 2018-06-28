#pragma once
#include "commbase.h"
class CommLG :
	public CommBase
{
public:
	CommLG(CommManager *manager);
	virtual ~CommLG(void);
};

