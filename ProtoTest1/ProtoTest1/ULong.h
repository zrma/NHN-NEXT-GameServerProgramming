
#ifndef __ULONG_H__
#define __ULONG_H__

#include "SystemIncludes.h"

namespace DiffieHellmanLib
{
	template<size_t Dimension> class ULong
	{
	public:
		ULong()
		{
			memset( m_InternalBuffer, 0, Dimension );
		}

		ULong( const unsigned long number )
		{
			memcpy( m_InternalBuffer, (char*)&number, sizeof( unsigned long ) );
			memset( m_InternalBuffer + sizeof( unsigned long ), 0, Dimension - sizeof( unsigned long ) );
		}

		ULong( const char* binary, size_t binarySize )
		{
			if ( binarySize <= Dimension )
			{
				memcpy( m_InternalBuffer, binary, binarySize );
				memset( m_InternalBuffer + binarySize, 0, Dimension - binarySize );
			}
			else
			{
				memset( m_InternalBuffer, 0, Dimension );
			}
		}

		ULong( const ULong<Dimension>& number )
		{
			memcpy( m_InternalBuffer, number.m_InternalBuffer, Dimension );
		}

		ULong<Dimension>& operator= ( const ULong<Dimension>& number )
		{
			memcpy( m_InternalBuffer, number.m_InternalBuffer, Dimension );
			return *this;
		}

		bool operator == ( const ULong<Dimension>& number ) const
		{
			return ( memcmp( m_InternalBuffer, number.m_InternalBuffer, Dimension ) == 0 );
		}

		bool operator != ( const ULong<Dimension>& number ) const
		{
			return !( *this == number );
		}

		bool operator > ( const ULong<Dimension>& number ) const
		{
			for ( int i = Dimension - 1; i >= 0; --i )
			{
				for ( int j = 1; j >= 0; --j )
				{
					char currentFirstChar = ( (unsigned char)( m_InternalBuffer[i] << 4 * ( 1 - j ) ) ) >> 4;
					char currentSecondChar = ( (unsigned char)( number.m_InternalBuffer[i] << 4 * ( 1 - j ) ) ) >> 4;

					if ( currentFirstChar == currentSecondChar ) continue;
					else return ( currentFirstChar > currentSecondChar );
				}
			}
			return false;
		}

		bool operator >= ( const ULong<Dimension>& number ) const
		{
			return ( *this > number ) || ( *this == number );
		}

		bool operator < ( const ULong<Dimension>& number ) const
		{
			return !( *this >= number );
		}

		bool operator <= ( const ULong<Dimension>& number ) const
		{
			return !( *this > number );
		}

		ULong<Dimension> operator+ ( const ULong<Dimension>& number ) const
		{
			ULong<Dimension> result;

			bool moveBase = false;
			for ( int i = 0; i < Dimension; ++i )
			{
				for ( int j = 0; j <= 1; ++j )
				{
					char currentFirstChar = ( (unsigned char)( m_InternalBuffer[i] << 4 * ( 1 - j ) ) ) >> 4;
					char currentSecondChar = ( (unsigned char)( number.m_InternalBuffer[i] << 4 * ( 1 - j ) ) ) >> 4;

					if ( moveBase )
					{
						currentFirstChar += 1;
					}
					if ( currentFirstChar + currentSecondChar >= 16 )
					{
						result.m_InternalBuffer[i] += ( currentFirstChar + currentSecondChar - 16 ) << j * 4;
						moveBase = true;
					}
					else
					{
						result.m_InternalBuffer[i] += ( currentFirstChar + currentSecondChar ) << j * 4;
						moveBase = false;
					}
				}
			}

			return result;
		}

		ULong<Dimension>& operator+= ( const ULong<Dimension>& number )
		{
			*this = *this + number;
			return *this;
		}

		ULong<Dimension>& operator++ ( )
		{
			*this += 1;
			return *this;
		}

		ULong<Dimension> operator++ ( int )
		{
			ULong<Dimension> tmp = *this;
			*this += 1;
			return tmp;
		}

		ULong<Dimension> operator- ( const ULong<Dimension>& number ) const
		{
			ULong<Dimension> result;

			bool moveBase = false;
			for ( int i = 0; i < Dimension; ++i )
			{
				for ( int j = 0; j <= 1; ++j )
				{
					char currentFirstChar = ( (unsigned char)( m_InternalBuffer[i] << 4 * ( 1 - j ) ) ) >> 4;
					char currentSecondChar = ( (unsigned char)( number.m_InternalBuffer[i] << 4 * ( 1 - j ) ) ) >> 4;

					if ( moveBase )
					{
						currentFirstChar -= 1;
					}
					if ( currentFirstChar >= currentSecondChar )
					{
						result.m_InternalBuffer[i] += ( currentFirstChar - currentSecondChar ) << j * 4;
						moveBase = false;
					}
					else
					{
						result.m_InternalBuffer[i] += ( currentFirstChar + 16 - currentSecondChar ) << j * 4;
						moveBase = true;
					}
				}
			}

			return result;
		}

		ULong<Dimension>& operator-= ( const ULong<Dimension>& number )
		{
			*this = *this - number;
			return *this;
		}

		ULong<Dimension>& operator-- ( )
		{
			*this -= 1;
			return *this;
		}

		ULong<Dimension> operator-- ( int )
		{
			ULong<Dimension> tmp = *this;
			*this -= 1;
			return tmp;
		}

		ULong<Dimension> operator* ( const ULong<Dimension>& number ) const
		{
			ULong<Dimension> counter = number;
			ULong<Dimension> result;

			size_t capacity = counter.GetDigitCapacity();

			while ( counter != 0 )
			{
				ULong<Dimension> tmp = *this;
				tmp.MultiplyBy16InPowDigits( capacity - 1 );
				result += tmp;

				ULong<Dimension> diff = 1;
				diff.MultiplyBy16InPowDigits( capacity - 1 );
				counter -= diff;

				capacity = counter.GetDigitCapacity();
			}

			return result;
		}

		ULong<Dimension>& operator*= ( const ULong<Dimension>& number )
		{
			*this = *this * number;
			return *this;
		}

		ULong<Dimension> operator/ ( const ULong<Dimension>& divider ) const
		{
			ULong<Dimension> quotient;
			ULong<Dimension> remainder;

			DivideInternal( divider, quotient, remainder );

			return quotient;
		}

		ULong<Dimension>& operator/= ( const ULong<Dimension>& number )
		{
			*this = *this / number;
			return *this;
		}

		ULong<Dimension> operator% ( const ULong<Dimension>& divider ) const
		{
			ULong<Dimension> quotient;
			ULong<Dimension> remainder;

			DivideInternal( divider, quotient, remainder );

			return remainder;
		}

		ULong<Dimension>& operator%= ( const ULong<Dimension>& number )
		{
			*this = *this % number;
			return *this;
		}

		void GetBinary( std::vector<char>& binary ) const
		{
			binary.assign( m_InternalBuffer, m_InternalBuffer + Dimension );
		}

		void CopyBinary( char * pData, size_t size ) const
		{
			memcpy( pData, m_InternalBuffer, size );
		}

		bool IsEvenNumber() const
		{
			char lastDigit = (unsigned char)( m_InternalBuffer[0] << 4 ) >> 4;
			if ( ( lastDigit != 0 ) && ( lastDigit % 2 != 0 ) )
			{
				return false;
			}

			return true;
		}

		void MultiplyAmodB( const ULong<Dimension>& a, const ULong<Dimension>& b )
		{
			if ( GetDigitCapacity() + a.GetDigitCapacity() <= Dimension )
			{
				*this = ( *this * a ) % b;
			}
			else
			{
				ULong<Dimension * 2> extThis( m_InternalBuffer, Dimension );
				ULong<Dimension * 2> extA( a.m_InternalBuffer, Dimension );
				ULong<Dimension * 2> extB( b.m_InternalBuffer, Dimension );
				ULong<Dimension * 2> extResult = ( extThis * extA ) % extB;

				extResult.CopyBinary( m_InternalBuffer, Dimension );
			}
		}

	private:
		size_t GetDigitCapacity() const
		{
			size_t result = Dimension * 2;
			for ( int i = Dimension - 1; i >= 0; --i )
			{
				char firstPart = m_InternalBuffer[i] >> 4;
				char secondPart = (unsigned char)( m_InternalBuffer[i] << 4 ) >> 4;
				if ( secondPart == 0 && firstPart == 0 )
				{
					result -= 2;
				}
				else if ( firstPart == 0 && secondPart != 0 )
				{
					--result;
					return result;
				}
				else
				{
					return result;
				}
			}
			return result;
		}

		void MultiplyBy16InPowDigits( size_t digits )
		{
			ULong<Dimension> tmpBuffContainer( m_InternalBuffer, Dimension );
			char( &tmpBuff )[Dimension] = tmpBuffContainer.m_InternalBuffer;

			memset( m_InternalBuffer, 0, Dimension );

			size_t nonParity = digits % 2;
			for ( int i = digits / 2; i < Dimension; ++i )
			{
				for ( size_t j = 0; j <= 1; ++j )
				{
					char current = ( (unsigned char)( tmpBuff[( Dimension - 1 ) - i] << 4 * ( 1 - j ) ) ) >> 4;

					char result = current << ( nonParity *( 1 - j ) * 4 + ( 1 - nonParity ) * j * 4 );

					int index = i - ( digits / 2 ) - nonParity * j;
					if ( index >= 0 )
					{
						m_InternalBuffer[( Dimension - 1 ) - index] += result;
					}
				}
			}
		}

		void DivideInternal( const ULong<Dimension>& divider, ULong<Dimension>& quotient, ULong<Dimension>& remainder ) const
		{
			ULong<Dimension> dividerClone = divider;
			remainder = *this;
			quotient = 0;

			size_t capacityRemainder = remainder.GetDigitCapacity();
			size_t capacityDivider = divider.GetDigitCapacity();

			while ( capacityRemainder > capacityDivider )
			{
				dividerClone = divider;
				dividerClone.MultiplyBy16InPowDigits( capacityRemainder - capacityDivider - 1 );

				ULong<Dimension> q = 1;
				q.MultiplyBy16InPowDigits( capacityRemainder - capacityDivider - 1 );

				ULong<Dimension> tmp = dividerClone;
				tmp.MultiplyBy16InPowDigits( 1 );

				if ( remainder >= tmp )
				{
					dividerClone = tmp;
					q.MultiplyBy16InPowDigits( 1 );
				}
				while ( remainder >= dividerClone )
				{
					remainder -= dividerClone;
					quotient += q;
				}

				capacityRemainder = remainder.GetDigitCapacity();
			}

			while ( remainder >= divider )
			{
				remainder -= divider;
				++quotient;
			}
		}

	private:
		char m_InternalBuffer[Dimension];
	};
}

#endif // __ULONG_H__
