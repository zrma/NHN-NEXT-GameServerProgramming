#include "stdafx.h"
#include "Bob.h"
#include "Common.h"
#include <stdlib.h>
#include <math.h> 

Bob::Bob()
{
}


Bob::~Bob()
{
}

void Bob::Generate_b()
{
	b = rand() % randMax;
	//b = 15;
	printf_s( "Bob_b:%lf \n", b );
}

double Bob::GetB()
{
	double B = fmod( pow( g, b ), p );
	printf_s( "BobB:%lf \n", B );
	return fmod( pow( g, b ), p );
}

double Bob::Get_s()
{
	return fmod( pow( A, b ), p );
}
