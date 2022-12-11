#include "stdafx.h"
#include "CCheckNetwork.h"

#include <netlistmgr.h>
#include <WbemIdl.h>
#include <WS2tcpip.h>
#include <IcmpAPI.h>
#pragma comment(lib, "wbemuuid")
#pragma comment(lib, "Ws2_32")
#pragma comment(lib, "Iphlpapi")

CCheckNetwork::CCheckNetwork()
{

}

CCheckNetwork::~CCheckNetwork()
{

}

//현재 네트워크 상태를 체크한다.
int CCheckNetwork::CheckNetworkStatusByMac(char* pszGuid, int& nRetCode)
{
	int nRet = RESULT_NET_SUCCESS;
	ArrNicInfo arrNicInfo;

	if (GetNICInfo(arrNicInfo))
	{
		for (int i = 0; i < arrNicInfo.GetCount(); i++)
		{
			PNIC_INFO pInfo = arrNicInfo.GetAt(i);
			if (pInfo != NULL)
			{
				if (_stricmp(pInfo->szNICID, pszGuid) == 0)
				{
					if (pInfo->nNICStatus != WORKING)
					{
						//NIC Error
						nRetCode = pInfo->nNICStatus;
						nRet = RESULT_NIC_OFF;
						goto _END;
					}
					else
					{
						if (pInfo->nNetStatus != LAN_CONNECTED)
						{
							//LAN Cable Error
							nRetCode = pInfo->nNetStatus;
							nRet = RESULT_LAN_UNCONNECTED;
							goto _END;
						}

						if (strlen(pInfo->szMacAddr) > 0 && pInfo->arrIp.IsEmpty())
						{
							//NIC On / IPV4 Off
							nRet = RESULT_IPV4_OFF;
							goto _END;
						}
					}

					BOOL bConnected = FALSE;
					for (int j = 0; j < pInfo->arrGateway.GetCount(); j++)
					{
						CComBSTR bstrGate = pInfo->arrGateway.GetAt(j);
						if (bstrGate.Length() > 0)
						{
							char* pszGateAddr = ConvertWCtoC(bstrGate.m_str);
							if (pszGateAddr != NULL)
							{
								if (SendPing(pszGateAddr, 10) == RESULT_PING_SUCCESS)
								{
									bConnected = TRUE;
								}
								delete[]pszGateAddr;
								pszGateAddr = NULL;
							}
						}
					}

					if (!bConnected)
					{
						//Gateway Ping Fail
						nRet = RESULT_LOCAL_ERROR;
						goto _END;
					}

					if(!CheckOtherByDNS(pInfo))
					{
						//DNS Error
						nRet = RESULT_EXTERNAL_ERROR;
						goto _END;
					}
					break;
				}
				else
				{
					//NIC Match Fail
					nRet = RESULT_NIC_NONE;
				}
			}
		}
	}
	else
	{
		//NIC Find Fail
		nRet = RESULT_NIC_NONE;
	}

_END:
	if (!arrNicInfo.IsEmpty())
	{
		for (int i = 0; i < arrNicInfo.GetCount(); i++)
		{
			PNIC_INFO pInfo = arrNicInfo.GetAt(i);
			if (pInfo != NULL)
			{
				if (!pInfo->arrIp.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrIp.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrIp.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrIp.RemoveAll();
					pInfo->arrIp.FreeExtra();
				}

				if (!pInfo->arrMask.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrMask.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrMask.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrMask.RemoveAll();
					pInfo->arrMask.FreeExtra();
				}

				if (!pInfo->arrGateway.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrGateway.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrGateway.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrGateway.RemoveAll();
					pInfo->arrGateway.FreeExtra();
				}

				if (!pInfo->arrDns.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrDns.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrDns.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrDns.RemoveAll();
					pInfo->arrDns.FreeExtra();
				}

				delete pInfo;
				pInfo = NULL;
			}
		}
		arrNicInfo.RemoveAll();
		arrNicInfo.FreeExtra();
	}

	return nRet;
}

