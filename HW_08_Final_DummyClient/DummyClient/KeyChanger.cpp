#include "stdafx.h"
#include "KeyChanger.h"


// The key size, in bits.
#define DHKEYSIZE 512

// Prime in little-endian format.
static const BYTE g_rgbPrime[] =
{
	0x91, 0x02, 0xc8, 0x31, 0xee, 0x36, 0x07, 0xec,
	0xc2, 0x24, 0x37, 0xf8, 0xfb, 0x3d, 0x69, 0x49,
	0xac, 0x7a, 0xab, 0x32, 0xac, 0xad, 0xe9, 0xc2,
	0xaf, 0x0e, 0x21, 0xb7, 0xc5, 0x2f, 0x76, 0xd0,
	0xe5, 0x82, 0x78, 0x0d, 0x4f, 0x32, 0xb8, 0xcb,
	0xf7, 0x0c, 0x8d, 0xfb, 0x3a, 0xd8, 0xc0, 0xea,
	0xcb, 0x69, 0x68, 0xb0, 0x9b, 0x75, 0x25, 0x3d,
	0xaa, 0x76, 0x22, 0x49, 0x94, 0xa4, 0xf2, 0x8d
};

// Generator in little-endian format.
static BYTE g_rgbGenerator[] =
{
	0x02, 0x88, 0xd7, 0xe6, 0x53, 0xaf, 0x72, 0xc5,
	0x8c, 0x08, 0x4b, 0x46, 0x6f, 0x9f, 0x2e, 0xc4,
	0x9c, 0x5c, 0x92, 0x21, 0x95, 0xb7, 0xe5, 0x58,
	0xbf, 0xba, 0x24, 0xfa, 0xe5, 0x9d, 0xcb, 0x71,
	0x2e, 0x2c, 0xce, 0x99, 0xf3, 0x10, 0xff, 0x3b,
	0xcb, 0xef, 0x6c, 0x95, 0x22, 0x55, 0x9d, 0x29,
	0x00, 0xb5, 0x4c, 0x5b, 0xa5, 0x63, 0x31, 0x41,
	0x13, 0x0a, 0xea, 0x39, 0x78, 0x02, 0x6d, 0x62
};

KeyChanger::KeyChanger()
{

	//==========================================
	//Construct data BLOBs for the prime and generator. The P and G
	//values, represented by the g_rgbPrime and g_rgbGenerator arrays
	//respectively, are shared values that have been agreed to by both
	//parties.
	//==========================================

	P.cbData = DHKEYSIZE / 8;
	P.pbData = (BYTE*)( g_rgbPrime );

	G.cbData = DHKEYSIZE / 8;
	G.pbData = (BYTE*)( g_rgbGenerator );
}


KeyChanger::~KeyChanger()
{
}

bool KeyChanger::GenerateKey( KeyPrivateSets* keyPrivateSets, KeySendingSets* keySendingSets )
{


	HCRYPTPROV hProvParty1 = keyPrivateSets->hProvParty;
	HCRYPTKEY hPrivateKey1 = keyPrivateSets->hPrivateKey;
	DWORD dwDataLen1 = keySendingSets->dwDataLen;
	PBYTE pbKeyBlob1 = keySendingSets->pbKeyBlob;


	BOOL fReturn;
	//==========================================
	//Create the private Diffie-Hellman key for party 1.
	//==========================================

	// Acquire a provider handle for party 1.
	fReturn = CryptAcquireContext(
		&hProvParty1,
		NULL,
		MS_ENH_DSS_DH_PROV,
		PROV_DSS_DH,
		CRYPT_VERIFYCONTEXT );
	if ( !fReturn )
	{
		return false;
	}

	// Create an ephemeral private key for party 1.
	fReturn = CryptGenKey(
		hProvParty1,
		CALG_DH_EPHEM,
		DHKEYSIZE << 16 | CRYPT_EXPORTABLE | CRYPT_PREGEN,
		&hPrivateKey1 );
	if ( !fReturn )
	{
		return false;
	}

	// Set the prime for party 1's private key.
	fReturn = CryptSetKeyParam(
		hPrivateKey1,
		KP_P,
		(PBYTE)&P,
		0 );
	if ( !fReturn )
	{
		return false;
	}

	// Set the generator for party 1's private key.
	fReturn = CryptSetKeyParam(
		hPrivateKey1,
		KP_G,
		(PBYTE)&G,
		0 );
	if ( !fReturn )
	{
		return false;
	}

	// Generate the secret values for party 1's private key.
	fReturn = CryptSetKeyParam(
		hPrivateKey1,
		KP_X,
		NULL,
		0 );
	if ( !fReturn )
	{
		return false;
	}








	// Get the size for the key BLOB.
	fReturn = CryptExportKey(
		hPrivateKey1,
		NULL,
		PUBLICKEYBLOB,
		0,
		NULL,
		&dwDataLen1 );
	if ( !fReturn )
	{
		return false;
	}



	// Allocate the memory for the key BLOB.
	if ( !( pbKeyBlob1 = (PBYTE)malloc( dwDataLen1 ) ) )
	{
		return false;
	}

	// Get the key BLOB.
	fReturn = CryptExportKey(
		hPrivateKey1,
		0,
		PUBLICKEYBLOB,
		0,
		pbKeyBlob1,
		&dwDataLen1 );
	if ( !fReturn )
	{
		return false;
	}



	keyPrivateSets->hProvParty = hProvParty1;
	keyPrivateSets->hPrivateKey = hPrivateKey1;
	keySendingSets->dwDataLen = dwDataLen1;
	keySendingSets->pbKeyBlob = pbKeyBlob1;


	return true;

}


