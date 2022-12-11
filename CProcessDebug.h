#pragma once
#include "Thread.h"
#include "CProcessDebugSubThread.h"

#include <mutex>
class CProcessDebugHandler : public CThread
{
public:
	CProcessDebugHandler(void);
	~CProcessDebugHandler(void);

public:
	virtual DWORD Run();
	void SetEndAndWait();

	void SetList(CArray<char*, char*> *ArrayProcList);
private:
	BOOL WMI_GetProcessID(std::map<DWORD, char*>* map_Proc);
	void DeleteProcList();
	void DeleteSubHandler(std::map<DWORD, char*> &map_Proc);
	void DeleteAllSubHandler();
private:
	CWEvent m_ExitEvent;
	BOOL	m_bStop;
	CWEvent m_evtWait;
	std::map<DWORD, PVOID> m_map;
	std::mutex	m_mtx;
	CArray<char*, char*>* m_pArrayProcList;

};