#pragma once
#include <limits.h>
class Elise
{
public:
	Elise();
	~Elise();


	void Generate_a();
	double GetA();
	void SetB( double BB ) { B = BB; }

	double Get_s();

	double a = INT_MIN;
	double B = INT_MIN;
};

