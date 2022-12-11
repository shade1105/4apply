#pragma once
class CCheckNetwork
{
public:
	CCheckNetwork();
	~CCheckNetwork();

public:
	//���� ��Ʈ��ũ ���¸� üũ�Ѵ�.
	int CheckNetworkStatusByMac(char *pszGuid, int & nRetCode);

	//���� ��Ʈ��ũ ���¸� üũ�Ѵ�.
	int CheckNetworkStatusByAll(ArrNicResult &arrNicResult);

private:

	//������ ��Ʈ��ũ ���� ���� �����´�.
	BOOL CheckInternetConnect();

	//NIC ������ �����´�.
	BOOL GetNICInfo(ArrNicInfo &arrNicInfo);

	//NIC�� ������ �����´�.(ip,gate,mask)
	BOOL GetNicDetailInfo(PNIC_INFO pInfo);

	//�ٸ� �ܺ� DNS üũ
	BOOL CheckOtherByDNS(PNIC_INFO pInfo);

	//���� ������ DNS ����� üũ�Ѵ�.
	BOOL CheckCurrentDns();

	//DNS ����
	BOOL SetCurrentDns(char* pszDNSAddr, PNIC_INFO pInfo);

	//DNS ����
	BOOL RecoveryDns(PNIC_INFO pInfo);

	//Ping�� ��������.
	int SendPing(char* pszIPAddr, int nPacketAmount);

	char* ConvertWCtoC(WCHAR* str);
};

