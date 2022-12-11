#pragma once
#include <highlevelmonitorconfigurationapi.h>
#include <WbemIdl.h>
#include <winevt.h>

#pragma comment(lib,"Wevtapi")
#pragma comment(lib, "wbemuuid")
#pragma comment(lib, "Dxva2.lib")



typedef struct _VideoController {
    char szVideoController[128];
    char szStatus[64];
    _VideoController()
    {
        ZeroMemory(szVideoController, sizeof(szVideoController));
        ZeroMemory(szStatus, sizeof(szStatus));
    }
}VideoController, * PVideoController;


class CMonitor
{
public:
    CMonitor();
    ~CMonitor();

public:
    void Initialize();

    void GetMonSettings(ArrayMonInfo* m_parrMonInfo);                           //����� ������ ��������
    void RemoveMonData(ArrayMonInfo* m_parrMonInfo);                            //����� ������ �����
    void CheckMonChange(char* pszJsonPath = NULL);

    void SendDisplayInfo(ArrayMonInfo* parrMonInfo);

    BOOL Get_VideoController_Status(VideoController* videocontroller);          //�׷��� ����̹� �̸�, ����

private:
    BOOL GetMonitorWithWMI(ArrayMonInfo* pArrpMonInfo);
    BOOL Get_Display_ConnectionType(char* szDeviceName, int& nConnectionType);  //����� ���� Ÿ��
    BOOL Get_Display_ProductingDate(char* szDeviceName, char* szProducingDate); //����� ����⵵
    static BOOL CALLBACK Monitorenumproc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM unnamedParam4);  //����� ���� �������� �Լ�(EnumMonitor �ݹ�)

private:
    wchar_t* ConvertCtoWC(char* str);
    

public:
    ArrayMonInfo m_arrMonInfo;
    

};