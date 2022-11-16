#pragma once
#include <afxwin.h>
class CMyEdit :
    public CEdit
{
public:
    DECLARE_MESSAGE_MAP()
    afx_msg void OnDropFiles(HDROP hDropInfo);
};

