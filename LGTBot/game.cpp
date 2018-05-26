std::unique_ptr<sql::Statement> state(sql::mysql::get_mysql_driver_instance()
                                      ->connect("tcp://127.0.0.1:3306", "root", "")
                                      ->createStatement());