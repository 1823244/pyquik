#include "serv.h"
#include "table.h"
#include "market.h"

enum ddt {tdtFloat=1, tdtString, tdtBool, tdtError, tdtBlank, tdtInt, tdtSkip, tdtTable = 16}; 

CMarketServer* g_pServer = 0;

HDDEDATA CALLBACK DDE_Callback(UINT uType, UINT uFmt, HCONV hConv, HSZ hsz1, HSZ hsz2, HDDEDATA hData, DWORD dwData1, DWORD dwData2)
{
	switch (uType)
	{
        case XTYP_CONNECT:
        {
            return (HDDEDATA)TRUE;
        }

	    case XTYP_POKE:	
		{
			BOOL flag = FALSE;
			CHAR buf[200];
			//flag = server.xltable.GetData(hData);
			
			if (!flag)
			{
				DdeQueryStringA(g_pServer->m_dwInst,hsz1,buf,200,CP_WINANSI);
				//server.xltable.GetBuf(buf);
	            return((HDDEDATA)DDE_FACK); 
			}
			return (HDDEDATA)TRUE;
	    }

	    case XTYP_DISCONNECT:
	    {
			break;
	    }

	    case XTYP_ERROR:
	    {
            break;
        }
    }
    return((HDDEDATA)NULL);
}

/*	Table table;
	g_pServer->ParseData( table, (PBYTE)pData, dwSize );
	for( MarketListeners::const_iterator it = m_listeners.begin(); it != m_listeners.end(); it++)
		(*it)->onTableData( pszTopic, pszItem, &table );
	return TRUE;*/

CMarketServer::CMarketServer(std::string name) : m_strName( name )
{
    g_pServer = this;
}

CMarketServer::~CMarketServer()
{
}

bool CMarketServer::Connect()
{
	if(!RegisterClipboardFormatA((LPSTR)"XlTable")) return false;
	if(DdeInitialize(&m_dwInst, DDE_Callback, APPCLASS_STANDARD, 0)) return false;
	m_hszService = DdeCreateStringHandleA(m_dwInst, m_strName.c_str(), CP_WINANSI);
	if(m_hszService == 0) return false;
	if(!DdeNameService(m_dwInst, m_hszService, NULL, DNS_REGISTER)) return false;
	return true;
}

void CMarketServer::Disconnect()
{
	if (m_dwInst)
	{
		DdeNameService(m_dwInst, m_hszService, (HSZ)NULL, DNS_UNREGISTER);
		if(m_hszService) DdeFreeStringHandle(m_dwInst, m_hszService);
		DdeUninitialize(m_dwInst);
		m_dwInst = 0;
	}
}

void CMarketServer::OnConnected()
{
	for( MarketListeners::const_iterator it = m_listeners.begin(); it != m_listeners.end(); it++)
		(*it)->onConnected( );
}

void CMarketServer::OnDisconnected()
{
	for( MarketListeners::const_iterator it = m_listeners.begin(); it != m_listeners.end(); it++)
		(*it)->onDisconnected( );
}


void CMarketServer::OnTransactionResult(long nTransactionResult, long nTransactionExtendedErrorCode, long nTransactionReplyCode, 
                                        unsigned long dwTransId, double dOrderNum, const char* lpcstrTransactionReplyMessage)
{
	for( MarketListeners::const_iterator it = m_listeners.begin(); it != m_listeners.end(); it++)
		(*it)->onTransactionResult( nTransactionResult, nTransactionExtendedErrorCode, 
                                    nTransactionReplyCode, dwTransId, dOrderNum, cp1251_to_utf8(lpcstrTransactionReplyMessage) );
}


