#pragma once
#include <jdbc/mysql_connection.h>
#include <jdbc/mysql_driver.h>

#define FAIL_THROW(sql_func, ...) \
do\
{\
  SQLRETURN ret = sql_func(##__VA_ARGS__);\
  if (!SQL_SUCCEEDED(ret)) { throw (std::stringstream() << #sql_func << " failed ret: " << ret).str(); }\
} while (0)

class DBManager
{
 public:
   DBManager(std::unique_ptr<sql::mysql::MySQL_Driver>&& driver, std::unique_ptr<sql::Connection>&& connection)
     : driver_(std::move(driver)), connection_(std::move(connection))
   {
     assert(driver_ && connection);
   }
   ~DBManager(){}

   static ErrCode MakeDBManager(const std::string& addr, const std::string& user, const std::string& passwd, const std::string& db_name, const bool create_if_not_found)
   {
     if (g_db_manager_) { return EC_DB_ALREADY_CONNECTED; }
     std::unique_ptr<sql::mysql::MySQL_Driver> driver(sql::mysql::get_mysql_driver_instance());
     if (!driver) { return EC_DB_GET_DRIVER_FAILED; }
     std::unique_ptr<sql::Connection> connection(driver->connect(addr, user, passwd));
     if (!connection) { return EC_DB_CONNECT_FAILED; }
     connection->setAutoCommit(false);
     connection->setSchema(db_name);
     return EC_OK;
     
   }

   static std::string DestroyDBManager()
   {
     if (!g_db_manager_) { return "db has not been connected"; }
     g_db_manager_ = nullptr;
     return "";
   }

   static const std::unique_ptr<DBManager>& GetDBManager() { return g_db_manager_; }

   void StartTransaction();
   void CommitTransaction();
   void AbortTransaction();
private:
   static std::unique_ptr<DBManager> g_db_manager_;
   std::unique_ptr<sql::mysql::MySQL_Driver> driver_;
   std::unique_ptr<sql::Connection> connection_;
};