//현재 네트워크 상태를 체크한다.
int CCheckNetwork::CheckNetworkStatusByAll(ArrNicResult& arrNicResult)
{
	int nRet = RESULT_NET_SUCCESS;
	PNIC_RESULT pNicRet = NULL;
	ArrNicInfo arrNicInfo;

	if (GetNICInfo(arrNicInfo))
	{
		for (int i = 0; i < arrNicInfo.GetCount(); i++)
		{
			PNIC_INFO pInfo = arrNicInfo.GetAt(i);
			if (pInfo != NULL)
			{
				if (pInfo->nNICStatus != WORKING)
				{
					pNicRet = new NIC_RESULT;
					StringCchPrintf(pNicRet->szNicName, sizeof(pNicRet->szNicName), "%s", pInfo->szNicName);
					StringCchPrintf(pNicRet->szMacAddr, sizeof(pNicRet->szMacAddr), "%s", pInfo->szMacAddr);
					pNicRet->nNICStatus = pInfo->nNICStatus;
					pNicRet->nRet = RESULT_NIC_OFF;
					arrNicResult.Add(pNicRet);
					nRet = RESULT_NIC_OFF;
					continue;
				}
				else
				{
					if (pInfo->nNetStatus != LAN_CONNECTED)
					{
						pNicRet = new NIC_RESULT;
						StringCchPrintf(pNicRet->szNicName, sizeof(pNicRet->szNicName), "%s", pInfo->szNicName);
						StringCchPrintf(pNicRet->szMacAddr, sizeof(pNicRet->szMacAddr), "%s", pInfo->szMacAddr);
						pNicRet->nNetStatus = pInfo->nNetStatus;
						pNicRet->nRet = RESULT_LAN_UNCONNECTED;
						arrNicResult.Add(pNicRet);
						nRet = RESULT_LAN_UNCONNECTED;
						continue;
					}

					if (strlen(pInfo->szMacAddr) > 0 && pInfo->arrIp.IsEmpty())
					{
						pNicRet = new NIC_RESULT;
						StringCchPrintf(pNicRet->szNicName, sizeof(pNicRet->szNicName), "%s", pInfo->szNicName);
						StringCchPrintf(pNicRet->szMacAddr, sizeof(pNicRet->szMacAddr), "%s", pInfo->szMacAddr);
						pNicRet->nNetStatus = pInfo->nNetStatus;
						pNicRet->nRet = RESULT_IPV4_OFF;
						arrNicResult.Add(pNicRet);
						nRet = RESULT_IPV4_OFF;
						continue;
					}
				}

				BOOL bConnected = FALSE;
				for (int j = 0; j < pInfo->arrGateway.GetCount(); j++)
				{
					CComBSTR bstrGate = pInfo->arrGateway.GetAt(j);
					if (bstrGate.Length() > 0)
					{
						char* pszGateAddr = ConvertWCtoC(bstrGate.m_str);
						if (pszGateAddr != NULL)
						{
							if (SendPing(pszGateAddr, 10) == RESULT_PING_SUCCESS)
							{
								bConnected = TRUE;
							}
							delete[]pszGateAddr;
							pszGateAddr = NULL;
						}
					}
				}

				if (!bConnected)
				{
					pNicRet = new NIC_RESULT;
					StringCchPrintf(pNicRet->szNicName, sizeof(pNicRet->szNicName), "%s", pInfo->szNicName);
					StringCchPrintf(pNicRet->szMacAddr, sizeof(pNicRet->szMacAddr), "%s", pInfo->szMacAddr);
					pNicRet->nNetStatus = pInfo->nNetStatus;
					pNicRet->nRet = RESULT_LOCAL_ERROR;
					arrNicResult.Add(pNicRet);
					nRet = RESULT_LOCAL_ERROR;
					continue;
				}

				if (!CheckOtherByDNS(pInfo))
				{
					pNicRet = new NIC_RESULT;
					StringCchPrintf(pNicRet->szNicName, sizeof(pNicRet->szNicName), "%s", pInfo->szNicName);
					StringCchPrintf(pNicRet->szMacAddr, sizeof(pNicRet->szMacAddr), "%s", pInfo->szMacAddr);
					pNicRet->nNetStatus = pInfo->nNetStatus;
					pNicRet->nRet = RESULT_EXTERNAL_ERROR;
					arrNicResult.Add(pNicRet);
					nRet = RESULT_EXTERNAL_ERROR;
					continue;
				}
				//NETWORK Success
				nRet = RESULT_NET_SUCCESS;
				break;
			}
		}
	}
	else
	{
		nRet = RESULT_NIC_NONE;
	}


	if (!arrNicInfo.IsEmpty())
	{
		for (int i = 0; i < arrNicInfo.GetCount(); i++)
		{
			PNIC_INFO pInfo = arrNicInfo.GetAt(i);
			if (pInfo != NULL)
			{
				if (!pInfo->arrIp.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrIp.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrIp.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrIp.RemoveAll();
					pInfo->arrIp.FreeExtra();
				}

				if (!pInfo->arrMask.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrMask.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrMask.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrMask.RemoveAll();
					pInfo->arrMask.FreeExtra();
				}

				if (!pInfo->arrGateway.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrGateway.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrGateway.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrGateway.RemoveAll();
					pInfo->arrGateway.FreeExtra();
				}

				if (!pInfo->arrDns.IsEmpty())
				{
					for (int j = 0; j < pInfo->arrDns.GetCount(); j++)
					{
						CComBSTR bstr = pInfo->arrDns.GetAt(j);
						SysFreeString(bstr);
					}
					pInfo->arrDns.RemoveAll();
					pInfo->arrDns.FreeExtra();
				}

				delete pInfo;
				pInfo = NULL;
			}
		}
		arrNicInfo.RemoveAll();
		arrNicInfo.FreeExtra();
	}

	return nRet;
}

