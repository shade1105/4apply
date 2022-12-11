#pragma once

class CMonitorResourceHandler : public CThread
{
public:
	CMonitorResourceHandler();
	~CMonitorResourceHandler();

public:
	virtual DWORD Run();
	void StopAndWait();
	void SetResourceLimit(BOOL nFlag, DWORD dwCPULimit, DWORD dwRAMLimit, DWORD dwNetworkLimit, DWORD dwIOLimit);

	
private:
	BOOL			 m_bNextStop;	//종료 플래그
	CWEvent			 m_ExitEvent;
	DWORD							m_dwProcessorNumber;

	BOOL m_bFlag;
	DWORD m_dwCPULimit;
	DWORD m_dwRAMLimit;
	DWORD m_dwNetworkLimit;
	DWORD m_dwIOLimit;
};

