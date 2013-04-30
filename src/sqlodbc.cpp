//
//  $Id: sqlodbc.cpp,v 1.17 2001/10/14 15:50:37 erngui Exp $
//
#include "sqlodbc.h"
#include <odbcinst.h>
#include <assert.h>
#include <ostream>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>

using namespace std;

namespace odbc_5189169245 {


SQLHENV DataSource::g_henv = SQL_NULL_HENV;
long DataSource::g_envCount = -1;

SqlStatement::SqlStatement(DataSource& db) 
  : m_dataSource(db), m_boundCols(0), m_hstmt(SQL_NULL_HSTMT)
{
    CheckStatus(::SQLAllocHandle(SQL_HANDLE_STMT, db.m_hdbc, 
        &m_hstmt));
}

SqlStatement::~SqlStatement()
{
    ::SQLFreeHandle(SQL_HANDLE_STMT, m_hstmt);
}

bool SqlStatement::IsValid() const
{
    return m_hstmt != SQL_NULL_HSTMT;
}

void SqlStatement::CheckStatus(RETCODE ret)
{
    m_dataSource.CheckStatus(ret, m_hstmt);
}

void DataSource::CheckStatus(RETCODE ret, HSTMT st)
{
    if (ret == SQL_SUCCESS_WITH_INFO) {
        TraceError(st);
    } else if (ret != SQL_SUCCESS) {
        ThrowError(st);
    }
}

void SqlStatement::Execute(const string& sql)
{
    assert(IsValid());
    CheckStatus(::SQLExecDirect(m_hstmt, (UCHAR*)sql.c_str(),
        SQL_NTS));
}

void SqlStatement::Tables(const string& type)
{
    assert(IsValid());
    CheckStatus(::SQLTables(m_hstmt, NULL, SQL_NTS,
        NULL, SQL_NTS,
        NULL, SQL_NTS,
        type.length() > 0? (SQLCHAR*)type.c_str() : NULL, 
        SQL_NTS));
}

void SqlStatement::Columns(const string& table)
{
    assert(IsValid());
    CheckStatus(::SQLColumns(m_hstmt, NULL, SQL_NTS,
        NULL, SQL_NTS,
        table.length() > 0? (SQLCHAR*)table.c_str() : NULL, 
        SQL_NTS,
        NULL, SQL_NTS));
}
    
void SqlStatement::DescribeCol(USHORT number, UCHAR *name,
       USHORT BufferLength, SHORT *NameLength,
       SHORT *DataType, ULONG *ColumnSize,
       SHORT *DecimalDigits, SHORT *Nullable)
{
    assert(IsValid());
    CheckStatus(::SQLDescribeCol(m_hstmt, number, name,
       BufferLength, NameLength,
       DataType, ColumnSize,
       DecimalDigits, Nullable));
}


SWORD SqlStatement::NumResultCols()
{
    assert(IsValid());
    SWORD cols;
    CheckStatus(::SQLNumResultCols(m_hstmt, &cols));
    return cols;
}

SDWORD SqlStatement::RowCount()
{
    assert(IsValid());
    SDWORD rows;
    CheckStatus(::SQLRowCount(m_hstmt, &rows));
    return rows;
}


bool SqlStatement::Next()
{
    assert(IsValid());
    RETCODE ret = ::SQLFetch(m_hstmt);
    if (ret == SQL_NO_DATA_FOUND)
        return false;
    CheckStatus(ret);
    return true;
}

void SqlStatement::GetData(WORD col, DWORD maxlen, void* data,
    SDWORD* len)
{
    assert(IsValid());
    CheckStatus(::SQLGetData(m_hstmt, col, SQL_C_DEFAULT,
        data, maxlen, len));
}

void SqlStatement::BindCol(void* rgbValue, SDWORD cbValueMax, 
    SDWORD* pcbValue, SQLSMALLINT TargetType)
{
    assert(IsValid());
    RETCODE ret = ::SQLBindCol(m_hstmt, m_boundCols+1, 
        TargetType, rgbValue, cbValueMax, pcbValue);
    CheckStatus(ret);
    m_boundCols++;
}

UWORD SqlStatement::NumBoundCols()
{
    return m_boundCols;
}


DataSource::DataSource() : m_hdbc(SQL_NULL_HDBC)
{    
}

void DataSource::Connect(const std::string& dsn, 
    const std::string& user, const std::string& pwd, 
    DWORD timeout)
{
    InitEnv();
    assert(g_henv != SQL_NULL_HENV);

    CheckStatus(::SQLAllocHandle(SQL_HANDLE_DBC, g_henv, 
        &m_hdbc));
    assert(m_hdbc != SQL_NULL_HDBC);

    if (timeout != INFINITE) {
        RETCODE ret = ::SQLSetConnectOption(m_hdbc, 
            SQL_LOGIN_TIMEOUT, timeout);
        if (!SQL_SUCCEEDED(ret))
            TraceError();
    }
    
    RETCODE ret = ::SQLConnect(m_hdbc, 
        (UCHAR*)dsn.c_str(),  SQL_NTS, 
        (UCHAR*)user.c_str(), SQL_NTS, 
        (UCHAR*)pwd.c_str(),  SQL_NTS);
    if (ret != SQL_SUCCESS) {
        if (ret != SQL_SUCCESS_WITH_INFO) {
            Exception ex = GetError();
            ::SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
            m_hdbc = SQL_NULL_HDBC;
            throw ex;
        } else {
            TraceError();
        }
    }
}

void DataSource::Disconnect()
{
    if (IsConnected()) {
        RETCODE ret1, ret2;
        ret1 = ::SQLDisconnect(m_hdbc);
        ret2 = ::SQLFreeHandle(SQL_HANDLE_DBC, m_hdbc);
        CheckStatus(ret1);
        CheckStatus(ret2);
        m_hdbc = SQL_NULL_HDBC;
    }
}


Exception DataSource::GetError(HSTMT hstmt)
{
    SWORD nOutlen;
    UCHAR lpszMsg[SQL_MAX_MESSAGE_LENGTH];
    UCHAR lpszState[SQL_SQLSTATE_SIZE+1];
    SDWORD lNative;

    ::SQLError(g_henv, m_hdbc, hstmt, lpszState, &lNative,
        lpszMsg, SQL_MAX_MESSAGE_LENGTH-1, &nOutlen);

    // START:ansioem
#ifdef _CONSOLE
    ::AnsiToOem(
        (const char*)lpszMsg, 
        (char*)lpszMsg);
#endif
    // END:ansioem

    stringstream out;
    out << "'" << (const char*)lpszState << "', " << lNative 
        << ", '" << (const char*)lpszMsg << "'." << endl;
    return Exception(out.str(), (char*)lpszState, lNative, 
        (char*)lpszMsg);
}


void DataSource::TraceError(HSTMT hstmt)
{
    Exception ex = GetError(hstmt);
    OutputDebugString(ex.what());
}

void DataSource::ThrowError(HSTMT hstmt)
{
    throw GetError(hstmt);
}


DataSource::~DataSource()
{
    Disconnect();
    FreeEnv();
}

bool DataSource::IsConnected() const
{
    return m_hdbc != SQL_NULL_HDBC;
}


void DataSource::InitEnv()
{
    if (InterlockedIncrement(&g_envCount) == 0) {
        ::SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HENV, 
            &g_henv);
        ::SQLSetEnvAttr(g_henv, SQL_ATTR_ODBC_VERSION,
            (SQLPOINTER)SQL_OV_ODBC2, SQL_IS_INTEGER);
    }
}

