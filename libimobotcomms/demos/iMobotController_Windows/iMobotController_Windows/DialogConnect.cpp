// DialogConnect.cpp : implementation file
//

#include "stdafx.h"
#include "iMobotController_Windows.h"
#include "DialogConnect.h"


// CDialogConnect dialog

IMPLEMENT_DYNAMIC(CDialogConnect, CDialog)

CDialogConnect::CDialogConnect(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogConnect::IDD, pParent)
{
  char path[MAX_PATH];
  /* Read the config file */
  if(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, path) != S_OK) 
  {
    /* Could not get the user's app data directory */
  } else {
    //MessageBox((LPCTSTR)path, (LPCTSTR)"Test");
    //fprintf(fp, "%s", path); 
  }
  strcat(path, "\\Barobo.config");
  m_robotManager.read(path);
}

CDialogConnect::~CDialogConnect()
{
}

BOOL CDialogConnect::OnInitDialog()
{
  CDialog::OnInitDialog();
  return TRUE;
}

void CDialogConnect::refreshLists()
{
	int i;
	USES_CONVERSION;
	/* Clear the list controls */
	m_listCtrl_availableBots.DeleteAllItems();
	/* Populate available bots listctrl */
	for(i = 0; i < m_robotManager.numEntries(); i++) {
		if(!m_robotManager.isConnected(i)) {
			m_listCtrl_availableBots.InsertItem(
				m_listCtrl_availableBots.GetItemCount(),
				A2T(m_robotManager.getEntry(i))
				);
		}
	}
	/* Populate connected bots listctrl */
	m_listCtrl_connectedBots.DeleteAllItems();
	for(i = 0; i < m_robotManager.numConnected(); i++) {
		m_listCtrl_connectedBots.InsertItem(
			i,
			A2T(m_robotManager.getConnected(i))
			);
	}
}

void CDialogConnect::DoDataExchange(CDataExchange* pDX)
{
  CDialog::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_LIST_AVAILABLEBOTS, m_listCtrl_availableBots);
  DDX_Control(pDX, IDC_LIST_CONNECTEDBOTS, m_listCtrl_connectedBots);

  /* Set up list controls */
  m_listCtrl_availableBots.InsertColumn(
    0, 
    TEXT("Mobot Address"),
    LVCFMT_LEFT,
    120,
    -1);
  m_listCtrl_connectedBots.InsertColumn(
    0,
    TEXT("Mobot Address"),
    LVCFMT_LEFT,
    120,
    -1);
  refreshLists();
  DDX_Control(pDX, IDC_EDIT_ROBOTADDRESS, m_edit_newRobotAddress);
}


BEGIN_MESSAGE_MAP(CDialogConnect, CDialog)
  ON_BN_CLICKED(IDC_BUTTON_TEACHING_CONNECT, &CDialogConnect::OnBnClickedButtonTeachingConnect)
  ON_BN_CLICKED(IDC_BUTTON_TEACHING_DISCONNECT, &CDialogConnect::OnBnClickedButtonTeachingDisconnect)
  ON_BN_CLICKED(IDC_BUTTON_TEACHING_MOVEUP, &CDialogConnect::OnBnClickedButtonTeachingMoveup)
  ON_BN_CLICKED(IDC_BUTTON_TEACHING_MOVEDOWN, &CDialogConnect::OnBnClickedButtonTeachingMovedown)
  ON_BN_CLICKED(IDC_BUTTON_ADDNEWBOT, &CDialogConnect::OnBnClickedButtonAddnewbot)
END_MESSAGE_MAP()


// CDialogConnect message handlers

void CDialogConnect::OnBnClickedButtonTeachingConnect()
{
	/* Get the selected item */
	int index;
	index = m_listCtrl_availableBots.GetSelectionMark();
	if(index == -1) {
		return;
	}
	m_robotManager.connect(index);
	refreshLists();
}

void CDialogConnect::OnBnClickedButtonTeachingDisconnect()
{
	int connectIndex;
	connectIndex = m_listCtrl_connectedBots.GetSelectionMark();
	if(connectIndex == -1) { return; }
	m_robotManager.disconnect(connectIndex);
	refreshLists();
}

void CDialogConnect::OnBnClickedButtonTeachingMoveup()
{
  // TODO: Add your control notification handler code here
}

void CDialogConnect::OnBnClickedButtonTeachingMovedown()
{
  // TODO: Add your control notification handler code here
}

void CDialogConnect::OnBnClickedButtonAddnewbot()
{
  /* Get the text from the correct edit and put it in the config file */
  USES_CONVERSION;
  TCHAR text[256];
  LPCSTR address;
  int count;
  memset(text, 0, sizeof(TCHAR)*256);
  ((DWORD*)text)[0] = 256;
  count = m_edit_newRobotAddress.GetLine(0, text, 255);
  text[count] = (TCHAR)'\0';
  /* Convert from wide char to normal char */
  //size_t convertedChars = 0;
  //size_t origsize = wcslen(text)+1;
  //wcstombs_s(&convertedChars, address, origsize, text, _TRUNCATE);
  address = T2CA(text);
  m_robotManager.addEntry(address);
  refreshLists();
}
