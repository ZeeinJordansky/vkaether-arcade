#include "vkaether/commands.hpp"
#include <cassert>
#include <filesystem>
int main(){auto p=std::filesystem::temp_directory_path()/"vkaether-test.sqlite3";std::filesystem::remove(p);{vkaether::Database db(p);vkaether::CommandRegistry c(db);assert(c.size()>=50);assert(c.dispatch({1,42,"помощь"}).text.find("Aether Arcade")!=std::string::npos);assert(db.daily_bonus(42,500)==500);assert(db.daily_bonus(42,500)==0);assert(db.transfer(42,43,100));assert(db.get_user(42).balance==1400);assert(db.get_user(43).balance==1100);}std::filesystem::remove(p);std::filesystem::remove(p.string()+"-wal");std::filesystem::remove(p.string()+"-shm");}
