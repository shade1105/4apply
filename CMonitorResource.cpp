#include "stdafx.h"
#include "CMonitorResourceHandler.h"

CMonitorResourceHandler::CMonitorResourceHandler(void)
{
	m_bNextStop = FALSE;		//���� �÷���
	m_dwProcessorNumber = 0;
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	m_dwProcessorNumber = sysInfo.dwNumberOfProcessors;

	m_bFlag = FALSE;
	m_dwCPULimit = 0;
	m_dwRAMLimit = 0;
	m_dwNetworkLimit = 0;
	m_dwIOLimit = 0;
}

CMonitorResourceHandler::~CMonitorResourceHandler(void)
{
}

DWORD CMonitorResourceHandler::Run()
{
	int		nRet = FP_RESULT_FAIL;
	char szCurrentTime[FP_MAX_PATH];
	char szJsonPath[FP_MAX_PATH];
	DWORD dwTime = 1000;// *60;
	DWORD dwCPUMEMTime = 0;
	CWWaitableCollection	WaitSet;
	WaitSet.AddObject(m_ExitEvent);

	time_t timer;
	struct tm stCurrentTime;
	
	PRESOURCE_INFO pstResourceInfo = NULL;
	pstResourceInfo = (PRESOURCE_INFO)MALLOC(sizeof(RESOURCE_INFO));
	ZeroMemory(pstResourceInfo, sizeof(RESOURCE_INFO));

	while (TRUE)
	{
		timer = time(NULL);
		localtime_s(&stCurrentTime, &timer);
		ZeroMemory(szCurrentTime, sizeof(szCurrentTime));

		StringCchPrintf(szCurrentTime, sizeof(szCurrentTime), "%04d%02d%02d%02d%02d%02d", stCurrentTime.tm_year + 1900
			, stCurrentTime.tm_mon + 1
			, stCurrentTime.tm_mday
			, stCurrentTime.tm_hour
			, stCurrentTime.tm_min
			, stCurrentTime.tm_sec);

		ZeroMemory(szJsonPath, sizeof(szJsonPath));
		StringCchPrintf(szJsonPath, sizeof(szJsonPath), "%s\\%s\\%s", g_Global.wfnGetCurrentDir(), DIR_CLIENT_RESULT, JSON_CLIENT_RESULT_CPU_MEM);
		nRet = GetCpuAndMemInfo(pstResourceInfo);

		//1���� �������� �ʱ�ȭ
		dwCPUMEMTime += dwTime;
		if (dwCPUMEMTime >= 1000 * 60)
			dwCPUMEMTime = 0;

		// ���ҽ� �Ӱ�ġ ���� ��
		if (m_bFlag)
		{
			ZeroMemory(szJsonPath, sizeof(szJsonPath));
			StringCchPrintf(szJsonPath, sizeof(szJsonPath), "%s\\%s\\%s", g_Global.wfnGetCurrentDir(), DIR_CLIENT_RESULT, JSON_CLIENT_RESULT_RESOURCE_LIMIT);
			nRet = GetNetworkDiskInfo(pstResourceInfo);
			if (nRet == FP_RESULT_SUCCESS)
			{
				if (!g_pGlobalFuncVarCls->IsRunningCheckDisk() && !g_pGlobalFuncVarCls->IsRunningWinsat())
				{
					//�Ӱ�ġ �Ѿ����� Ȯ���ϰ� �� �κ� Top5 ���μ��� ��������
					if (atol(pstResourceInfo->szCpuUsaged) > m_dwCPULimit)
					{
						pstResourceInfo->bCpuOver = TRUE;
					}
					if (atol(pstResourceInfo->szMemUsagePer) > m_dwRAMLimit)
					{
						pstResourceInfo->bRamOver = TRUE;
					}
					if (pstResourceInfo->dwNetworkTotal > m_dwNetworkLimit)
					{
						pstResourceInfo->bNetworkOver = TRUE;
					}
					if (pstResourceInfo->dwDiskTotal > m_dwIOLimit)
					{
						pstResourceInfo->bDiskOver = TRUE;
					}

					if (pstResourceInfo->bCpuOver || pstResourceInfo->bRamOver || pstResourceInfo->bNetworkOver || pstResourceInfo->bDiskOver)
					{
						CJsonCreator jsonCreator;
						jsonCreator.CreateResourceLimitJson(szJsonPath, szCurrentTime, pstResourceInfo, m_dwCPULimit, m_dwRAMLimit, m_dwNetworkLimit, m_dwIOLimit);

						pstResourceInfo->bCpuOver = FALSE;
						pstResourceInfo->bRamOver = FALSE;
						pstResourceInfo->bNetworkOver = FALSE;
						pstResourceInfo->bDiskOver = FALSE;
					}
				}
				else
				{
					AddLogMessage(__LINE__, "CMonitorResourceHandler - Send ResourceLimit Pass");
					pstResourceInfo->bCpuOver = FALSE;
					pstResourceInfo->bRamOver = FALSE;
					pstResourceInfo->bNetworkOver = FALSE;
					pstResourceInfo->bDiskOver = FALSE;
				}
			}
		}

		if (WaitSet.Wait(TRUE, dwTime) != WAIT_TIMEOUT)
		{
			break;
		}
	}
	FREE(pstResourceInfo);

	return 0;
}

void CMonitorResourceHandler::StopAndWait()
{
	m_bNextStop = TRUE;
	m_ExitEvent.Set();
	Stop();
}


void CMonitorResourceHandler::SetResourceLimit(BOOL bFlag, DWORD dwCPULimit, DWORD dwRAMLimit, DWORD dwNetworkLimit, DWORD dwIOLimit)
{
	m_bFlag = bFlag;
	m_dwCPULimit = dwCPULimit;
	m_dwRAMLimit = dwRAMLimit;
	m_dwNetworkLimit = dwNetworkLimit;
	m_dwIOLimit = dwIOLimit;
}