//윈도우 네트워크 상태 값을 가져온다.
BOOL CCheckNetwork::CheckInternetConnect()
{
	BOOL bRet = FALSE;
	HRESULT hr = S_FALSE;

	INetworkListManager* pNetworkListManager = NULL;
	hr = CoCreateInstance(CLSID_NetworkListManager, NULL, CLSCTX_ALL, __uuidof(INetworkListManager), (LPVOID*)&pNetworkListManager);
	if (SUCCEEDED(hr) && pNetworkListManager != NULL)
	{
		int nlmConnectivity = NLM_CONNECTIVITY_DISCONNECTED;
		VARIANT_BOOL isConnected = VARIANT_FALSE;
		hr = pNetworkListManager->get_IsConnectedToInternet(&isConnected);
		if (SUCCEEDED(hr))
		{
			if (isConnected == VARIANT_TRUE)
			{
				bRet = TRUE;
			}
		}
		else
		{
		}

		pNetworkListManager->Release();
		pNetworkListManager = NULL;
	}
	else
	{
	}

	return bRet;
}

//NIC 정보를 가져온다.
BOOL CCheckNetwork::GetNICInfo(ArrNicInfo& arrNicInfo)
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
				hRet = pSvc->ExecQuery(L"WQL", L"SELECT GUID,Manufacturer,Description,ConfigManagerErrorCode, NetConnectionStatus from Win32_NetworkAdapter", WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnumerator);

				if (SUCCEEDED(hRet))
				{
					ULONG puReturn = 0;
					while (pEnumerator)
					{
						pEnumerator->Next(WBEM_INFINITE, 1, &pWCObject, &puReturn);

						if (puReturn <= 0) break;	\

						if (pWCObject != NULL)
						{
							VARIANT vtProp;

							if (SUCCEEDED(pWCObject->Get(L"Manufacturer", 0, &vtProp, 0, 0)))
							{
								if (V_VT(&vtProp) & VT_BSTR)
								{
									if(_wcsicmp(vtProp.bstrVal, L"Microsoft") != 0)
									{
										PNIC_INFO pNicInfo = new NIC_INFO;
										VariantClear(&vtProp);
										if (SUCCEEDED(pWCObject->Get(L"GUID", 0, &vtProp, 0, 0)))
										{
											if (V_VT(&vtProp) & VT_BSTR)
											{
												strData = vtProp.bstrVal;
												StringCchPrintf(pNicInfo->szNICID, sizeof(pNicInfo->szNICID), "%s", strData);
											}
											VariantClear(&vtProp);
										}

										if (SUCCEEDED(pWCObject->Get(L"Description", 0, &vtProp, 0, 0)))
										{
											if (V_VT(&vtProp) & VT_BSTR)
											{
												strData = vtProp.bstrVal;
												StringCchPrintf(pNicInfo->szNicName, sizeof(pNicInfo->szNICID), "%s", strData);
											}
											VariantClear(&vtProp);
										}

										if (SUCCEEDED(pWCObject->Get(L"ConfigManagerErrorCode", 0, &vtProp, 0, 0)))
										{
											if (V_VT(&vtProp) & VT_I4)
											{
												pNicInfo->nNICStatus = vtProp.lVal;
											}
											VariantClear(&vtProp);
										}

										if (SUCCEEDED(pWCObject->Get(L"NetConnectionStatus", 0, &vtProp, 0, 0)))
										{
											if (V_VT(&vtProp) & VT_I4)
											{
												pNicInfo->nNetStatus = vtProp.lVal;
											}
											VariantClear(&vtProp);
										}

										/// NIC 설정된 세부사항 가져오기
										bGetNicInfo = GetNicDetailInfo(pNicInfo);

										if (bGetNicInfo)
										{
											arrNicInfo.Add(pNicInfo);
										}
										else
										{
											if (!pNicInfo->arrGateway.IsEmpty())
											{
												for(int i = 0; i < pNicInfo->arrGateway.GetCount(); i++)
												{
													CComBSTR bstr = (CComBSTR)pNicInfo->arrGateway.GetAt(i);
													if (bstr.Length() > 0)
													{
														SysFreeString(bstr);
													}
												}
												pNicInfo->arrGateway.RemoveAll();
												pNicInfo->arrGateway.FreeExtra();
											}

											if (!pNicInfo->arrIp.IsEmpty())
											{
												for (int i = 0; i < pNicInfo->arrIp.GetCount(); i++)
												{
													CComBSTR bstr = (CComBSTR)pNicInfo->arrIp.GetAt(i);
													if (bstr.Length() > 0)
													{
														SysFreeString(bstr);
													}
												}
												pNicInfo->arrIp.RemoveAll();
												pNicInfo->arrIp.FreeExtra();
											}

											if (!pNicInfo->arrMask.IsEmpty())
											{
												for (int i = 0; i < pNicInfo->arrMask.GetCount(); i++)
												{
													CComBSTR bstr = (CComBSTR)pNicInfo->arrMask.GetAt(i);
													if (bstr.Length() > 0)
													{
														SysFreeString(bstr);
													}
												}
												pNicInfo->arrMask.RemoveAll();
												pNicInfo->arrMask.FreeExtra();
											}

											if (!pNicInfo->arrDns.IsEmpty())
											{
												for (int i = 0; i < pNicInfo->arrDns.GetCount(); i++)
												{
													CComBSTR bstr = (CComBSTR)pNicInfo->arrDns.GetAt(i);
													if (bstr.Length() > 0)
													{
														SysFreeString(bstr);
													}
												}
												pNicInfo->arrDns.RemoveAll();
												pNicInfo->arrDns.FreeExtra();
											}

											delete pNicInfo;
											pNicInfo = NULL;
										}
									}
									else
									{
										VariantClear(&vtProp);
									}
								}
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

	if (pSvc != NULL)
	{
		pSvc->Release();
		pSvc = NULL;
	}
	
	if (pLoc != NULL)
	{
		pLoc->Release();
		pLoc = NULL;
	}
	
	if (!arrNicInfo.IsEmpty())
	{
		bRet = TRUE;
	}
	
	return bRet;
}

//NIC의 상세정보 가져온다.(ip,gate,mask)
BOOL CCheckNetwork::GetNicDetailInfo(PNIC_INFO pInfo)
{
	CString strData = _T("");
	BOOL bCompareID = FALSE;
	BOOL bRet = FALSE;
	ULONG puReturn = 0;
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
				hRet = pSvc->ExecQuery(L"WQL", L"SELECT SettingID,DefaultIPGateway,IPAddress,IPSubnet,MACAddress,Index,DNSServerSearchOrder from Win32_NetworkAdapterConfiguration", WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnumerator);
				if (SUCCEEDED(hRet))
				{
					while (pEnumerator != NULL)
					{
						bCompareID = FALSE;
						pEnumerator->Next(WBEM_INFINITE, 1, &pWCObject, &puReturn);

						if (puReturn <= 0) break;	//Instance 없으면 break

						if (pWCObject != NULL)
						{
							VARIANT vtProp;

							if (SUCCEEDED(pWCObject->Get(L"SettingID", 0, &vtProp, 0, 0)))
							{
								if (V_VT(&vtProp) & VT_BSTR)
								{
									strData = vtProp.bstrVal;
									// 가져온 ID 다르면 break
									if (strData.CompareNoCase(pInfo->szNICID) == 0)
									{
										bCompareID = TRUE;
									}
								}
								VariantClear(&vtProp);
							}
							if (bCompareID)
							{
								if (SUCCEEDED(pWCObject->Get(L"DefaultIPGateway", 0, &vtProp, 0, 0)))
								{
									if ((V_VT(&vtProp) & VT_ARRAY) && (V_VT(&vtProp) & VT_BSTR))
									{
										SAFEARRAY* safe = NULL;
										safe = V_ARRAY(&vtProp);
										if (safe != NULL)
										{
											ULONG nElements = safe->rgsabound[0].cElements;
											if (nElements > 0)
											{
												for (LONG i = 0; i < nElements; i++)
												{
													CComBSTR comBstr;
													if (SUCCEEDED(SafeArrayGetElement(safe, &i, &comBstr)))
													{
														pInfo->arrGateway.Add(comBstr);
														SysFreeString(comBstr);
													}
												}
											}
										}
									}
									VariantClear(&vtProp);
								}

								if (SUCCEEDED(pWCObject->Get(L"IPAddress", 0, &vtProp, 0, 0)))
								{
									if ((V_VT(&vtProp) & VT_ARRAY) && (V_VT(&vtProp) & VT_BSTR))
									{
										SAFEARRAY* safe = NULL;
										safe = V_ARRAY(&vtProp);
										if (safe != NULL)
										{
											ULONG nElements = safe->rgsabound[0].cElements;
											if (nElements > 0)
											{
												for (LONG i = 0; i < nElements; i++)
												{
													CComBSTR comBstr;
													if (SUCCEEDED(SafeArrayGetElement(safe, &i, &comBstr)))
													{
														if (wcsstr(comBstr.m_str, L":") == NULL)
														{
															//ipv6가 아닐때
															pInfo->arrIp.Add(comBstr);
														}
														SysFreeString(comBstr);
													}
												}
											}
										}
									}
									VariantClear(&vtProp);

								}

								if (SUCCEEDED(pWCObject->Get(L"IPSubnet", 0, &vtProp, 0, 0)))
								{
									if ((V_VT(&vtProp) & VT_ARRAY) && (V_VT(&vtProp) & VT_BSTR))
									{
										SAFEARRAY* safe = NULL;
										safe = V_ARRAY(&vtProp);
										if (safe != NULL)
										{
											ULONG nElements = safe->rgsabound[0].cElements;
											if (nElements > 0)
											{
												for (LONG i = 0; i < nElements; i++)
												{
													CComBSTR comBstr;
													if (SUCCEEDED(SafeArrayGetElement(safe, &i, &comBstr)))
													{
														if (wcsstr(comBstr.m_str, L".") != NULL)
														{
															pInfo->arrMask.Add(comBstr);
														}
														SysFreeString(comBstr);
													}
												}
											}
										}
									}
									VariantClear(&vtProp);
								}

								if (SUCCEEDED(pWCObject->Get(L"MACAddress", 0, &vtProp, 0, 0)))
								{
									if (V_VT(&vtProp) & VT_BSTR)
									{
										strData = vtProp.bstrVal;
										StringCchPrintf(pInfo->szMacAddr, sizeof(pInfo->szMacAddr), "%s", strData);
									}
									VariantClear(&vtProp);

								}

								if (SUCCEEDED(pWCObject->Get(L"Index", 0, &vtProp, 0, 0)))
								{
									if (V_VT(&vtProp) & VT_I4)
									{
										pInfo->nIndex = vtProp.lVal;
									}
									VariantClear(&vtProp);
								}

								if (SUCCEEDED(pWCObject->Get(L"DNSServerSearchOrder", 0, &vtProp, 0, 0)))
								{
									if ((V_VT(&vtProp) & VT_ARRAY) && (V_VT(&vtProp) & VT_BSTR))
									{
										SAFEARRAY* safe = NULL;
										safe = V_ARRAY(&vtProp);
										if (safe != NULL)
										{
											ULONG nElements = safe->rgsabound[0].cElements;
											if (nElements > 0)
											{
												for (LONG i = 0; i < nElements; i++)
												{
													CComBSTR comBstr;
													if (SUCCEEDED(SafeArrayGetElement(safe, &i, &comBstr)))
													{
														pInfo->arrDns.Add(comBstr);
														SysFreeString(comBstr);
													}
												}
											}
										}
									}
									VariantClear(&vtProp);
								}
								bRet = TRUE;
								if (pWCObject != NULL)
								{
									pWCObject->Release();
									pWCObject = NULL;
								}
								break;
							}

							pWCObject->Release();
							pWCObject = NULL;
						}
					}
					pEnumerator->Release();
					pEnumerator = NULL;
				}
			}
		}
	}

	if (pSvc != NULL)
	{
		pSvc->Release();
		pSvc = NULL;
	}

	if (pLoc != NULL)
	{
		pLoc->Release();
		pLoc = NULL;
	}

	return bRet;
}