bool KeyChanger::GetSessionKey( KeyPrivateSets* myPrivateSets, KeySendingSets* othersSendingSets )
{

	HCRYPTPROV hProvParty1 = myPrivateSets->hProvParty;
	HCRYPTKEY hPrivateKey1 = myPrivateSets->hPrivateKey;
	PBYTE pbKeyBlob2 = othersSendingSets->pbKeyBlob;
	DWORD dwDataLen2 = othersSendingSets->dwDataLen;
	HCRYPTKEY hSessionKey2 = myPrivateSets->hSessionKey;

	//==========================================
	//Party 1 imports party 2's public key.
	//The imported key will contain the new shared secret
	//key (Y^X) mod P.
	//==========================================
	BOOL fReturn;
	fReturn = CryptImportKey(
		hProvParty1,
		pbKeyBlob2,
		dwDataLen2,
		hPrivateKey1,
		0,
		&hSessionKey2 );
	if ( !fReturn )
	{
		return false;
	}

	myPrivateSets->hProvParty = hProvParty1;
	myPrivateSets->hPrivateKey = hPrivateKey1;
	othersSendingSets->pbKeyBlob = pbKeyBlob2;
	othersSendingSets->dwDataLen = dwDataLen2;
	myPrivateSets->hSessionKey = hSessionKey2;

	return true;
}

bool KeyChanger::EncryptData( HCRYPTKEY sessionKey, BYTE* originalData, int originalSize, BYTE* encryptedData )
{

	//============================
	//Convert the agreed keys to symmetric keys. They are currently of
	//the form CALG_AGREEDKEY_ANY. Convert them to CALG_RC4.
	//============================

	BOOL fReturn;
	ALG_ID Algid = CALG_RC4;

	// Enable the party 1 public session key for use by setting the 
	// ALGID.
	fReturn = CryptSetKeyParam(
		sessionKey,
		KP_ALGID,
		(PBYTE)&Algid,
		0 );
	if ( !fReturn )
	{
		return false;
	}





	/************************
	Encrypt some data with party 1's session key.
	************************/
	// Get the size.
	DWORD dwLength = originalSize;
	fReturn = CryptEncrypt(
		sessionKey,
		0,
		TRUE,
		0,
		NULL,
		&dwLength,
		originalSize );
	if ( !fReturn )
	{
		return false;
	}

	// Allocate a buffer to hold the encrypted data.
	//pbData = (PBYTE)malloc( dwLength );
	if ( !encryptedData )
	{
		return false;
	}

	// Copy the unencrypted data to the buffer. The data will be 
	// encrypted in place.
	memcpy( encryptedData, originalData, originalSize );

	// Encrypt the data.
	dwLength = originalSize;
	fReturn = CryptEncrypt(
		sessionKey,
		0,
		TRUE,
		0,
		encryptedData,
		&dwLength,
		originalSize );
	if ( !fReturn )
	{
		return false;
	}

	return true;
}

bool KeyChanger::DecryptData( HCRYPTKEY sessionKey, BYTE* encryptedData, int originalSize )
{
	//============================
	//Decrypt the data with party 2's session key.
	//============================

	BOOL fReturn;
	ALG_ID Algid = CALG_RC4;

	fReturn = CryptSetKeyParam(
		sessionKey,
		KP_ALGID,
		(PBYTE)&Algid,
		0 );
	if ( !fReturn )
	{
		return false;
	}


	DWORD dwLength = originalSize;
	fReturn = CryptDecrypt(
		sessionKey,
		0,
		TRUE,
		0,
		encryptedData,
		&dwLength );
	if ( !fReturn )
	{
		return false;
	}

	return true;
}
