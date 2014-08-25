#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "DBHelper.h"

//todo: DbHelper의 static 멤버변수 초기화

SQLHENV DbHelper::mSqlHenv = nullptr;
SQL_CONN* DbHelper::mSqlConnPool = nullptr; ///< 워커스레드수만큼
int DbHelper::mDbWorkerThreadCount = NULL;


DbHelper::DbHelper()
{
	CRASH_ASSERT(mSqlConnPool[LWorkerThreadId].mUsingNow == false);

	mCurrentSqlHstmt = mSqlConnPool[LWorkerThreadId].mSqlHstmt;
	mCurrentResultCol = 1;
	mCurrentBindParam = 1;
	CRASH_ASSERT(mCurrentSqlHstmt != nullptr);

	mSqlConnPool[LWorkerThreadId].mUsingNow = true;
}

DbHelper::~DbHelper()
{
	//todo: SQLFreeStmt를 이용하여 현재 SQLHSTMT 해제(unbind, 파라미터리셋, close 순서로)
	//순서의 이유를 찾다가 보니 ms와 좀 다르던데... 왜죠
	///# CLOSE를 먼저 해도 됨. 지켜줘야 하는 것은 unbind 다음에 reset_param하면 됨

	//http://msdn.microsoft.com/en-us/library/ms709284(v=vs.85).aspx
	SQLFreeStmt( mCurrentSqlHstmt, SQL_UNBIND );
	SQLFreeStmt( mCurrentSqlHstmt, SQL_RESET_PARAMS );
	SQLFreeStmt( mCurrentSqlHstmt, SQL_CLOSE );
	
	mSqlConnPool[LWorkerThreadId].mUsingNow = false;
}

bool DbHelper::Initialize(const wchar_t* connInfoStr, int workerThreadCount)
{


	//todo: mSqlConnPool, mDbWorkerThreadCount를 워커스레스 수에 맞추어 초기화
	mDbWorkerThreadCount = workerThreadCount;
	mSqlConnPool = new SQL_CONN[mDbWorkerThreadCount];

	// 	SQLRETURN ret = SQL_ERROR;
	// 
	// 	for ( int i = 0; i < mDbWorkerThreadCount; ++i )
	// 	{
	// 		ret = SQLConnect( mSqlConnPool[i].mSqlHdbc, NULL, (SQLWCHAR*)connInfoStr, SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT );
	// 	}


	if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &mSqlHenv))
	{
		printf_s("DbHelper Initialize SQLAllocHandle failed\n");
		return false;
	}

	if (SQL_SUCCESS != SQLSetEnvAttr(mSqlHenv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, SQL_IS_INTEGER))
	{
		printf_s("DbHelper Initialize SQLSetEnvAttr failed\n");
		return false;
	}


	/// 스레드별로 SQL connection을 풀링하는 방식. 즉, 스레드마다 SQL서버로의 연결을 갖는다.
	for (int i = 0; i < mDbWorkerThreadCount; ++i)
	{
		//todo: SQLAllocHandle을 이용하여 SQL_CONN의 mSqlHdbc 핸들 사용가능하도록 처리
		///# 왜 에러처리 안함? SQLAllocHandle( SQL_HANDLE_DBC, mSqlHenv, &mSqlConnPool[i].mSqlHdbc );

		
		if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_DBC, mSqlHenv, &mSqlConnPool[i].mSqlHdbc))
		{
			printf_s("DbHelper Initialize SQLAllocHandle failed\n");
			return false;
		}
		
		//이거 어디서 쓰지?
		//http://msdn.microsoft.com/en-us/library/ms715433(v=vs.85).aspx
		//예제코드 참고
		SQLSMALLINT resultLen = 0;
		
		//todo: SQLDriverConnect를 이용하여 SQL서버에 연결하고 그 핸들을 SQL_CONN의 mSqlHdbc에 할당
		//마지막 인자를 PROMPT로 하면 a dialog box prompts the user to specify whether the file should be overwritten
		SQLRETURN ret = SQLDriverConnect( mSqlConnPool[i].mSqlHdbc, NULL, (SQLWCHAR*)connInfoStr, SQL_NTS, NULL, 0, &resultLen, SQL_DRIVER_NOPROMPT );

		if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
		{
			SQLWCHAR sqlState[1024] = { 0, } ;
			SQLINTEGER nativeError = 0;
			SQLWCHAR msgText[1024] = { 0, } ;
			SQLSMALLINT textLen = 0 ;

			SQLGetDiagRec(SQL_HANDLE_DBC, mSqlConnPool[i].mSqlHdbc, 1, sqlState, &nativeError, msgText, 1024, &textLen);

			wprintf_s(L"DbHelper Initialize SQLDriverConnect failed: %s \n", msgText);

			return false;
		}

		//todo: SQLAllocHandle를 이용하여 SQL_CONN의 mSqlHstmt 핸들 사용가능하도록 처리
		///# 에러처리... SQLAllocHandle(SQL_HANDLE_STMT, mSqlConnPool[i].mSqlHdbc, &mSqlConnPool[i].mSqlHstmt);

		if (SQL_SUCCESS != SQLAllocHandle(SQL_HANDLE_STMT, mSqlConnPool[i].mSqlHdbc, &mSqlConnPool[i].mSqlHstmt))
		{
			printf_s("DbHelper Initialize SQLAllocHandle SQL_HANDLE_STMT failed\n");
			return false;
		}

	
	}

	return true;
}


