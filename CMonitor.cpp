#include "pch.h"
#include "CMonitor.h"

CMonitor::CMonitor()
{
}

CMonitor::~CMonitor()
{
    m_mtxAlert.lock();
    RemoveMonData(&m_arrMonInfo);
    m_mtxAlert.unlock();
}

void CMonitor::Initialize()
{
    char szJsonPath[FP_MAX_PATH];
    ZeroMemory(szJsonPath, sizeof(szJsonPath));
    StringCchPrintf(szJsonPath, sizeof(szJsonPath), "%s\\%s\\%s", g_Global.wfnGetCurrentDir(), DIR_CLIENT_RESULT, JSON_CLIENT_RESULT_DISPLAY_INFO);

    if (g_Global.wfnFindFile(szJsonPath) > 0)
    {
        CheckMonChange(szJsonPath);
    }
    else
    {
        //�ʱ� ����� ���� ��������
        GetMonSettings(&m_arrMonInfo);
        //S�� ������ ������
        SendDisplayInfo(&m_arrMonInfo);
    }
}

void CMonitor::SendDisplayInfo(ArrayMonInfo * parrMonInfo)
{
    //���� �Ŀ� ���� ������ �ѹ� ����
    char szJsonPath[FP_MAX_PATH];
    ZeroMemory(szJsonPath, sizeof(szJsonPath));
    StringCchPrintf(szJsonPath, sizeof(szJsonPath), "%s\\%s\\%s", g_Global.wfnGetCurrentDir(), DIR_CLIENT_RESULT, JSON_CLIENT_RESULT_DISPLAY_INFO);
    HANDLE hWaitEvent = NULL;

    //json ����
    CMessageSender* pSender = new CMessageSender;
    if (pSender != NULL)
    {
        hWaitEvent = pSender->SendMonitorInfo(TCP_SEND_MONINFO, parrMonInfo);
        if (hWaitEvent != NULL)
        {
            ::WaitForSingleObject(hWaitEvent, INFINITE);
        }
    }
}


