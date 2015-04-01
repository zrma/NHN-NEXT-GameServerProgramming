#include "stdafx.h"
#include "DHKeyExchanger.h"


DHKeyExchanger::DHKeyExchanger()
{
	Init();
}


DHKeyExchanger::~DHKeyExchanger()
{
	Release();
}

void DHKeyExchanger::Init()
{
	//////////////////////////////////////////////////////////////////////////
	// Construct data BLOBs for the prime and generator. The P and G
	// values, represented by the g_rgbPrime and g_rgbGenerator arrays
	// respectively, are shared values that have been agreed to by both
	// parties.
	//////////////////////////////////////////////////////////////////////////
	P.cbData = DHKEYSIZE / 8;
	P.pbData = (BYTE*)(g_rgbPrime);

	G.cbData = DHKEYSIZE / 8;
	G.pbData = (BYTE*)(g_rgbGenerator);
}

void DHKeyExchanger::Release()
{
	if ( pbData )
	{
		free( pbData );
		pbData = NULL;
	}

	if ( hSessionKey1 )
	{
		CryptDestroyKey( hSessionKey1 );
		hSessionKey1 = NULL;
	}

	if ( pbKeyBlob1 )
	{
		free( pbKeyBlob1 );
		pbKeyBlob1 = NULL;
	}

	if ( hPrivateKey1 )
	{
		CryptDestroyKey( hPrivateKey1 );
		hPrivateKey1 = NULL;
	}

	if ( hProvParty1 )
	{
		CryptReleaseContext( hProvParty1, 0 );
		hProvParty1 = NULL;
	}
}

BOOL DHKeyExchanger::CreatePrivateKey()
{
	BOOL fReturn = FALSE;

	do
	{
		/************************
		Create the private Diffie-Hellman key
		************************/
		// Acquire a provider handle
		fReturn = CryptAcquireContext( &hProvParty1, NULL, MS_ENH_DSS_DH_PROV, PROV_DSS_DH, CRYPT_VERIFYCONTEXT );
		if ( !fReturn )
		{
			break;
		}

		// Create an ephemeral private key
		fReturn = CryptGenKey( hProvParty1, CALG_DH_EPHEM, DHKEYSIZE << 16 | CRYPT_EXPORTABLE | CRYPT_PREGEN, &hPrivateKey1 );
		if ( !fReturn )
		{
			break;
		}

		// Set the prime for private key.
		fReturn = CryptSetKeyParam( hPrivateKey1, KP_P, (PBYTE)&P, 0 );
		if ( !fReturn )
		{
			break;
		}

		// Set the generator for private key.
		fReturn = CryptSetKeyParam( hPrivateKey1, KP_G, (PBYTE)&G, 0 );
		if ( !fReturn )
		{
			break;
		}

		// Generate the secret values for private key.
		fReturn = CryptSetKeyParam( hPrivateKey1, KP_X, NULL, 0 );
		if ( !fReturn )
		{
			break;
		}
	} while ( false );

	return fReturn;
}