void DbHelper::Finalize()
{
	for (int i = 0; i < mDbWorkerThreadCount; ++i)
	{
		SQL_CONN* currConn = &mSqlConnPool[i];
		if (currConn->mSqlHstmt)
			SQLFreeHandle(SQL_HANDLE_STMT, currConn->mSqlHstmt);

		if (currConn->mSqlHdbc)
			SQLFreeHandle(SQL_HANDLE_DBC, currConn->mSqlHdbc);
	}

	delete[] mSqlConnPool;


}

bool DbHelper::Execute(const wchar_t* sqlstmt)
{
	//todo: mCurrentSqlHstmt핸들 사용하여 sqlstmt를 수행.  
	SQLRETURN ret = SQLExecDirect( mCurrentSqlHstmt, (SQLWCHAR*)sqlstmt, SQL_NTS ); //= SQLExecDirect(...);
	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::FetchRow()
{
	//todo: mCurrentSqlHstmt가 들고 있는 내용 fetch
	//http://wolfsi.egloos.com/2301972
	//다음 행으로 커서를 진행시키고 바인드된 열을 가지고온다
	SQLRETURN ret = SQLFetch( mCurrentSqlHstmt );

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		if (SQL_NO_DATA != ret)
		{
			PrintSqlStmtError();
		}
		
		return false;
	}

	return true;
}



bool DbHelper::BindParamInt(int* param)
{
	//todo: int형 파라미터 바인딩
	//http://wolfsi.egloos.com/10205557
	//두번째 매개변수는 1부터 시작이므로 후위연산
	//precision 참고
	//http://publib.boulder.ibm.com/infocenter/idshelp/v111/index.jsp?topic=/com.ibm.odbc.doc/sii-04-26325.htm
	SQLRETURN ret =SQLBindParameter(mCurrentSqlHstmt,mCurrentBindParam++,SQL_PARAM_INPUT, 
									 SQL_C_LONG, SQL_INTEGER, 10, 0, param, 0, NULL ); // = SQLBindParameter(...);
	///# 굿! 유일하게 제대로 한 팀.

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::BindParamFloat(float* param)
{
	SQLRETURN ret = SQLBindParameter(mCurrentSqlHstmt, mCurrentBindParam++, SQL_PARAM_INPUT,
		SQL_C_FLOAT, SQL_REAL, 7, 0, param, 0, NULL);

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::BindParamBool(bool* param)
{
	//todo: bool형 파라미터 바인딩
	//http://msdn.microsoft.com/en-us/library/ms714556(v=vs.85).aspx
	SQLRETURN ret = SQLBindParameter(mCurrentSqlHstmt, mCurrentBindParam++, SQL_PARAM_INPUT,
									  SQL_C_TINYINT, SQL_TINYINT, 1,0, param, 0, NULL); // = SQLBindParameter(...);

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}

bool DbHelper::BindParamText(const wchar_t* text)
{

	//todo: 유니코드 문자열 바인딩
	//SQLLEN len = SQL_NTS;

	SQLRETURN ret = SQLBindParameter( mCurrentSqlHstmt, mCurrentBindParam++, SQL_PARAM_INPUT,
									  SQL_C_WCHAR, SQL_WVARCHAR, wcslen(text), 0, (SQLPOINTER)text, 0, NULL );; // = SQLBindParameter(...);

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
		return false;
	}

	return true;
}


void DbHelper::BindResultColumnInt(int* r)
{
	SQLLEN len = 0;
	SQLRETURN ret = SQLBindCol(mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_LONG, r, 4, &len);

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}
void DbHelper::BindResultColumnFloat(float* r)
{
	SQLLEN len = 0;
	//todo: float형 결과 컬럼 바인딩
	SQLRETURN ret = SQLBindCol( mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_FLOAT, r, 7, &len ); ///# 왜 7?? 버퍼 크기를 넣어줘야 함

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}

void DbHelper::BindResultColumnBool(bool* r)
{
	SQLLEN len = 0;
	SQLRETURN ret = SQLBindCol(mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_TINYINT, r, 1, &len);

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}
void DbHelper::BindResultColumnText(wchar_t* text, size_t count)
{
	SQLLEN len = 0;
	//todo: wchar_t*형 결과 컬럼 바인딩
	SQLRETURN ret = SQLBindCol( mCurrentSqlHstmt, mCurrentResultCol++, SQL_C_WCHAR, text, count, &len ); ///# 버퍼 크기를 넣어줘야지... count*2

	if (SQL_SUCCESS != ret && SQL_SUCCESS_WITH_INFO != ret)
	{
		PrintSqlStmtError();
	}
}


void DbHelper::PrintSqlStmtError()
{
	SQLWCHAR sqlState[1024] = { 0, };
	SQLINTEGER nativeError = 0;
	SQLWCHAR msgText[1024] = { 0, };
	SQLSMALLINT textLen = 0;

	SQLGetDiagRec(SQL_HANDLE_STMT, mCurrentSqlHstmt, 1, sqlState, &nativeError, msgText, 1024, &textLen);

	wprintf_s(L"DbHelper SQL Statement Error: %ls \n", msgText);
}