void DataSource::FreeEnv()
{
    if (InterlockedDecrement(&g_envCount) == -1) {
        ::SQLFreeHandle(SQL_HANDLE_ENV, g_henv);
        g_henv = SQL_NULL_HENV;
    }
}


void DataSource::Commit()
{
    HRESULT ret = ::SQLTransact(g_henv, m_hdbc, SQL_COMMIT);
    CheckStatus(ret);
}

void SqlStatement::WriteResultColumns(ostream& out, 
    size_t columns, size_t dataLen, const string& sep)
{
   string line;
   vector<char> colName(dataLen+1);
   for (int i = 1; i <= columns; i++) {
      SWORD dummy;
      UDWORD prec;
      SWORD nameLen = colName.size();
      DescribeCol(i, (UCHAR*)&colName[0], colName.size(),
          &nameLen, &dummy, &prec, &dummy, &dummy);
      line += &colName[0] + sep;
   }
   if (!line.empty()) {
      line[line.length()-1]='\0'; // delete last tab
   }
   out << line << endl;
}
void SqlStatement::WriteResultColumnsToFILE(FILE* _fout, 
    size_t columns, size_t dataLen, const string& sep)
{
   string line;
   vector<char> colName(dataLen+1);
   assert(_fout!=NULL);
   for (int i = 1; i <= columns; i++) {
      SWORD dummy;
      UDWORD prec;
      SWORD nameLen = colName.size();
      DescribeCol(i, (UCHAR*)&colName[0], colName.size(),
          &nameLen, &dummy, &prec, &dummy, &dummy);
      line += &colName[0] + sep;
   }
   if (!line.empty()) {
	  //changed to add a new line character for proper
	  //importing into database and for viewing in text editor
      line[line.length()-1]='\n'; // delete last tab
   }
	fputs(line.c_str(),_fout);
	//fputs("\r\n",_fout);
   //out << line << endl;
}
void SqlStatement::WriteColumnSchemaToFILE(FILE* _fout, 
    size_t columns, size_t dataLen)
{
   //string line;
   vector<char> colName(dataLen+1);
   assert(_fout!=NULL);
   for (int i = 1; i <= columns; i++) {
      SWORD dummy;
      UDWORD prec;
      SWORD nameLen = colName.size();
      DescribeCol(i, (UCHAR*)&colName[0], colName.size(),
          &nameLen, &dummy, &prec, &dummy, &dummy);
      
	  switch(i){
			  case 1:
			  case 2:
			  case 3:
				  fprintf(_fout,"%+40s\n\n",&colName[0]);
				  break;
			  case 4:
					fprintf(_fout,"%s",&colName[0]);
				  break;
			  default :
				  if(i%12==0){
					fprintf(_fout,"\n  ");
				  }
				   fprintf(_fout,"     %s",&colName[0]);
   }
	
   }
   fprintf(_fout,"\n");
	for(int j=1;j<140;j++){
		fprintf(_fout,"-");
	}
	fprintf(_fout,"\n");
}


