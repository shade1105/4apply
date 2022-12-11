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

    void GetMonSettings(ArrayMonInfo* m_parrMonInfo);                           //모니터 데이터 가져오기
    void RemoveMonData(ArrayMonInfo* m_parrMonInfo);                            //모니터 데이터 지우기
    void CheckMonChange(char* pszJsonPath = NULL);

    void SendDisplayInfo(ArrayMonInfo* parrMonInfo);

    BOOL Get_VideoController_Status(VideoController* videocontroller);          //그래픽 드라이버 이름, 상태

private:
    BOOL GetMonitorWithWMI(ArrayMonInfo* pArrpMonInfo);
    BOOL Get_Display_ConnectionType(char* szDeviceName, int& nConnectionType);  //모니터 연결 타입
    BOOL Get_Display_ProductingDate(char* szDeviceName, char* szProducingDate); //모니터 생산년도
    static BOOL CALLBACK Monitorenumproc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM unnamedParam4);  //모니터 정보 가져오는 함수(EnumMonitor 콜백)

private:
    wchar_t* ConvertCtoWC(char* str);
    

public:
    ArrayMonInfo m_arrMonInfo;
    

};