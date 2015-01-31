#pragma once

#include <vector>
#include "KeyExchanger.h"

// Module P (The bytes representation of number 0x24357685944363427687549776543601)
const size_t PModuleLength = 32;

unsigned char buffer[PModuleLength] =
{ 0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5,
0xAE, 0x9F, 0x24, 0x11, 0x7C, 0x4B, 0x1F, 0xE6,
0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

std::vector<char> cryptoPModule( (char*)buffer, (char*)buffer + PModuleLength );

// Module G
unsigned long cryptoGModule = 0x02;

class Crypter
{
public:
	Crypter() : m_KeyExchanger( cryptoPModule, cryptoGModule ) {}
	~Crypter() {}

	void PrintExchangeKey() { _PrintBuffer( m_ExchangeKey ); }
	void PrintSharedKey() { _PrintBuffer( m_SharedKey ); }

	void GenerateExchangeKey() { m_KeyExchanger.GenerateExchangeData( m_ExchangeKey ); }
	void CreateSharedKey( std::vector<char>& receiveKey )
	{
		m_KeyExchanger.CompleteExchangeData( receiveKey, m_SharedKey ); 
		m_KeySize = m_SharedKey.size();
	}

	std::vector<char>& GetExchangeKey() { return m_ExchangeKey; }
	std::vector<char>& GetSharedKey() { return m_SharedKey; }

	bool Encrypt( char* originalData, int dataSize )
	{
		if ( dataSize == 0 || m_KeySize == 0 )
		{
			return false;
		}

		for ( int i = 0; i < dataSize; ++i )
		{
			*( originalData + i ) += m_SharedKey[m_EncryptCount];
			m_EncryptCount = ( ++m_EncryptCount % m_KeySize );
		}

		return true;
	}

	bool Decrypt( char* encryptedData, int dataSize )
	{
		if ( dataSize == 0 || m_KeySize == 0 )
		{
			return false;
		}

		for ( int i = 0; i < dataSize; ++i )
		{
			*( encryptedData + i ) -= m_SharedKey[m_DecryptCount];
			m_DecryptCount = ( ++m_DecryptCount % m_KeySize );
		}

		return true;
	}

private:
	void _PrintBuffer( const std::vector<char>& buffer )
	{
		for ( size_t i = 0; i < buffer.size(); ++i )
		{
			std::cout << "0x" << std::setbase( 16 ) << std::setw( 2 ) << std::setfill( '0' ) << (int)(unsigned char)buffer[i] << " ";

			if ( ( i + 1 ) % 8 == 0 )
			{
				std::cout << std::endl;
			}
		}
	}
	
private:
	// Keys exchanger
	DiffieHellmanLib::DiffieHellmanKeysExchanger<PModuleLength> m_KeyExchanger;

	std::vector<char> m_ExchangeKey;
	std::vector<char> m_SharedKey;

	int m_KeySize = 0;
	int m_EncryptCount = 0;
	int m_DecryptCount = 0;
};