void SqlStatement::InitResultRows(size_t columns, 
    size_t dataLen, Row& row)
{
    row.resize(columns);
    for (int i = 0; i < row.size(); i++) {
        row[i].length = 0;
        row[i].data.resize(dataLen);
        row[i].data[0] = '\0';
    }

    // bind column data to the result set
    for (int i = 0; i < columns; i++) {
        BindCol(&row[i].data[0], dataLen, &row[i].length, 
            SQL_C_CHAR);
    }
}


void SqlStatement::WriteResultRows(ostream& out, 
    size_t columns, Row& row, const string& null, 
    const string& sep)
{
    while (Next()) {
        string line;
        for (int i = 0; i < columns; i++) {
            if (row[i].length == SQL_NULL_DATA) {
                line += null;
            } else {
                line += (char*)&row[i].data[0];
            }
            line += sep;
        }
        if (!line.empty()) {
            line[line.length()-1]='\0'; // delete last tab
        } else {
            break;
        }
        out << line << endl;
   }
}

void SqlStatement::WriteResultRowsToFILE(FILE* _fout, 
    size_t columns, Row& row, const string& null, 
    const string& sep)
{
	assert(_fout!=NULL);
    while (Next()) {
        string line;
        for (int i = 0; i < columns; i++) {
            if (row[i].length == SQL_NULL_DATA) {
                line += null;
            } else {
                line += (char*)&row[i].data[0];
            }
            line += sep;
        }
        if (!line.empty()) {
			//changed to add a new line character for proper
			//importing into database and for viewing in text editor
            line[line.length()-1]='\n'; // delete last tab
        } else {
            break;
        }
		fputs(line.c_str(),_fout);
		//fputs("\r\n",_fout);
        //out << line << endl;
   }
}
//This method will write out the schema data to FILE*
void SqlStatement::WriteRowsSchemaToFILE(FILE* _fout, 
    size_t columns, Row& row)
{
	assert(_fout!=NULL);
	//pull the first 3 fields from row 1 to populate the table schema info
	//before writing each row of column info in file 

	for (int j = 0; j < columns; j++) {
            if (row[j].length == SQL_NULL_DATA) {
                
            } else {
				switch(j){
					case 0:
						/*fprintf(_fout,"\v");
						fprintf(_fout,"%+35s",(char*)&row[j].data[0]);
						break;*/
					case 1:
						/*fprintf(_fout,"\v\v");
						fprintf(_fout,"%+35s",(char*)&row[j].data[0]);
						break;*/
					case 2:
						/*fprintf(_fout,"\v\v");
						fprintf(_fout,"%+35s",(char*)&row[j].data[0]);
						fprintf(_fout,"\v\v\v\v");
						break;*/
					default :
						if(j%12==0){
					//fprintf(_fout,"\n\t");
				  }
				   //fprintf(_fout,"%s     ",(char*)&row[j].data[0]);



				}
            }
            
        }
    while (Next()) {
        //string line;
        for (int i = 3; i < columns; i++) {
            if (row[i].length == SQL_NULL_DATA) {
                fprintf(_fout,"%-17s","/*/");
				//(char*)&row[i].data[0]="/*/";
            } 
			else{
				fprintf(_fout,"%-17s",(char*)&row[i].data[0]);
			}
              if(i==10){
					fprintf(_fout,"\n\t");
				  }
				   
			
            //line += sep;
        }
		fprintf(_fout,"\n\n");
  //      if (!line.empty()) {
		//	//changed to add a new line character for proper
		//	//importing into database and for viewing in text editor
  //          line[line.length()-1]='\n'; // delete last tab
  //      } else {
  //          break;
  //      }
		//fputs(line.c_str(),_fout);
		//fputs("\r\n",_fout);
        //out << line << endl;
   }
}

