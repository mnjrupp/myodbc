
#include "sqlodbc.h"
#include <iostream>
using namespace std;
using namespace odbc;
char* _sqlfromFILE(const char* ifilename);
int main(int argc, char** argv)
{
	char* strNull="";

	if (argc < 2 ||argc >5)
	{
		cerr<<"Usage: <data source> [-n] [-f][sql][-s][table][-i file]\n ";
		cerr<<"     : -dump <datasource> [-n][-s][table]\n ";
		cerr<<"\t[-n] Replace blank fields with literal NULL\n";
		cerr<<"\t     no NULL by default\n";
		cerr<<"\t[-f] Will place stdout to file instead of console\n";
		cerr<<"\t     for SQL,file name will be <datasource>_sql.sql\n";
		cerr<<"\t    -dump flag will place stdout in <table>.txt by default\n";
		cerr<<"\t[-s] Will display schema of table.Table name is not\n";
		cerr<<"\t     optional;it must be included.\n";
		cerr<<"\t	  To print to a file use the [-f] flag.When using\n";
		cerr<<"\t     the -dump flag, -f is not necessary\n";
		cerr<<"\t	  <data source> without any other parameters will\n";
		cerr<<"\t     return all table names for the given DSN.\n";
		cerr<<"\t[-i] used to load sql from a file.Extension is not\n";
		cerr<<"\t     necessary, but complete filename w/path must\n";
		cerr<<"\t     follow this flag.\n"<<endl;
		exit(-1);
	}

	if(strcmp(argv[1],"-dump")!=0)
	{
		if(argc > 2 && strcmp(argv[2],"-n")==0 || argc > 3 && strcmp(argv[3],"-n")==0){
		
			strNull = "NULL";
		}
		
			try {
		DataSource db;
		db.Connect(argv[1]);

		SqlStatement sql(db);
		if (argc == 2) {
		  sql.Tables();
		  sql.WriteResultSet(cout, 256,
		  strNull, "|");
		  return 0;
		} else {
			if(argc==5 && strcmp(argv[3],"-i")==0){
				sql.Execute(_sqlfromFILE(argv[4]));
			}
			if(argc==5 && strcmp(argv[3],"-s")==0){
				sql.Columns(argv[4]);
			}
			if(argc==4 && strcmp(argv[2],"-i")==0){
				sql.Execute(_sqlfromFILE(argv[3]));
			}
			if(argc==4 && strcmp(argv[2],"-s")==0){
				sql.Columns(argv[3]);
			}
			if(argc==4 && strcmp(argv[2],"-i")!=0 && strcmp(argv[2],"-s")!=0){
				sql.Execute(argv[3]);
			}
			if(argc==3){
				sql.Execute(argv[2]);
			}
		}
		if((argc > 2 && strcmp(argv[2],"-f")==0)||(argc > 3 && strcmp(argv[3],"-f")==0)){
			FILE* pfile;
			char* strDSN = argv[1];
			pfile = fopen(strcat(strDSN,"_sql.txt"),"w");
			if(pfile!=NULL)
			{
				sql.WriteResultSetToFILE(pfile,256,strNull, "|");
				fclose(pfile);
			}
		}
		else{
		sql.WriteResultSet(cout, 256,
		  strNull, "|");
		}
		return 0;
	  } catch (const exception& ex) {
		cerr << argv[0] << ": " 
			 << ex.what() << endl;
	  } catch (...) {
		cerr << "Unknown exception." 
			 << endl;
	  }
	  return 1;
	}
	else 
		if(strcmp(argv[1],"-dump")== 0)
	{
		if(argc<4)
		{
			cerr<<"Usage: -dump <datasource> [-n][-s] <table> "<<endl;
			exit(-1);
		}

		if(argc == 4 && ((strcmp(argv[3],"-n")== 0 || strcmp(argv[3],"-f")== 0
			|| strcmp(argv[3],"-s")== 0))){
			cerr<<"Usage: -dump <datasource> [-n][-s] <table> "<<endl;
			exit(-1);
		}

		if(argc == 5 && (strcmp(argv[3],"-n")!= 0 && strcmp(argv[3],"-s")!= 0)){
			cerr<<"Usage: -dump <datasource> [-n][-s] <table> "<<endl;
			exit(-1);
		}

			try {
		DataSource db;
		db.Connect(argv[2]);
		SqlStatement sql(db);
		char sqlstr[100];
		//char* strResult;
		char sqlTble[100];
		//requesting schema of table using -s flag
		if(strcmp(argv[3],"-s")== 0){
			//grab the table name
			strcpy_s(sqlTble,sizeof(sqlTble),argv[4]);
			sql.Columns(sqlTble);
			FILE* pfile;
			strcat_s(sqlTble,sizeof(sqlTble),"_schema.txt");
			pfile = fopen(sqlTble,"w");
			if(pfile!=NULL)
			{
				/*sql.WriteResultSetToFILE(pfile,256,"","|");*/
				sql.WriteSchemaResultToFILE(pfile,256);
				fclose(pfile);
			}
			return 0;

		}
		strcpy_s(sqlstr,sizeof(sqlstr),"SELECT * FROM ");
		if(strcmp(argv[3],"-n")== 0){
			strNull = "NULL";
			strcpy_s(sqlTble,sizeof(sqlTble),argv[4]);
			strcat_s(sqlstr,sizeof(sqlstr),argv[4]);
		}
		else{
			strcpy_s(sqlTble,sizeof(sqlTble),argv[3]);
			strcat_s(sqlstr,sizeof(sqlstr),argv[3]);
		}
	    sql.Execute(sqlstr);
		FILE* pfile;
		strcat_s(sqlTble,sizeof(sqlTble),".txt");
		pfile = fopen(sqlTble,"w");
			if(pfile!=NULL)
			{
				sql.WriteResultSetToFILE(pfile,256,strNull, "|");
				fclose(pfile);
			}
			return 0;
	  } 
		catch (const exception& ex) 
			{
				cerr << argv[0] << ": " 
				<< ex.what() << endl;
		}
		catch (...) 
		{
			cerr << "Unknown exception." 
			 << endl;
	  }
		return 1;
	}
	
}
 char* _sqlfromFILE(const char* ifilename){
	FILE* _sqlFile;
	int ch;
	string strSQL;
	if(strlen(ifilename)>0 && strlen(ifilename)<=MAX_PATH){
		try{
		_sqlFile = fopen(ifilename,"r");
		if(_sqlFile!=NULL){
		 do
		 {
			ch= fgetc(_sqlFile);
			if(ch != '\n'&& ch != -1){strSQL+=(char)ch;}

		 }while(ch!=EOF);
			fclose(_sqlFile);
			if(!strSQL.empty())
			{
				char * chrsql = new char[strSQL.size()];
			return strcpy(chrsql,strSQL.c_str());
			}
		}
		
		else{
			cerr<<"error opening SQL file."<<endl;
			exit(-1);
		}
		}
		catch (const exception& ex) 
			{
				cerr << ex.what() << endl;
				exit(-1);
		}
		catch (...) 
		{
			cerr << "Unknown exception." 
			 << endl;
			exit(-1);
	  }
	}
	return "";
}