//외부 DNS 체크
BOOL CCheckNetwork::CheckOtherByDNS(PNIC_INFO pInfo)
{
	BOOL bCompare = FALSE;
	BOOL bRet = FALSE;
	int nloop = 0;
	char* pszNameServerList[] =
	{
		"168.126.63.1",		//KT
		"219.250.36.130",	//SK BroadBand
		"8.8.8.8"			//GOOGLE
	};

	nloop = sizeof(pszNameServerList) / sizeof(char*);
	if (!CheckCurrentDns())
	{
		for (int i = 0; i < nloop; i++)
		{
			bCompare = FALSE;
			for (int j = 0; j < pInfo->arrDns.GetCount(); j++)
			{
				CComBSTR bstrDns = pInfo->arrDns.GetAt(j);
				if (bstrDns.Length() > 0)
				{
					char* pszDnsAddr = ConvertWCtoC(bstrDns.m_str);
					if (pszDnsAddr != NULL)
					{
						if (strcmp(pszDnsAddr, pszNameServerList[i]) == 0)
						{
							bCompare = TRUE;
							delete[]pszDnsAddr;
							pszDnsAddr = NULL;
							break;
						}
						delete[]pszDnsAddr;
						pszDnsAddr = NULL;
					}
				}
			}

			if (!bCompare)
			{
				SetCurrentDns(pszNameServerList[i], pInfo);
				bRet = CheckCurrentDns();
				if (bRet)
				{
					break;
				}
			}
		}
		RecoveryDns(pInfo);
	}
	else
	{
		bRet = TRUE;
	}

	return bRet;
}

