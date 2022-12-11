#include "stdafx.h"
#include "CProcessDebugHandler.h"


#include <WbemIdl.h>
#pragma comment(lib, "wbemuuid")

CProcessDebugHandler::CProcessDebugHandler(void)
{
	m_bStop = FALSE;
	m_pArrayProcList = NULL;
}


CProcessDebugHandler::~CProcessDebugHandler(void)
{
}

DWORD CProcessDebugHandler::Run()
{
	int nCount = 0;
	CWWaitableCollection	WaitSet;
	WaitSet.AddObject(m_ExitEvent);

	DWORD dwPID = 0;
	std::map<DWORD, char*> map_Proc;
	int nProcessCnt;

	while (TRUE)
	{
		m_mtx.lock();

		if (m_pArrayProcList != NULL && m_pArrayProcList->GetCount() > 0)
		{
			if (m_bStop)
			{
				break;
			}
			//PID 찾기
			if (WMI_GetProcessID(&map_Proc))
			{
				if (map_Proc.size() > 0)
				{
					std::map<DWORD, char*>::iterator Prociter;

					for (Prociter = map_Proc.begin(); Prociter != map_Proc.end(); )
					{
						dwPID = (DWORD)Prociter->first;
						char* pszProc = (char*)Prociter->second;

						std::map<DWORD, PVOID>::iterator iter = m_map.find(dwPID);
						if (iter == m_map.end())
						{
							//없음 - 추가
							CProcessDebugSubThread* pHandler = NULL;
							pHandler = new CProcessDebugSubThread;
							if (pHandler != NULL)
							{
								pHandler->SetProcessID(dwPID, pszProc);
								pHandler->Start();
								m_map.insert(std::pair<DWORD, PVOID>(dwPID, (PVOID)pHandler));
								printf("추가 : %lu\n", dwPID);
							}
						}
						Prociter++;
					}
				}
			}
		}

		DeleteSubHandler(map_Proc);

		m_mtx.unlock();
		

		if (WaitSet.Wait(TRUE, 1000) != WAIT_TIMEOUT)
		{
			break;
		}
	}

	DeleteProcList();
	DeleteAllSubHandler();
	m_evtWait.Set();
	return 0;
}

void CProcessDebugHandler::SetEndAndWait()
{
	m_bStop = TRUE;
	m_ExitEvent.Set();
	m_evtWait.Wait();
	Stop();
}

void CProcessDebugHandler::SetList(CArray<char*, char*> *pArrayProcList)
{
	m_mtx.lock();

	DeleteProcList();

	m_pArrayProcList = pArrayProcList;

	m_mtx.unlock();
}

//PID를 가져온다.
BOOL CProcessDebugHandler::WMI_GetProcessID(std::map<DWORD, char*>* map_Proc)
{
	CString strData = _T("");
	BOOL bRet = FALSE;
	BOOL bGetNicInfo = FALSE;
	HRESULT hRet = S_OK;

	IWbemLocator* pLoc = NULL;
	IWbemServices* pSvc = NULL;
	IEnumWbemClassObject* pEnumerator = NULL;
	IWbemClassObject* pWCObject = NULL;

	hRet = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
	if (SUCCEEDED(hRet))
	{
		hRet = pLoc->ConnectServer(L"ROOT\\CIMV2", NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
		if (SUCCEEDED(hRet))
		{
			hRet = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
			if (SUCCEEDED(hRet))
			{
				CString strSubQuery = _T("");
				CString strQuery = _T("");
				if (m_pArrayProcList != NULL)
				{
					for (int i = 0; i < m_pArrayProcList->GetCount(); i++)
					{
						char* pszProc = m_pArrayProcList->GetAt(i);
						if (pszProc != NULL)
						{
							strQuery.Format("SELECT ProcessId FROM Win32_Process Where Caption = \"%s\"", pszProc);
							BSTR bstrQuery = strQuery.AllocSysString();
							hRet = pSvc->ExecQuery(L"WQL", bstrQuery, WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnumerator);
							SysFreeString(bstrQuery);

							if (SUCCEEDED(hRet))
							{
								ULONG puReturn = 0;
								while (pEnumerator)
								{
									pEnumerator->Next(WBEM_INFINITE, 1, &pWCObject, &puReturn);

									if (puReturn <= 0) break;	//Instance 없으면 break

									if (pWCObject != NULL)
									{
										VARIANT vtProp;
										if (SUCCEEDED(pWCObject->Get(L"ProcessId", 0, &vtProp, 0, 0)))
										{
											if (V_VT(&vtProp) & VT_I4)
											{
												//pProcIDList->AddTail(vtProp.intVal);
												map_Proc->insert(std::pair<DWORD, char*>((DWORD)vtProp.intVal, pszProc));

											}
											VariantClear(&vtProp);
										}

										if (pWCObject != NULL)
										{
											pWCObject->Release();
											pWCObject = NULL;
										}
									}
								}
								pEnumerator->Release();
								pEnumerator = NULL;
							}
						}
					}
				}
				pSvc->Release();
				pSvc = NULL;
			}
		}
		pLoc->Release();
		pLoc = NULL;
	}


	if (map_Proc != NULL && map_Proc->size() > 0)
	{
		bRet = TRUE;
	}


	return bRet;
}

void CProcessDebugHandler::DeleteProcList()
{
	if (m_pArrayProcList != NULL)
	{
		for (int i = 0; i < m_pArrayProcList->GetCount(); i++)
		{
			char* pszProc = m_pArrayProcList->GetAt(i);
			if (pszProc != NULL)
			{
				delete[] pszProc;
				pszProc = NULL;
			}
		}

		m_pArrayProcList->RemoveAll();
		m_pArrayProcList->FreeExtra();

		delete m_pArrayProcList;
		m_pArrayProcList = NULL;
	}
}

void CProcessDebugHandler::DeleteSubHandler(std::map<DWORD, char*>& map_Proc)
{
	if (m_map.size() > 0)
	{
		std::map<DWORD, PVOID>::iterator iter;

		for (iter = m_map.begin(); iter != m_map.end(); )
		{
			BOOL bfind = FALSE;
			DWORD dwPID_map = (DWORD)iter->first;
			CProcessDebugSubThread* pHandler = (CProcessDebugSubThread*)iter->second;

			if (pHandler != NULL)
			{
				if (map_Proc.find(dwPID_map) == map_Proc.end())
				{
					pHandler->SetEndAndWait();
					delete pHandler;
					pHandler = NULL;
					++iter;
					m_map.erase(dwPID_map);
				}
				else
				{
					iter++;
				}
			}
		}
	}
}

void CProcessDebugHandler::DeleteAllSubHandler()
{
	if (m_map.size() > 0)
	{
		std::map<DWORD, PVOID>::iterator iter;

		for (iter = m_map.begin(); iter != m_map.end(); iter++)
		{
			BOOL bfind = FALSE;
			DWORD dwPID_map = (DWORD)iter->first;
			CProcessDebugSubThread* pHandler = (CProcessDebugSubThread*)iter->second;

			if (pHandler != NULL)
			{
				pHandler->SetEndAndWait();
				delete pHandler;
				pHandler = NULL;
				++iter;
				m_map.erase(dwPID_map);
			}
		}
	}
}