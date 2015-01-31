// Diffie-HellmanTest1.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "Crypter.h"

int _tmain( int argc, _TCHAR* argv[] )
{
	std::cout << "Diffie-Hellman key exchange alogrithm example:\n";
	
	Crypter alice;
	Crypter bob;

	// Generation of exchange keys for Alice and Bob
	std::cout << "\nGeneration of exchange key for Alice...\n";
	alice.GenerateExchangeKey();
	std::cout << "Exchange key for Alice is equal:\n";
	alice.PrintExchangeKey();

	std::cout << "\nGeneration of exchange key for Bob...\n";
	bob.GenerateExchangeKey();
	std::cout << "Exchange key for Bob is equal:\n";
	bob.PrintExchangeKey();

	// Calculation if shared keys for Alice and Bob
	std::cout << "\nCalculation of shared key for Alice...\n";
	alice.CreateSharedKey( bob.GetExchangeKey() );
	std::cout << "Shared key for Alice is equal:\n";
	alice.PrintSharedKey();

	std::cout << "\nCalculation of shared key for Bob...\n";
	bob.CreateSharedKey( alice.GetExchangeKey() );
	std::cout << "Shared key for Bob is equal:\n";
	bob.PrintSharedKey();

	if ( alice.GetSharedKey() == bob.GetSharedKey() )
	{
		std::cout << "\nOperation finished successfully: shared keys for Alice and Bob are equal.";
	}
	else
	{
		std::cout << "\nOperation failed: shared keys for Alice and Bob are different.";
	}

	char test[16] = { 
		'a', 'b', 'c', 'd', 'e',
		'f', 'g', 'h', 'i', 'j',
		'k', 'l', 'm', 'n', 'o',
		0 };

	alice.Encrypt( test, 6 );
	bob.Decrypt( test, 3 );
	bob.Decrypt( test + 3, 3 );

	std::cout << "\n\n" << test << std::endl;

	_getch();

	return 0;
}