#include "stdafx.h"
#include "Elise.h"
#include "Common.h"
#include <stdlib.h>
#include <math.h> 

Elise::Elise()
{
}


Elise::~Elise()
{
}

void Elise::Generate_a()
{
	a = rand() % randMax;
	//a = 6;
	printf_s( "Elise_a:%lf \n", a );
}

double Elise::GetA()
{
	return fmod( pow( g, a ), p );
}

double Elise::Get_s()
{
	return fmod( pow( B, a ), p );
}
