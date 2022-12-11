#pragma once
class CCheckNetwork
{
public:
	CCheckNetwork();
	~CCheckNetwork();

public:
	//현재 네트워크 상태를 체크한다.
	int CheckNetworkStatusByMac(char *pszGuid, int & nRetCode);

	//현재 네트워크 상태를 체크한다.
	int CheckNetworkStatusByAll(ArrNicResult &arrNicResult);

private:

	//윈도우 네트워크 상태 값을 가져온다.
	BOOL CheckInternetConnect();

	//NIC 정보를 가져온다.
	BOOL GetNICInfo(ArrNicInfo &arrNicInfo);

	//NIC의 상세정보 가져온다.(ip,gate,mask)
	BOOL GetNicDetailInfo(PNIC_INFO pInfo);

	//다른 외부 DNS 체크
	BOOL CheckOtherByDNS(PNIC_INFO pInfo);

	//현재 설정된 DNS 통신을 체크한다.
	BOOL CheckCurrentDns();

	//DNS 설정
	BOOL SetCurrentDns(char* pszDNSAddr, PNIC_INFO pInfo);

	//DNS 원복
	BOOL RecoveryDns(PNIC_INFO pInfo);

	//Ping을 보내본다.
	int SendPing(char* pszIPAddr, int nPacketAmount);

	char* ConvertWCtoC(WCHAR* str);
};

