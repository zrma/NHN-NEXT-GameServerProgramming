#pragma once
#include <limits.h>
class Bob
{
public:
	Bob();
	~Bob();

	void Generate_b();
	double GetB();
	void SetA( double AA ) { A = AA; }

	double Get_s();

	double b = INT_MIN;
	double A = INT_MIN;

};