//현재 설정된 DNS 통신을 체크한다.
BOOL CCheckNetwork::CheckCurrentDns()
{
	BOOL bRet = FALSE;
	addrinfo hints;
	addrinfo* pTarget = NULL;
	addrinfo* pGrabAllTargetIPs = NULL;
	CString strTargetDomain = _T("");

	WORD wVersionRequested;
	WSADATA wsaData;
	int nerror = 0;
	int nDomainCount = 0;

	wVersionRequested = MAKEWORD(2, 2);

	nerror = WSAStartup(wVersionRequested, &wsaData);

	if (nerror == 0)
	{
		char* pszIPList[] =		// 확인할 도메인 리스트
		{
			"www.google.com",
			"www.naver.com",
			//"www.daum.net",
			//"www.microsoft.com",
			//"www.yahoo.com"
		};
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;

		int nloop = sizeof(pszIPList) / sizeof(char*);
		for (int i = 0; i < nloop; i++)
		{
			strTargetDomain = pszIPList[i];
			int nstatus = getaddrinfo(strTargetDomain, NULL, &hints, &pTarget);

			if (nstatus != 0)
			{
				//printf("%s IP 맵핑 불가\n", pszIPList[i]);
				//printf("getaddrinfo() error: %d\n", nstatus);
			}

			pGrabAllTargetIPs = NULL;
			for (pGrabAllTargetIPs = pTarget; pGrabAllTargetIPs != NULL; pGrabAllTargetIPs = pGrabAllTargetIPs->ai_next)
			{
				char printableIP[INET_ADDRSTRLEN];
				ZeroMemory(printableIP, sizeof(printableIP));

				struct sockaddr_in* ipv4 = (struct sockaddr_in*)pGrabAllTargetIPs->ai_addr;
				void* ipAddress = &(ipv4->sin_addr);

				inet_ntop(ipv4->sin_family, ipAddress, printableIP, sizeof(printableIP));

				//printf("도메인 \t%s의 IP : %s\n", pszIPList[i], printableIP);
				nDomainCount++;
			}
			freeaddrinfo(pTarget);
			pTarget = NULL;	
		}
	}
	else
	{
		//printf("WSAStartup failed with error: %d\n", nerror);
	}

	WSACleanup();

	if (nDomainCount > 0)
	{
		bRet = TRUE;
	}

	printf("\n");
	return bRet;
}

