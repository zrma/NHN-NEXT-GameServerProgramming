#pragma once

//todo: 아래 설정은 여러분들의 상황에 맞게 고치기

#define LISTEN_PORT		9000
#define MAX_CONNECTION	10000

#define CONNECT_SERVER_ADDR	"127.0.0.1"
#define CONNECT_SERVER_PORT 9001

///# 실제로 IP로 접근하는 것이 좋다. 실 서비스 환경에서 TRIZDREAMING의 컴퓨터 이름을 일일이 확인할 수가 없잖아 ㅎㅎ
#define SQL_SERVER_CONN_STR	L"Driver={SQL Server};Server=TRIZDREAMING-PC\\SQLEXPRESS;Database=GameDB;UID=triztest;PWD=next11223344;"

#define GQCS_TIMEOUT	10 //INFINITE