void CMonitor::GetMonSettings(ArrayMonInfo* m_parrMonInfo)
{
    EnumDisplayMonitors(NULL, NULL, Monitorenumproc, (LPARAM)m_parrMonInfo);


    //����ͺ��� ���̺� Ÿ��, ���� ��� �����ͼ� ����Ʈ�� ����
    PMonInfo pMonInfoTemp = NULL;
    for (int i = 0; i < m_parrMonInfo->GetCount(); i++)
    {
        PMonInfo pmoninfo = m_parrMonInfo->GetAt(i);

        //����� �����ϰ��
        if (strstr(pmoninfo->szIndex, "copy") != NULL)
        {
            pMonInfoTemp = pmoninfo;
            m_parrMonInfo->RemoveAt(i);
            GetMonitorWithWMI(m_parrMonInfo);
            
            int nsize = m_parrMonInfo->GetCount();
            char szIndex[64];
            ZeroMemory(szIndex, sizeof(szIndex));
            StringCchPrintf(szIndex, sizeof(szIndex), "%s", "DISPLAY");
            for (int j = 1; j <= nsize; j++)
            {
                if (j == 1)
                    StringCchPrintf(szIndex, sizeof(szIndex), "%s %d", szIndex, j);
                else
                    StringCchPrintf(szIndex, sizeof(szIndex), "%s|%d", szIndex, j);
            }

            for (int j = 0; j < nsize; j++)
            {
                pmoninfo = m_parrMonInfo->GetAt(j);
                pmoninfo->nType = DISPLAY_COPY;
                pmoninfo->rect.left = pMonInfoTemp->rect.left;
                pmoninfo->rect.right = pMonInfoTemp->rect.right;
                pmoninfo->rect.bottom = pMonInfoTemp->rect.bottom;
                pmoninfo->rect.top = pMonInfoTemp->rect.top;
                StringCchPrintf(pmoninfo->szResoulution, sizeof(pmoninfo->szResoulution), "%s", pMonInfoTemp->szResoulution);
                pmoninfo->bPrimary = FALSE;
                StringCchPrintf(pmoninfo->szIndex, sizeof(pmoninfo->szIndex), "%s", szIndex);
            }
        }
        else
        {
            //�ָ���� ����
            POINT ptZero = { 0 };
            HMONITOR hmonPrimary = MonitorFromPoint(ptZero, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO monitorinfo = { 0 };
            monitorinfo.cbSize = sizeof(monitorinfo);
            GetMonitorInfo(hmonPrimary, &monitorinfo);

            if (hmonPrimary == pmoninfo->hMonitor)
            {
                pmoninfo->bPrimary = TRUE;
            }
        }

        pmoninfo = m_parrMonInfo->GetAt(i);
        //���̺� Ÿ�� ����
        Get_Display_ConnectionType(pmoninfo->Device_Info.DeviceID, pmoninfo->nConnectionType);
        //������� ����
        Get_Display_ProductingDate(pmoninfo->Device_Info.DeviceID, pmoninfo->szProducingDate);
    }

    if (pMonInfoTemp != NULL)
    {
        delete pMonInfoTemp;
        pMonInfoTemp = NULL;
    }

    return;
}

void CMonitor::RemoveMonData(ArrayMonInfo* m_parrMonInfo)
{
    int nsize = m_parrMonInfo->GetSize();
    for (int i = 0; i < nsize; i++)
    {
        PMonInfo pMonInfo = m_parrMonInfo->GetAt(i);
        if (pMonInfo != NULL)
        {
            delete pMonInfo;
            pMonInfo = NULL;
        }
    }
    m_parrMonInfo->RemoveAll();
}

void CMonitor::CheckMonChange(char *pszJsonPath)
{
    //������ �ҷ��°�� �ε��ؼ� ���������� ����
    if (pszJsonPath != NULL)
    {
        CkJsonObject json;
        CkJsonArray* pArrDisplayInfo = NULL;
        CkJsonObject* pJsonObj = NULL;
        CkString ckstring;
        json.LoadFile(pszJsonPath);
        pArrDisplayInfo = json.ArrayOf("display");
        if (pArrDisplayInfo != NULL)
        {
            int nsize = pArrDisplayInfo->get_Size();
            for (int i = 0; i < nsize; i++)
            {
                pJsonObj = pArrDisplayInfo->ObjectAt(i);

                if (pJsonObj != NULL)
                {
                    pJsonObj->StringOf("name", ckstring);
                    if (strlen(ckstring.getString()) > 0)
                    {
                        PMonInfo pMonInfo = NULL;
                        pMonInfo = new MonInfo;

                        StringCchPrintf(pMonInfo->Device_Info.DeviceString, sizeof(pMonInfo->Device_Info.DeviceString), "%s", ckstring.getString());
                        pJsonObj->StringOf("id", ckstring);
                        StringCchPrintf(pMonInfo->Device_Info.DeviceID, sizeof(pMonInfo->Device_Info.DeviceID), "%s", ckstring.getString());
                        pJsonObj->StringOf("type", ckstring);
                        pMonInfo->nType = ckstring.intValue();
                        pJsonObj->StringOf("connectiontype", ckstring);
                        pMonInfo->nConnectionType = ckstring.intValue();
                        pJsonObj->StringOf("left", ckstring);
                        pMonInfo->rect.left = ckstring.intValue();
                        pJsonObj->StringOf("right", ckstring);
                        pMonInfo->rect.right = ckstring.intValue();
                        pJsonObj->StringOf("bottom", ckstring);
                        pMonInfo->rect.bottom = ckstring.intValue();
                        pJsonObj->StringOf("top", ckstring);
                        pMonInfo->rect.top = ckstring.intValue();
                        pJsonObj->StringOf("resolution", ckstring);
                        StringCchPrintf(pMonInfo->szResoulution, sizeof(pMonInfo->szResoulution), "%s", ckstring.getString());
                        pJsonObj->StringOf("primary", ckstring);
                        pMonInfo->bPrimary = ckstring.intValue();
                        pJsonObj->StringOf("index", ckstring);
                        StringCchPrintf(pMonInfo->szIndex, sizeof(pMonInfo->szIndex), "%s", ckstring.getString());
                        pJsonObj->StringOf("producingdate", ckstring);
                        StringCchPrintf(pMonInfo->szProducingDate, sizeof(pMonInfo->szProducingDate), "%s", ckstring.getString());

                        if (pMonInfo != NULL)
                        {
                            m_arrMonInfo.Add(pMonInfo);
                        }
                    }
                }

                delete pJsonObj;
                pJsonObj = NULL;
            }
        }
    }

    ArrayMonInfo arrMonInfonew;
    ArrayMonInfo arrMonTemp;
    //���� ����� ���� ���� ��������
    GetMonSettings(&arrMonInfonew);

    //ù��° ����Ʈ(���� ������ ����Ʈ)
    for (int i = 0; i < arrMonInfonew.GetCount(); i++)
    {
        PMonInfo pMonInfo1 = arrMonInfonew.GetAt(i);
        //�ι�° ����Ʈ(������ ������ ����Ʈ)
        for (int j = 0; j < m_arrMonInfo.GetCount(); j++)
        {
            PMonInfo pMonInfo2 = m_arrMonInfo.GetAt(j);
            //���� ������ �׸� ���� �׸��� ������ ���� �׸� ��, ã������ �� �����ʿ� ����
            if (strstr(pMonInfo1->Device_Info.DeviceID, pMonInfo2->Device_Info.DeviceID) != NULL)
            {
                if (pMonInfo1->nConnectionType != pMonInfo2->nConnectionType)
                {
                    //������ �ִ� �׸��ε� ����, ���� �̺�Ʈ ���� ���� ���̺� ������ �ٸ����� ��������.
                }
                if (pMonInfo1->nType == DISPLAY_COPY)
                {
                    if (pMonInfo2->nType == DISPLAY_COPY)
                    {
                        //���絵 ���� ���ŵ� ������ ���ƿ°� �ػ� ���� Ȯ�� �ʿ�
                        if (pMonInfo1->rect.left != pMonInfo2->rect.left ||    //�ػ󵵰� ����Ǿ����� Ÿ�� ����
                            pMonInfo1->rect.right != pMonInfo2->rect.right ||
                            pMonInfo1->rect.bottom != pMonInfo2->rect.bottom ||
                            pMonInfo1->rect.top != pMonInfo2->rect.top)
                        {
                            pMonInfo1->nType = RESOLUTION_CHANGED;
                            printf("�ػ� ���� %s : %d, %d, %d, %d\t->\t", pMonInfo2->Device_Info.DeviceID, pMonInfo2->rect.left, pMonInfo2->rect.right, pMonInfo2->rect.top, pMonInfo2->rect.bottom);
                            printf("%s : %d, %d, %d, %d\n", pMonInfo1->Device_Info.DeviceID, pMonInfo1->rect.left, pMonInfo1->rect.right, pMonInfo1->rect.top, pMonInfo1->rect.bottom);
                        }
                    }
                }
                else
                {
                    if (strstr(pMonInfo2->szIndex, "|") != NULL)
                    {
                        //������ �ٲ��
                        //���÷��� Ȯ�� ���� ��
                        pMonInfo1->nType = DISPLAY_EXPANSION;
                    }
                    else
                    {
                        if (pMonInfo1->rect.left != pMonInfo2->rect.left ||    //�ػ󵵰� ����Ǿ����� Ÿ�� ����
                            pMonInfo1->rect.right != pMonInfo2->rect.right ||
                            pMonInfo1->rect.bottom != pMonInfo2->rect.bottom ||
                            pMonInfo1->rect.top != pMonInfo2->rect.top)
                        {
                            pMonInfo1->nType = RESOLUTION_CHANGED;
                            printf("�ػ� ���� %s : %d, %d, %d, %d\t->\t", pMonInfo2->Device_Info.DeviceID, pMonInfo2->rect.left, pMonInfo2->rect.right, pMonInfo2->rect.top, pMonInfo2->rect.bottom);
                            printf("%s : %d, %d, %d, %d\n", pMonInfo1->Device_Info.DeviceID, pMonInfo1->rect.left, pMonInfo1->rect.right, pMonInfo1->rect.top, pMonInfo1->rect.bottom);
                        }
                    }
                }

                arrMonTemp.Add(pMonInfo1);
                arrMonInfonew.RemoveAt(i);
                i--;
                if (pMonInfo2 != NULL)
                {
                    delete pMonInfo2;
                    pMonInfo2 = NULL;
                }
                m_arrMonInfo.RemoveAt(j);
                j--;
            }
        }
    }

    int nArrSize1 = arrMonInfonew.GetSize();
    int nArrSize2 = m_arrMonInfo.GetSize();
    //�� ã�Ҵµ� ������, �����ִ� ����Ͱ� ������
    //���� ����Ʈ�� 0�� �ƴϸ� ����Ȱ� �ִ°�
    if (nArrSize1 != 0)
    {
        for (int i = 0; i < arrMonInfonew.GetSize(); i++)
        {
            PMonInfo pMonInfo = arrMonInfonew.GetAt(i);
            pMonInfo->nType = CABLE_CONNECTED;
            printf("%s �����, ���̺� Ÿ�� : %d\n", pMonInfo->Device_Info.DeviceID, pMonInfo->nConnectionType);

            arrMonInfonew.RemoveAt(i);
            i--;
            arrMonTemp.Add(pMonInfo);
        }
    }

    //���� ����� ����Ʈ�� 0�� �ƴϸ� �����Ȱ� �ִ°�
    if (nArrSize2 != 0)
    {
        for (int i = 0; i < m_arrMonInfo.GetSize(); i++)
        {
            //���� ����Ʈ ũ�Ⱑ �� ũ�� �����Ȱ� �����ȰŸ� ���� ����Ʈ���� ������ ���� ����Ʈ���� ������.
            PMonInfo pMonInfo = m_arrMonInfo.GetAt(i);
            pMonInfo->nType = CABLE_DISCONNECTED;
            printf("%s ����������\n", pMonInfo->Device_Info.DeviceID);

            m_arrMonInfo.RemoveAt(i);
            i--;
            arrMonTemp.Add(pMonInfo);
        }
    }

    SendDisplayInfo(&arrMonTemp);
    m_mtxAlert.lock();
    RemoveMonData(&m_arrMonInfo);
    //���� ������ �����ϰ� �������� �߰�
    int nsize = arrMonTemp.GetSize();
    for (int i = 0; i < nsize; i++)
    {
        PMonInfo pMonInfo = NULL;
        pMonInfo = arrMonTemp.GetAt(i);
        if (pMonInfo != NULL)
        {
            if (pMonInfo->nType == CABLE_DISCONNECTED)
            {
                delete pMonInfo;
                pMonInfo = NULL;
                arrMonTemp.RemoveAt(i);
                i--;
                nsize--;
            }
            else
            {
                m_arrMonInfo.Add(pMonInfo);
            }
        }
    }

    

    m_mtxAlert.unlock();
}


BOOL CMonitor::Get_Display_ConnectionType(char* szDeviceName, int& nConnectionType)
{
    BOOL bRet = FALSE;
    HRESULT hRet = S_OK;
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject* pWCObject = NULL;

    CString strTemp = _T("");

    HRESULT hr = ::CoInitialize(NULL);

    hRet = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (SUCCEEDED(hRet))
    {
        hRet = pLoc->ConnectServer((BSTR)L"ROOT\\WMI", NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
        if (SUCCEEDED(hRet))
        {
            hRet = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
            if (SUCCEEDED(hRet))
            {
                hRet = pSvc->ExecQuery((BSTR)L"WQL", (BSTR)L"SELECT InstanceName, VideoOutputTechnology FROM WmiMonitorConnectionParams", WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnumerator);
                if (SUCCEEDED(hRet))
                {
                    ULONG puReturn = 0;
                    while (pEnumerator && bRet == FALSE)
                    {
                        pEnumerator->Next(WBEM_INFINITE, 1, &pWCObject, &puReturn);

                        if (puReturn <= 0) break;	//Instance ������ break

                        if (pWCObject != NULL)
                        {
                            VARIANT vtProp1;
                            if (SUCCEEDED(pWCObject->Get(L"InstanceName", 0, &vtProp1, 0, 0)))
                            {
                                if (V_VT(&vtProp1) & VT_BSTR)
                                {
                                    strTemp = vtProp1.bstrVal;
                                    strTemp.MakeUpper();
                                    if (strstr(strTemp.GetBuffer(), strupr(szDeviceName)) != NULL)
                                    {
                                        VARIANT vtProp2;
                                        if (SUCCEEDED(pWCObject->Get(L"VideoOutputTechnology", 0, &vtProp2, 0, 0)))
                                        {
                                            if (V_VT(&vtProp2) & VT_I4)
                                            {
                                                nConnectionType = vtProp2.intVal;
                                                bRet = TRUE;
                                            }
                                            VariantClear(&vtProp2);
                                        }
                                    }
                                }
                                VariantClear(&vtProp1);
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

    CoUninitialize();

    return bRet;
}

BOOL CMonitor::Get_Display_ProductingDate(char* szDeviceName, char* szProducingDate)
{
    BOOL bRet = FALSE;
    CString strProducingDate = _T("");
    HRESULT hRet = S_OK;
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject* pWCObject = NULL;
    CString strTemp = _T("");

    hRet = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (SUCCEEDED(hRet))
    {
        hRet = pLoc->ConnectServer((BSTR)L"ROOT\\WMI", NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
        if (SUCCEEDED(hRet))
        {
            hRet = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
            if (SUCCEEDED(hRet))
            {
                hRet = pSvc->ExecQuery((BSTR)L"WQL", (BSTR)L"SELECT InstanceName, WeekOfManufacture, YearOfManufacture FROM WmiMonitorID", WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnumerator);
                if (SUCCEEDED(hRet))
                {
                    ULONG puReturn = 0;
                    while (pEnumerator && bRet == FALSE)
                    {
                        pEnumerator->Next(WBEM_INFINITE, 1, &pWCObject, &puReturn);

                        if (puReturn <= 0) break;	//Instance ������ break

                        if (pWCObject != NULL)
                        {
                            VARIANT vtProp1;

                            if (SUCCEEDED(pWCObject->Get(L"InstanceName", 0, &vtProp1, 0, 0)))
                            {
                                if (V_VT(&vtProp1) & VT_BSTR)
                                {
                                    strTemp = vtProp1.bstrVal;
                                    strTemp.MakeUpper();
                                    if (strstr(strTemp.GetBuffer(), strupr(szDeviceName)) != NULL)
                                    {
                                        VARIANT vtProp2;
                                        if (SUCCEEDED(pWCObject->Get(L"YearOfManufacture", 0, &vtProp2, 0, 0)))
                                        {
                                            if (V_VT(&vtProp2) & VT_I4)
                                            {
                                                strProducingDate.Format("%d/", vtProp2.intVal);
                                            }
                                            VariantClear(&vtProp2);
                                        }

                                        VARIANT vtProp3;
                                        if (SUCCEEDED(pWCObject->Get(L"WeekOfManufacture", 0, &vtProp3, 0, 0)))
                                        {
                                            if (V_VT(&vtProp3) & VT_UI1)
                                            {
                                                strProducingDate.AppendFormat("%d", vtProp3.intVal);
                                            }
                                            VariantClear(&vtProp3);
                                        }

                                        if (strlen(strProducingDate) == strlen("0000/00"))
                                        {
                                            StringCchPrintf(szProducingDate, 8, "%s", strProducingDate);
                                            bRet = TRUE;
                                        }
                                    }
                                }
                                VariantClear(&vtProp1);
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

BOOL CMonitor::Get_VideoController_Status(VideoController* videocontroller)
{
    BOOL bRet = FALSE;
    HRESULT hRet = S_OK;
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject* pWCObject = NULL;


    hRet = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (SUCCEEDED(hRet))
    {
        hRet = pLoc->ConnectServer((BSTR)L"ROOT\\CIMV2", NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
        if (SUCCEEDED(hRet))
        {
            hRet = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
            if (SUCCEEDED(hRet))
            {
                hRet = pSvc->ExecQuery((BSTR)L"WQL", (BSTR)L"SELECT Caption, Status FROM Win32_VideoController", WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnumerator);
                if (SUCCEEDED(hRet))
                {
                    ULONG puReturn = 0;
                    while (pEnumerator && bRet == FALSE)
                    {
                        pEnumerator->Next(WBEM_INFINITE, 1, &pWCObject, &puReturn);

                        if (puReturn <= 0) break;	//Instance ������ break

                        if (pWCObject != NULL)
                        {
                            VARIANT vtProp;

                            if (SUCCEEDED(pWCObject->Get(L"Caption", 0, &vtProp, 0, 0)))
                            {
                                if (V_VT(&vtProp) & VT_BSTR)
                                {
                                    StringCchPrintf(videocontroller->szVideoController, sizeof(videocontroller->szVideoController), CString(vtProp.bstrVal));
                                }
                                VariantClear(&vtProp);
                            }
                            if (SUCCEEDED(pWCObject->Get(L"Status", 0, &vtProp, 0, 0)))
                            {
                                if (V_VT(&vtProp) & VT_BSTR)
                                {
                                    StringCchPrintf(videocontroller->szStatus, sizeof(videocontroller->szStatus), CString(vtProp.bstrVal));
                                }
                                VariantClear(&vtProp);
                            }
                            if (strlen(videocontroller->szVideoController) != 0 && strlen(videocontroller->szStatus) != 0)
                            {
                                bRet = TRUE;
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



BOOL CALLBACK CMonitor::Monitorenumproc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM lparam)
{
    DWORD dwPhysicalMonitor = 0;

    //�������Ϳ� ���� ���� ����� ���� Ȯ��
    GetNumberOfPhysicalMonitorsFromHMONITOR(hMonitor, &dwPhysicalMonitor);

    //�������Ϳ� ���� ��������� ������ 2�� �̻��̸�
    if (dwPhysicalMonitor >= 2)
    {
        ArrayMonInfo* m_pArrMonInfo = (ArrayMonInfo*)lparam;
        PMonInfo pMonInfo = new MonInfo;
        pMonInfo->hMonitor = hMonitor;

        //��ǥ ��������
        MONITORINFOEX MonInfoex;
        MonInfoex.cbSize = sizeof(MonInfoex);
        GetMonitorInfo(hMonitor, &MonInfoex);

        //���÷��� ��ǥ 
        pMonInfo->rect = MonInfoex.rcMonitor;
        StringCchPrintf(pMonInfo->szResoulution, sizeof(pMonInfo->szResoulution), "%dX%d", MonInfoex.rcMonitor.right - MonInfoex.rcMonitor.left, MonInfoex.rcMonitor.bottom - MonInfoex.rcMonitor.top);
        pMonInfo->nType = FIRST_COLLECT;

        //�̸����� Device ID ������
        DISPLAY_DEVICE pdd;
        pdd.cb = sizeof(DISPLAY_DEVICE);
        EnumDisplayDevices(MonInfoex.szDevice, NULL, &pdd, EDD_GET_DEVICE_INTERFACE_NAME);
        pMonInfo->Device_Info = pdd;
        
        StringCchPrintf(pMonInfo->szIndex, sizeof(pMonInfo->szIndex), "%s", "copy");

        m_pArrMonInfo->Add(pMonInfo);
    }
    else
    {
        PMonInfo pMonInfo = new MonInfo;
        ArrayMonInfo* m_pArrMonInfo = (ArrayMonInfo*)lparam;
        DWORD dwPhysicalMonitor = 0;
        pMonInfo->hMonitor = hMonitor;

        //����� �ڵ�� �ش��ϴ� �̸� ������
        MONITORINFOEX MonInfoex;
        MonInfoex.cbSize = sizeof(MonInfoex);
        GetMonitorInfo(hMonitor, &MonInfoex);

        //���÷��� ��ǥ 
        pMonInfo->rect = MonInfoex.rcMonitor;
        StringCchPrintf(pMonInfo->szResoulution, sizeof(pMonInfo->szResoulution), "%dX%d", MonInfoex.rcMonitor.right - MonInfoex.rcMonitor.left, MonInfoex.rcMonitor.bottom - MonInfoex.rcMonitor.top);
        pMonInfo->nType = FIRST_COLLECT;

        //�̸����� Device ID ������
        DISPLAY_DEVICE pdd;
        pdd.cb = sizeof(DISPLAY_DEVICE);
        EnumDisplayDevices(MonInfoex.szDevice, NULL, &pdd, EDD_GET_DEVICE_INTERFACE_NAME);
        pMonInfo->Device_Info = pdd;

        CString strID = pdd.DeviceID;
        strID = strID.Mid(4, (strID.ReverseFind('#')) - 4);
        strID.Replace('#', '\\');
        StringCchPrintf(pMonInfo->Device_Info.DeviceID, sizeof(pMonInfo->Device_Info.DeviceID), strID);

        //����� �ε��� ���ϱ�
        CString strIdx = pMonInfo->Device_Info.DeviceName;
        strIdx = strIdx.Mid(4, (strIdx.ReverseFind('\\')) - 4);
        StringCchPrintf(pMonInfo->szIndex, sizeof(pMonInfo->szIndex), strIdx);

        m_pArrMonInfo->Add(pMonInfo);
    }
    

    return TRUE;
}

BOOL CMonitor::GetMonitorWithWMI(ArrayMonInfo* pArrpMonInfo)
{
    BOOL bRet = FALSE;
    HRESULT hRet = S_OK;
    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject* pWCObject = NULL;
    CString strData = _T("");

    hRet = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (void**)&pLoc);
    if (SUCCEEDED(hRet))
    {
        hRet = pLoc->ConnectServer((BSTR)L"ROOT\\CIMV2", NULL, NULL, NULL, 0, NULL, NULL, &pSvc);
        if (SUCCEEDED(hRet))
        {
            hRet = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
            if (SUCCEEDED(hRet))
            {
                hRet = pSvc->ExecQuery((BSTR)L"WQL", (BSTR)L"SELECT DeviceID, Name FROM Win32_PnpEntity WHERE Service=\"monitor\"", WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 0, &pEnumerator);
                if (SUCCEEDED(hRet))
                {
                    ULONG puReturn = 0;
                    while (pEnumerator && bRet == FALSE)
                    {
                        PMonInfo pMonInfo = new MonInfo;
                        pEnumerator->Next(WBEM_INFINITE, 1, &pWCObject, &puReturn);

                        if (puReturn <= 0) break;	//Instance ������ break

                        if (pWCObject != NULL)
                        {
                            VARIANT vtProp1;

                            if (SUCCEEDED(pWCObject->Get(L"Name", 0, &vtProp1, 0, 0)))
                            {
                                if (V_VT(&vtProp1) & VT_BSTR)
                                {
                                    strData = vtProp1.bstrVal;
                                    StringCchPrintf(pMonInfo->Device_Info.DeviceString, sizeof(pMonInfo->Device_Info.DeviceString), "%s", strData);
                                }
                                VariantClear(&vtProp1);
                            }

                            VARIANT vtProp2;
                            if (SUCCEEDED(pWCObject->Get(L"DeviceID", 0, &vtProp2, 0, 0)))
                            {
                                if (V_VT(&vtProp2) & VT_BSTR)
                                {
                                    strData = vtProp2.bstrVal;
                                    StringCchPrintf(pMonInfo->Device_Info.DeviceID, sizeof(pMonInfo->Device_Info.DeviceID), "%s", strData);
                                }
                                VariantClear(&vtProp2);
                            }
                            pWCObject->Release();
                            pWCObject = NULL;
                        }

                        if (pMonInfo != NULL)
                        {
                            pArrpMonInfo->Add(pMonInfo);
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