//DNS 설정
BOOL CCheckNetwork::SetCurrentDns(char* pszDNSAddr, PNIC_INFO pInfo)
{
	CString strInstancePath = _T("");
	CString strDNS = _T("");
	BSTR bstrDnsAddr = NULL;
	BSTR bstrInstancePath = NULL;
	HRESULT hRet = S_OK;
	BOOL bRet = FALSE;
	SAFEARRAY* pArrDns = NULL;
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
				IWbemClassObject* pClass = NULL;
				IWbemClassObject* pInInst_ES = NULL;
				IWbemClassObject* pInClass_ES = NULL;
				hRet = pSvc->GetObjectA(L"Win32_NetworkAdapterConfiguration", 0, NULL, &pClass, NULL);
				if (SUCCEEDED(hRet))
				{
					hRet = pClass->GetMethod(L"SetDNSServerSearchOrder", 0, &pInClass_ES, NULL);
					if (SUCCEEDED(hRet))
					{
						hRet = pInClass_ES->SpawnInstance(0, &pInInst_ES);
						if (SUCCEEDED(hRet))
						{
							pArrDns = SafeArrayCreateVector(VT_BSTR, 0, 1);
							if (pArrDns != NULL)
							{
								strDNS.Format("%s", pszDNSAddr);
								bstrDnsAddr = strDNS.AllocSysString();
								if (bstrDnsAddr != NULL)
								{
									long nIdx[] = { 0 };
									nIdx[0] = 0;
									if (SUCCEEDED(SafeArrayPutElement(pArrDns, nIdx, bstrDnsAddr)))
									{
										VARIANT arg1_ES;
										VariantInit(&arg1_ES);
										arg1_ES.vt = VT_ARRAY | VT_BSTR;
										arg1_ES.parray = pArrDns;

										if (WBEM_S_NO_ERROR == pInInst_ES->Put(L"DNSServerSearchOrder", 0, &arg1_ES, 0))
										{
											strInstancePath.Format("Win32_NetworkAdapterConfiguration.Index='%d'", pInfo->nIndex);
											bstrInstancePath = strInstancePath.AllocSysString();
											if (bstrInstancePath != NULL)
											{
												IWbemClassObject* pOutInst = NULL;
												hRet = pSvc->ExecMethod(bstrInstancePath, L"SetDNSServerSearchOrder", 0, NULL, pInInst_ES, &pOutInst, NULL);

												if (SUCCEEDED(hRet))
												{
													bRet = TRUE;
												}

												if (pOutInst != NULL)
												{
													pOutInst->Release();
													pOutInst = NULL;
												}

												SysFreeString(bstrInstancePath);
												bstrInstancePath = NULL;
											}
										}
									}
									SysFreeString(bstrDnsAddr);
									bstrDnsAddr = NULL;
								}
								SafeArrayDestroy(pArrDns);
								pArrDns = NULL;
							}
							pInInst_ES->Release();
							pInInst_ES = NULL;
						}
						pInClass_ES->Release();
						pInClass_ES = NULL;
					}
					pClass->Release();
					pClass = NULL;
				}
			}
		}
	}

	if (pSvc != NULL)
	{
		pSvc->Release();
		pSvc = NULL;
	}

	if (pLoc != NULL)
	{
		pLoc->Release();
		pLoc = NULL;
	}
	return bRet;
}

