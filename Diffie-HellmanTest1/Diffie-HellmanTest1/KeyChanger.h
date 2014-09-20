#pragma once

#include <tchar.h>
#include <windows.h>
#include <wincrypt.h>
#pragma comment(lib, "crypt32.lib")

struct KeyPrivateSets
{
	HCRYPTPROV hProvParty = NULL;
	HCRYPTKEY hPrivateKey = NULL;
	HCRYPTKEY hSessionKey = NULL;
};
struct KeySendingSets
{
	DWORD dwDataLen = -1;
	PBYTE pbKeyBlob = NULL;
};

class KeyChanger
{
public:
	KeyChanger();
	~KeyChanger();

	bool GenerateKey( KeyPrivateSets* keyPrivateSets, KeySendingSets* keySendingSets );

	bool GetSessionKey( KeyPrivateSets* myPrivateSets, KeySendingSets* othersSendingSets );

	bool EncryptData( HCRYPTKEY sessionKey, BYTE* originalData, int originalSize, BYTE* encryptedData );
	bool DecryptData( HCRYPTKEY sessionKey, BYTE* encryptedData, int originalSize );

	DATA_BLOB P;
	DATA_BLOB G;

};