void SqlStatement::WriteResultSet(ostream& out, 
    size_t dataLen, const string& null, const string& sep)
{
    // see how many columns we have in the result set
    // zero columns means nothing to show
    SWORD nCols = NumResultCols();
    if (nCols != 0) {
        WriteResultSet(out, nCols, dataLen, null, sep);
    }
}
void SqlStatement::WriteResultSetToFILE(FILE* _fout, 
    size_t dataLen, const string& null, const string& sep)
{
    // see how many columns we have in the result set
    // zero columns means nothing to show
    SWORD nCols = NumResultCols();
    if (nCols != 0) {
		WriteResultSetToFILE(_fout, nCols, dataLen, null, sep);
    }
}
//This will write Table schema to file using the -s flag
void SqlStatement::WriteSchemaResultToFILE(FILE* _fout, 
    size_t dataLen)
	
{
	
	 // see how many columns we have in the result set
    // zero columns means nothing to show
    SWORD nCols = NumResultCols();
    if (nCols != 0) {
	vector<char> colName(dataLen+1);
		Row row;
		InitResultRows(nCols, dataLen, row);
		assert(_fout!=NULL);
   for (int i = 1; i <= nCols; i++) {
      SWORD dummy;
      UDWORD prec;
      SWORD nameLen = colName.size();
      DescribeCol(i, (UCHAR*)&colName[0], colName.size(),
          &nameLen, &dummy, &prec, &dummy, &dummy);
      
	  switch(i){
			  case 1:
				fprintf(_fout,"%+40s\n",&colName[0]);
				Next();
				if (row[0].length != SQL_NULL_DATA) {
				fprintf(_fout,"%+40s",(char*)&row[0].data[0]);}
				fprintf(_fout,"\n");
				break;
			  case 2:
				fprintf(_fout,"%+41s\n",&colName[0]);
				Next();
				if (row[1].length != SQL_NULL_DATA) {
				fprintf(_fout,"%+40s",(char*)&row[1].data[0]);}
				fprintf(_fout,"\n");
				break;
			  case 3:
				fprintf(_fout,"%+40s\n",&colName[0]);
				Next();
				if (row[2].length != SQL_NULL_DATA) {
				fprintf(_fout,"%+41s",(char*)&row[2].data[0]);}
				fprintf(_fout,"\n");
				  break;
			  case 4:
					fprintf(_fout,"%s\n\t",&colName[0]);
				  break;
			  default :
				  if(i%12==0){
					fprintf(_fout,"\n");
				  }
				   fprintf(_fout,"%s   ",&colName[0]);
   }
	
   }
   fprintf(_fout,"\n");
	for(int j=1;j<124;j++){
		fprintf(_fout,"-");
	}
	fprintf(_fout,"\n");
	
		//Will now proceed to walk through each remaining rows
		while (Next()) {
        //string line;
        for (int i = 3; i < nCols; i++) {
            if (row[i].length == SQL_NULL_DATA) {
                fprintf(_fout,"%-14.14s","/*/");
				//(char*)&row[i].data[0]="/*/";
            } 
			else{
				if(i==3){
					fprintf(_fout,"%s\n\t",(char*)&row[i].data[0]);
					}
				else{fprintf(_fout,"%-14.14s ",(char*)&row[i].data[0]);}
			}
              if(i==10){
					fprintf(_fout,"\n");
				  }
				  
        }
		fprintf(_fout,"\n");
		for(int j=1;j<124;j++){
			fprintf(_fout,"-");
		}
		fprintf(_fout,"\n");
  
   }

    }
	
  

  //  }
}

void SqlStatement::WriteResultSet(ostream& out, size_t cols,
    size_t length, const string& null, const std::string& sep)
{
    WriteResultColumns(out, cols, length, sep);

    Row row;
    InitResultRows(cols, length, row);
    WriteResultRows(out, cols, row, null, sep);
}
void SqlStatement::WriteResultSetToFILE(FILE* _fout, size_t cols,
    size_t length, const string& null, const std::string& sep)
{
    WriteResultColumnsToFILE(_fout, cols, length, sep);

    Row row;
    InitResultRows(cols, length, row);
    WriteResultRowsToFILE(_fout, cols, row, null, sep);
}
bool DataSource::Add(const char* driver, const char* attr)
{
    return SQLConfigDataSource(NULL, ODBC_ADD_DSN, driver, 
        attr) != FALSE;
}

bool DataSource::Remove(const char* driver, const char* attr)
{
    return SQLConfigDataSource(NULL, ODBC_REMOVE_DSN, driver, 
        attr) != FALSE;
}

} // namespace odbc