//DNS 원복
BOOL CCheckNetwork::RecoveryDns(PNIC_INFO pInfo)
{
	CString strInstancePath = _T("");
	CString strDNS = _T("");
	BSTR bstrDnsAddr = NULL;
	BSTR bstrInstancePath = NULL;
	HRESULT hRet = S_OK;
	BOOL bRet = FALSE;
	SAFEARRAY* pArrDns = NULL;
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
				IWbemClassObject* pClass = NULL;
				IWbemClassObject* pInInst_ES = NULL;
				IWbemClassObject* pInClass_ES = NULL;
				hRet = pSvc->GetObjectA(L"Win32_NetworkAdapterConfiguration", 0, NULL, &pClass, NULL);
				if (SUCCEEDED(hRet))
				{
					hRet = pClass->GetMethod(L"SetDNSServerSearchOrder", 0, &pInClass_ES, NULL);
					if (SUCCEEDED(hRet))
					{
						hRet = pInClass_ES->SpawnInstance(0, &pInInst_ES);
						if (SUCCEEDED(hRet))
						{
							pArrDns = SafeArrayCreateVector(VT_BSTR, 0, pInfo->arrDns.GetCount());
							if (pArrDns != NULL)
							{
								for (LONG i = 0; i < pInfo->arrDns.GetCount(); i++)
								{
									CComBSTR bstrDnsAddr = pInfo->arrDns.GetAt(i);
									if (bstrDnsAddr.Length() > 0)
									{
										if (SUCCEEDED(SafeArrayPutElement(pArrDns, &i, bstrDnsAddr.m_str)))
										{
											
										}
										SysFreeString(bstrDnsAddr);
									}
								}
								if (pArrDns->cbElements == pInfo->arrDns.GetCount())
								{
									VARIANT arg1_ES;
									VariantInit(&arg1_ES);
									arg1_ES.vt = VT_ARRAY | VT_BSTR;
									arg1_ES.parray = pArrDns;

									if (WBEM_S_NO_ERROR == pInInst_ES->Put(L"DNSServerSearchOrder", 0, &arg1_ES, 0))
									{
										strInstancePath.Format("Win32_NetworkAdapterConfiguration.Index='%d'", pInfo->nIndex);
										bstrInstancePath = strInstancePath.AllocSysString();
										if (bstrInstancePath != NULL)
										{
											IWbemClassObject* pOutInst = NULL;
											hRet = pSvc->ExecMethod(bstrInstancePath, L"SetDNSServerSearchOrder", 0, NULL, pInInst_ES, &pOutInst, NULL);

											if (SUCCEEDED(hRet))
											{
												bRet = TRUE;
											}

											if (pOutInst != NULL)
											{
												pOutInst->Release();
												pOutInst = NULL;
											}

											SysFreeString(bstrInstancePath);
											bstrInstancePath = NULL;
										}
									}
								}
								
								SafeArrayDestroy(pArrDns);
								pArrDns = NULL;
							}
							pInInst_ES->Release();
							pInInst_ES = NULL;
						}
						pInClass_ES->Release();
						pInClass_ES = NULL;
					}
					pClass->Release();
					pClass = NULL;
				}
			}
		}
	}

	if (pSvc != NULL)
	{
		pSvc->Release();
		pSvc = NULL;
	}

	if (pLoc != NULL)
	{
		pLoc->Release();
		pLoc = NULL;
	}

	return bRet;
}
//Ping을 보내본다.
int CCheckNetwork::SendPing(char* pszIPAddr, int nPacketAmount)
{
	int nRet = 0;
	int nPingResultCalc = 1;
	HANDLE hIcmpFile = NULL;
	unsigned long ptonResult = INADDR_NONE;	//주소 값 처리에 대한 결과임(주소가 아님)
	DWORD dwRetVal = 0;
	char SendData[32];
	ZeroMemory(SendData, sizeof(SendData));
	StringCchPrintf(SendData, sizeof(SendData), "%s", "Send Data");

	LPVOID ReplyBuffer = NULL;
	DWORD ReplySize = 0;

	in_addr addr;
	ptonResult = inet_pton(AF_INET, pszIPAddr, &addr.S_un.S_addr);

	//inet_ntop(AF_INET, &addr.S_un.S_addr, pszIPAddr, sizeof(pszIPAddr));
	if (ptonResult == INADDR_NONE)
	{
		//printf("주소 형식 변환 오류\n");
		return nRet;
	}

	hIcmpFile = IcmpCreateFile();
	if (hIcmpFile != NULL && hIcmpFile != INVALID_HANDLE_VALUE)
	{
		ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
		ReplyBuffer = (VOID*)malloc(ReplySize);
		if (ReplyBuffer != NULL)
		{
			int nSend = 0;
			int nReceived = 0;
			int nLost = 0;
			int nSuccess = 0;

			unsigned long ulMinRTT = pow(2, sizeof(ulMinRTT) * 8) - 1; //2^32 - 1
			unsigned long ulMaxRTT = 0;
			unsigned long ulSumRTT = 0;

			for (int i = 0; i < nPacketAmount; i++)
			{
				ZeroMemory(ReplyBuffer, ReplySize);
				dwRetVal = IcmpSendEcho(hIcmpFile, addr.S_un.S_addr, SendData, sizeof(SendData), NULL, ReplyBuffer, ReplySize, 1000);
				PICMP_ECHO_REPLY pEchoReply = (PICMP_ECHO_REPLY)ReplyBuffer;
				nSend++;

				char szipaddr[64];
				ZeroMemory(szipaddr, sizeof(szipaddr));
				unsigned long ulRequestStatus = 0;
				unsigned long ulRoundTripTime = 0;
				unsigned short usDataSize = 0;
				unsigned short usReserved = 0;
				ip_option_information Options;
				unsigned char TTL = 0;

				inet_ntop(AF_INET, &pEchoReply->Address, szipaddr, sizeof(szipaddr));
				ulRequestStatus = pEchoReply->Status;
				ulRoundTripTime = pEchoReply->RoundTripTime;

				usDataSize = pEchoReply->DataSize;
				usReserved = pEchoReply->Reserved;
				Options = pEchoReply->Options;
				TTL = Options.Ttl;

				switch (ulRequestStatus)
				{
				case IP_SUCCESS:
					nReceived++;
					nSuccess++;

					if (ulRoundTripTime < ulMinRTT)
					{
						ulMinRTT = ulRoundTripTime;
					}
					else if (ulRoundTripTime > ulMaxRTT)
					{
						ulMaxRTT = ulRoundTripTime;
					}
					ulSumRTT += ulRoundTripTime;

					if (ulRoundTripTime < 1)
					{
						//printf("시간<1 ");
					}
					else
					{
						//printf("시간=%dms ", ulRoundTripTime);
					}
					//printf("TTL=%d\n", TTL);

					nPingResultCalc *= SUCCESS;

					break;

				case IP_BUF_TOO_SMALL:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_DEST_NET_UNREACHABLE:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					//printf("")
					break;

				case IP_DEST_HOST_UNREACHABLE:
					nReceived++;
					nPingResultCalc *= UNREACHABLE;

					//printf("%s의 응답: 대상 호스트에 연결할 수 없습니다.\n", szipaddr);
					break;

				case IP_DEST_PROT_UNREACHABLE:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_DEST_PORT_UNREACHABLE:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_NO_RESOURCES:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_BAD_OPTION:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_HW_ERROR:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_PACKET_TOO_BIG:
					nReceived++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_REQ_TIMED_OUT:
					nLost++;
					nPingResultCalc *= REQ_TIMED_OUT;

					//printf("요청 시간이 만료되었습니다.\n");
					break;

				case IP_BAD_REQ:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_BAD_ROUTE:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_TTL_EXPIRED_TRANSIT:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_TTL_EXPIRED_REASSEM:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_PARAM_PROBLEM:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_SOURCE_QUENCH:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_OPTION_TOO_BIG:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_BAD_DESTINATION:
					nLost++;
					nPingResultCalc *= OTHER_ERROR;

					break;

				case IP_GENERAL_FAILURE:
					nLost++;
					nPingResultCalc *= GENERAL_FAILURE;

					//printf("PING: 전송하지 못했습니다. 일반 오류입니다.\n");
					break;
				}
			}

			//Ping 결과처리
			int nScnt = 0;
			int nUcnt = 0;
			int nRcnt = 0;
			int nGcnt = 0;
			int nOcnt = 0;
			while (nPingResultCalc % SUCCESS == 0)
			{
				nPingResultCalc /= SUCCESS;
				nScnt++;
			}
			while (nPingResultCalc % UNREACHABLE == 0)
			{
				nPingResultCalc /= UNREACHABLE;
				nUcnt++;
			}
			while (nPingResultCalc % REQ_TIMED_OUT == 0)
			{
				nPingResultCalc /= REQ_TIMED_OUT;
				nRcnt++;
			}
			while (nPingResultCalc % GENERAL_FAILURE == 0)
			{
				nPingResultCalc /= GENERAL_FAILURE;
				nGcnt++;
			}
			while (nPingResultCalc % OTHER_ERROR == 0)
			{
				nPingResultCalc /= OTHER_ERROR;
				nOcnt++;
			}

			if (nScnt >= 7)
			{
				nRet |= RESULT_PING_SUCCESS;
			}
			else
			{
				if (nUcnt > 0)
				{
					nRet |= RESULT_PING_UNREACHABLE;
				}
				if (nRcnt > 0)
				{
					nRet |= RESULT_PING_REQ_TIMED_OUT;
				}
				if (nGcnt > 0)
				{
					nRet |= RESULT_PING_GENERAL_FAILURE;
				}
				if (nOcnt > 0)
				{
					nRet |= RESULT_PING_OTHER_ERROR;
				}
			}

			//printf("\n");
			//printf("%s에 대한 Ping 통계:\n", pszIPAddr);
			//printf("    패킷: 보냄 = %d, 받음 = %d, 손실 = %d, (%d%% 손실),\n", nSend, nReceived, nLost, nLost * 100 / nSend);

			if (nRet & RESULT_PING_SUCCESS)
			{
				//printf("\t왕복 시간(밀리초):\n    최소 = %dms, 최대 = %dms, 평균 = %dms\n\n", ulMinRTT, ulMaxRTT, ulSumRTT / nReceived);
			}

			free(ReplyBuffer);
			ReplyBuffer = NULL;
		}

		

		if (hIcmpFile != NULL)
		{
			IcmpCloseHandle(hIcmpFile);
			hIcmpFile = NULL;
		}
	}
	else
	{
	}
	

	return nRet;
}

char* CCheckNetwork::ConvertWCtoC(WCHAR* str)
{
	//반환할 char* 변수 선언
	char* pStr = NULL;

	//입력받은 wchar_t 변수의 길이를 구함
	int strSize = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);

	//char* 메모리 할당
	pStr = new char[strSize];

	if (pStr != NULL)
	{
		//형 변환 
		WideCharToMultiByte(CP_ACP, 0, str, -1, pStr, strSize, 0, 0);
	}

	return pStr;
}