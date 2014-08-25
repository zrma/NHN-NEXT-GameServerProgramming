#pragma once


#define LISTEN_PORT		9000
#define MAX_CONNECTION	10000

#define CONNECT_SERVER_ADDR	"127.0.0.1"
#define CONNECT_SERVER_PORT 9001

//todo: SQL 연결 스트링 여러분의 상황에 맞게 수정
//#define SQL_SERVER_CONN_STR	L"Driver={SQL Server};Server=127.0.0.1,1433\\TRIZDREAMING-PC;Database=GameDB;UID=triztest;PWD=next11223344;"

///# good!
#define SQL_SERVER_CONN_STR	L"Driver={SQL Server};Server=TRIZDREAMING-PC\\SQLEXPRESS;Database=GameDB;UID=triztest;PWD=next11223344;"

//#define SQL_SERVER_CONN_STR	L"Provider=SQLOLEDB.1;Password=next11223344;User ID=triztest;Initial Catalog=GameDB;Data Source=TRIZDREAMING-PC\\SQLEXPRESS"

#define GQCS_TIMEOUT	10 //INFINITE

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER
};

extern __declspec(thread) int LThreadType;