#ifndef __m_cltabs_h__
#define __m_cltabs_h__

//////////////////////////////////////////////////
// API file for creation custom tabs.
// see std_tabs.cpp for an example
//////////////////////////////////////////////////

//////////////////////////////////////////////////
// Create your custom tabs during this event
// lParam = (LPARAM)(HWND)hParentWnd
#define ME_CLTABS_CREATETABS "CLTabs/CreateTabs"

//////////////////////////////////////////////////
// PostMessage(GetParent(hwnd), UM_CLTABS_UPDATE, 0, 0)
// to refresh tab list in options
#define UM_CLTABS_UPDATE WM_USER+10

//////////////////////////////////////////////////
// Creates new tab
// wParam = (WPARAM)(LPCLTABINFO)&tabInfo
// laram = 0
struct tagCLTABINFO;

// chesk if passed contact is present on the tab
#define CLTABS_REASON_CHECK   1
// initilize tab. not implemented yet
#define CLTABS_REASON_INIT    2
// clean all tab data
#define CLTABS_REASON_CLEANUP 3
// delete tab (so it no longer appears in tab list)
#define CLTABS_REASON_DELETE  4

// return TRUE if hContact is present on this tab
typedef BOOL (__cdecl *TABFUNC)(HANDLE hContact, struct tagCLTABINFO *tabInfo, char reason);

#define CLTABS_CLIST     1
#define CLTABS_CUSTOM    2
#define CLTABS_TYPE      0x0f
#define CLTABS_CANDELETE 0x80

typedef struct tagCLTABINFO
{
	int cbSize;

	BYTE tabType; // CLTABS_CLIST or CLTABS_CUSTOM
	LPSTR lpzId; // This is supposed to be unique so use syntax similar to 
	HINSTANCE hInstance;

	// Tab look
	LPSTR lpzTitle; // NULL means no text
	HICON hIcon; // NULL means no icon

	// Tab options. 200x225 pixels
	LPSTR pszCfgTemplate; // NULL means no options
	DLGPROC pfnCfgDlgProc; // can be NULL

	TABFUNC func; // For non-clist tabs it is still used for INIT and CLEANUP
	HWND hwndTab; // not NULL if type is CLTABS_CUSTOM

	// Plugin data
	LPVOID data;
} CLTABINFO, *LPCLTABINFO;

#define MS_CLTABS_CREATETAB "CLTabs/CreateTab"

//////////////////////////////////////////////////
// Hook this event to add you types into
// "New Tab..." dialog.
// To add tab type call
// PostMessage((HWND)lParam, UM_ADDTYPE, (WPARAM)"type name", (LPARAM)"creation service");
// Tab creation recieves new tab title via lParam

#define UM_ADDTYPE WM_USER+100
#define CLTABS_ADDTYPE(name, svc) \
	PostMessage((HWND)lParam, UM_ADDTYPE, (WPARAM)(name), (LPARAM)(svc))

// lParam = hwnd of window
#define ME_CLTABS_LISTTYPES "CLTabs/ListTypes"

#endif // __m_cltabs_h__