myodbc
======

ODBC SQL command line tool for DSN

 1)Download zip to desired location

 2)compile using MS Visual C++ 2010 express

 3) Set up a DSN to any data source using the ODBC driver for the source.
 
 4)Open cmd prompt and type "myodbc"
  
  
  Usage: \<data source\> [-n] [-f][sql][-s][table][-i file]
      
        -dump <datasource> [-n][-s][table]
        [-n] Replace blank fields with literal NULL
             no NULL by default
        [-f] Will place stdout to file instead of console
             for SQL,file name will be <datasource>_sql.sql
            -dump flag will place stdout in <table>.txt by default
        [-s] Will display schema of table.Table name is not
             optional;it must be included.
                  To print to a file use the [-f] flag.When using
             the -dump flag, -f is not necessary
                  <data source> without any other parameters will
             return all table names for the given DSN.
        [-i] used to load sql from a file.Extension is not
             necessary, but complete filename w/path must
             follow this flag.

This is designed to work with any datasource as long as the vendor supplies an ODBC driver.
You can type sql or use a sql file. Ex

 Using SQL:

    myodbc <dsn name> "select * from customer"
 
 Using input SQL file and output to file with name of <DSN>:
 
    myodbc <dsn name> -f -i "c:\tmp\my_favorite.sql"
    
 NOTE: at this time you can not give the output file a name, it defaults to dsn.txt.
   
