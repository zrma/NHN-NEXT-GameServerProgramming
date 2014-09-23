// Diffie-HellmanTest1.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include "KeyChanger.h"



int _tmain( int argc, _TCHAR* argv[] )
{
	KeyChanger keyChanger1, keyChanger2;
	
	KeyPrivateSets bobPrivateKeySets;
	KeyPrivateSets alicePrivateKeySets;
	KeySendingSets bobSendingKeySets;
	KeySendingSets aliceSendingKeySets;

	printf_s( "" );
	keyChanger1.GenerateKey( &bobPrivateKeySets, &bobSendingKeySets );
	keyChanger2.GenerateKey( &alicePrivateKeySets, &aliceSendingKeySets );
	printf_s( "" );
	keyChanger1.GetSessionKey( &bobPrivateKeySets, &aliceSendingKeySets );
	keyChanger2.GetSessionKey( &alicePrivateKeySets, &bobSendingKeySets );
	printf_s( "" );

	BYTE oriData[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 };
	BYTE temp[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	printf_s( "Original : %s (%d) \n", oriData, sizeof( oriData ) );
	keyChanger1.EncryptData( bobPrivateKeySets.hSessionKey, oriData, sizeof( oriData ), temp );
	printf_s( "Encrypte : %s (%d) \n", temp, sizeof( temp ) );
	keyChanger2.DecryptData( alicePrivateKeySets.hSessionKey, temp, sizeof( oriData ) );
	printf_s( "Decrypte : %s (%d) \n", temp, sizeof(temp) );

	bool flag = false;

	for ( int i = 0; i < sizeof( temp ); ++i )
	{
		if ( oriData[i] != temp[i] )
		{
			flag = true;
			break;
		}
	}

	if ( flag )
		printf_s( "Diff! \n" );
	else
		printf_s( "Same! \n" );
	
	getchar();

	return 0;
}




/*



// Diffie-HellmanTest1.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <time.h>
#include <stdlib.h>
#include "Elise.h"
#include "Bob.h"




int _tmain(int argc, _TCHAR* argv[])
{
srand( (unsigned int)time( NULL ) );

// phase 1 set
Elise* elise = new Elise();
Bob* bob = new Bob();

elise->Generate_a();
bob->Generate_b();

// phase 2 get set AB

long double A = elise->GetA();
long double B = bob->GetB();

bob->SetA( A );
elise->SetB( B );

long double bob_s = bob->Get_s();
long double elise_s = elise->Get_s();

printf_s( "bob_s:%f elise_s:%f \n", bob_s, elise_s );

getchar();

return 0;
}



*/