BOOL CMarketServer::ParseData(Table& table, PBYTE data, DWORD length)
{
	 WORD row_n	= 0;
	 WORD col_n = 0;
     UINT offset  = 0;                       // ������� � ������� ������
	 WORD cnum = 0, rnum = 0;                // ���������� ��� �������� ����� � �������� �������
	 WORD cb = 0;                            // ���������� ��� �������� cb
	 WORD size = 0;                          // ���������� ��� �������� ������� ������
	 WORD type = 0;                          // ���������� ��� �������� ���� ������
	 string str;
          
     union                                   // ����������� ��� �������������� ������ � ��������� ����
     {
           WORD w; 
           double d; 
           BYTE b[8];
     } conv;  

     if (length < 8) return 1;               


     // �������, ����� ������ ���� ��� tdtTable
     conv.b[0] = data[offset++];
     conv.b[1] = data[offset++];
	 if (conv.w != tdtTable) return 1;
   
     // �������, ����� cb ��������� 4
     conv.b[0] = data[offset++];
     conv.b[1] = data[offset++];
	 if (conv.w != 4) return 1;

     // �������� ���������� ����� �������
     conv.b[0] = data[offset++];
     conv.b[1] = data[offset++];
     row_n = conv.w;

     // �������� ���������� �������� �������
     conv.b[0] = data[offset++];
     conv.b[1] = data[offset++];
     col_n = conv.w;

	 if (!col_n || !row_n)                       // ���� ���-�� ����� ��� �������� ����� ����, �� ������
     {
         return 1;
     }    

	 table.init(row_n, col_n);
  
     // ��������� ��� ������� �������
	 while (offset < length)
	 {
         // ��������� ��� ������  
		 conv.b[0] = data[offset++];
	     conv.b[1] = data[offset++];
		 type = conv.w;

		 switch(type)
		 {
		 case tdtFloat:                      // ������ ������ ���� Double
			 {
	 			conv.b[0] = data[offset++];
				conv.b[1] = data[offset++];
				cb = conv.w;
				if (cb%8) 
				{
					return 1;
				}
				for (int tmp=0, i=0; i < cb; i++, tmp++)
				{
					conv.b[tmp] = data[offset++];
					if (tmp == 7)  // ������� ���� �����
					{
						 tmp = -1;
						 /*
						 str = DoubleToString(conv.d);
						 if (!str.compare("")) 
						 {
							 return 1;
						 }
						 */
						 table.setString(rnum,cnum,"DOUBLE");
						 table.setDouble(rnum,cnum,conv.d);
						 cnum++;
						 if (cnum == table.cols())
						 {
							 cnum = 0;
							 rnum++;
						 }
					 }
				}
			 }
			 break;

		 case tdtString:                     // ������ ������ ���� String
			 {
                 // ��������� cb
			     conv.b[0] = data[offset++];
			     conv.b[1] = data[offset++];
			     cb = conv.w;

        		 for (WORD i = 0; i < cb; )
			     {	 // ��������� ����� ������ (��� ��� ������������ ����)
				    size = data[offset++];
					
					string s;
				    // ��������� ������ ��������� 
				    for(int j = 0; j < size; j++)
                        s.push_back(data[offset++]);
					table.setString(rnum,cnum, s.c_str() );
				    i += 1 + size;
				    cnum++;
				    if (cnum == table.cols())
				    {
					   cnum = 0;
					   rnum++;
			        }
                 }
			 }
			 break;

		 case tdtBool:                       // ������ ������ ���� Bool
			 {
			     conv.b[0] = data[offset++];
			     conv.b[1] = data[offset++];
			     cb = conv.w;
			     if (cb%2) 
			     {
					 table.reset();
 				     return 1;
		         }
			     for (int tmp=0, i=0; i < cb; i++, tmp++)
			     {
				     conv.b[tmp] = data[offset++];
				     if (tmp == 1)  // ������� ���� �����
				     {
					    tmp = -1;
						table.setString(rnum,cnum,"BOOL");
						if (conv.w) table.setDouble(rnum,cnum,1);
					    else table.setDouble(rnum,cnum,0);
					    cnum++;
					    if (cnum == table.cols())
					    {
						   cnum = 0;
						   rnum++;
                        }
                     }
                 }
			 }
			 break;

		 case tdtError:                      // ������ ������ ���� Error
			 {
                 conv.b[0] = data[offset++];
			     conv.b[1] = data[offset++];
			     cb = conv.w;
			     if (cb%2) 
			     {
				     table.reset();
				     return 1;
                 }
			     for (int tmp=0, i=0; i < cb; i++, tmp++)
			     {
				     conv.b[tmp] = data[offset++];
				     if (tmp == 1)  // ������� ���� �����
				     {
					    tmp = -1;
					    table.setString(rnum,cnum,"ERROR");
					    cnum++;
					    if (cnum == table.cols())
					    {
						    cnum = 0;
						    rnum++;
				        }
				     }
		         }
			 }
			 break;

		 case tdtBlank:                      // ������ ������ ���� Blank
			 {
                 conv.b[0] = data[offset++];
			     conv.b[1] = data[offset++];
			     cb = conv.w;
			     if (cb != 2)
			     {
				     table.reset();
				     return 1;
		         }
			     conv.b[0] = data[offset++];
			     conv.b[1] = data[offset++];
			     size = conv.w;
			     for (int i = 0; i<size; i++)
			     {
				     table.setString(rnum,cnum, "NULL");
				     cnum++;
				     if (cnum == table.cols())
				     {
					     cnum = 0;
					     rnum++;
			         }
	             }
			 }
			 break;

		 case tdtInt:	                     // ������ ������ ���� Int
			 {
                 conv.b[0] = data[offset++];
			     conv.b[1] = data[offset++];
			     cb = conv.w;
			     if (cb%2) 
			     {
				     table.reset();
				     return 1;
		         }
			     for (int tmp=0, i=0; i < cb; i++, tmp++)
			     {
				     conv.b[tmp] = data[offset++];
				     if (tmp == 1)  // ������� ���� �����
				     {
					     tmp = -1;
						 //_itoa_s(conv.w,mas,10);
						 table.setString(rnum,cnum,"INT");
						 table.setDouble(rnum,cnum,conv.w);
					     cnum++;
					     if (cnum == table.cols())
					     {
						     cnum = 0;
						     rnum++;
				         }
		             }
			     }
			 }
			 break;

		 case tdtSkip:                       // �� ��������������
		     {
				 table.reset();
				 return 1;
             }
		 }
	 }

	 return 0;                               // ������� ��������� �������
}
