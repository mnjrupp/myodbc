//  $Id: sqlodbc.h,v 1.19 2001/10/14 15:50:37 erngui Exp $
//
//  SqlOdbc by Ernesto Guisado (erngui@acm.org)
//
//  Lightweight wrapper around the MS ODBC API.
//  
#ifndef INCLUDED_SQLODBC_H
#define INCLUDED_SQLODBC_H 1

// identifier truncated in debug info
#pragma warning(disable:4786) 

#include <windows.h>
#include <sqlext.h>
#include <iosfwd>
#include <string>
#include <vector>
#include <stdio.h>

namespace odbc_5189169245 {


// An ODBC Exception
class Exception : public std::exception {
public:
    Exception(const std::string& w, const std::string& state, 
        long native, const std::string& msg)
      : std::exception("odbc exception"), m_errorMsg(w), 
        m_state(state), m_native(native), m_msg(msg)
    {}
    virtual ~Exception() {}

    virtual const char* what() const 
    { return m_errorMsg.c_str(); }
    
    std::string m_errorMsg; // full error string
    std::string m_state; // from SQLError
    long m_native;       // from SQLError
    std::string m_msg;   // from SQLError
};

//  An ODBC data source
class DataSource {
    friend class SqlStatement;
public:
    DataSource();
    virtual ~DataSource();
    
    // Add a data source to the driver manager
    static bool Add(const char* driver, const char* attr);

    // Remove a data source to the driver manager
    static bool Remove(const char* driver, const char* attr);

    void Connect(const std::string& dsn, 
        const std::string& user = "", 
        const std::string& pwd = "", 
        u_long timeout = INFINITE);

    void Disconnect();

    bool IsConnected() const;

    // Commit transaction (if in manual commit mode)
    void Commit();
private:
    // Don't allow copying. They are private on purpose.
    DataSource(const DataSource&);
    void operator=(const DataSource&);

    SQLHDBC m_hdbc;

    Exception GetError(HSTMT hstmt = SQL_NULL_HSTMT);
    virtual void TraceError(HSTMT hstmt = SQL_NULL_HSTMT);
    void ThrowError(HSTMT hstmt = SQL_NULL_HSTMT);
    void CheckStatus(RETCODE ret, HSTMT = SQL_NULL_HSTMT);

    static void InitEnv();
    static void FreeEnv();
    static SQLHENV g_henv;
    static long g_envCount;
};


// A SQL statement to execute against an ODBC data 
// source
class SqlStatement {
public:
    SqlStatement(DataSource&);
    virtual ~SqlStatement();

    void Execute(const std::string& sql);
    void Tables(const std::string& type = "");
    void Columns(const std::string& table);
    
    // after execute, tables or columns you can..

    // write result set to stream
    void WriteResultSet(std::ostream& out, 
        size_t dataLen = 256, 
        const std::string& null = "NULL",
        const std::string& sep = "\t");
	//write result to a file
	void WriteResultSetToFILE(FILE* _fout,
		size_t dataLen = 256, 
        const std::string& null = "NULL",
        const std::string& sep = "\t");
	//write Table schema to FILE*
	void SqlStatement::WriteSchemaResultToFILE(FILE* _fout, 
    size_t dataLen);
	
    // ask number of columns in result
    SWORD NumResultCols();

    // ask number of rows in result
    SDWORD RowCount();

    // describe a column
    void DescribeCol(USHORT number, UCHAR *name,
       USHORT BufferLength, SHORT *NameLength,
       SHORT *DataType, ULONG *ColumnSize,
       SHORT *DecimalDigits, SHORT *Nullable);

    // bind the result set to a data structure and ...
    void BindCol(void* rgbValue, SDWORD cbValueMax, 
        SDWORD* pcbValue, 
        SQLSMALLINT TargetType = SQL_C_DEFAULT);
    
    // retrieve each data row in the result set
    bool Next();

    UWORD NumBoundCols();

    void GetData(WORD col, DWORD maxlen, 
        void* data, SDWORD* len);
private:
    // No copying allowed. Private on purpose.
    SqlStatement();
    SqlStatement(const SqlStatement&);
    SqlStatement& operator=(const SqlStatement&);

    bool IsValid() const;
    void CheckStatus(RETCODE ret);

    SQLHSTMT m_hstmt;
    u_short  m_boundCols;
    DataSource& m_dataSource;

    typedef struct Field {
        std::vector<UCHAR> data;
        SDWORD length;
    };
    typedef std::vector<Field> Row;

    // write result set to stream
    void WriteResultSet(std::ostream& out, size_t cols, 
        size_t dataLen, const std::string& null, 
        const std::string& sep);
	//write result set to FILE*
	void WriteResultSetToFILE(FILE* _fout, size_t cols, 
        size_t dataLen, const std::string& null, 
        const std::string& sep);
    // write to stream the columns names in the result set
    void WriteResultColumns(std::ostream& stream, size_t cols,
        size_t dataLen, const std::string& sep);
	//write to file the column names
	void WriteResultColumnsToFILE(FILE* _fout, size_t columns,
		size_t dataLen, const std::string& sep);
	//private function for output to File
	void SqlStatement::WriteColumnSchemaToFILE(FILE* _fout, 
    size_t columns, size_t dataLen);
    // initialize column data and column data length arrays
	void SqlStatement::WriteRowsSchemaToFILE(FILE* _fout, 
    size_t columns, Row& row);
    void InitResultRows(size_t columns, size_t dataLen, 
        Row& row);
    // fetch each row of the result set and write to stream
    void WriteResultRows(std::ostream& out, size_t columns, 
        Row& row, const std::string& null, 
        const std::string& sep);
	//fetch each row of the result set and write to FILE object
	void WriteResultRowsToFILE(FILE* _fout, 
		size_t columns, Row& row, const std::string& null, 
		const std::string& sep);
};


} // namespace odbc 

#ifndef NO_ODBC_5189169245_ALIAS
    namespace odbc = odbc_5189169245;
#endif

#endif // INCLUDED_SQLODBC_H

