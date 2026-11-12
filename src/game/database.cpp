#include "vkaether/database.hpp"

#include <algorithm>
#include <filesystem>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>

namespace vkaether {
namespace {

void check(sqlite3* db, int rc, const char* operation) {
    if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW) {
        throw std::runtime_error(std::string(operation) + ": " +
                                 (db ? sqlite3_errmsg(db) : "SQLite error"));
    }
}

class Statement {
public:
    Statement(sqlite3* database, const char* sql) : database_(database) {
        check(database_, sqlite3_prepare_v2(database_, sql, -1, &statement_, nullptr), "prepare");
    }
    ~Statement() { sqlite3_finalize(statement_); }
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    sqlite3_stmt* get() const noexcept { return statement_; }

private:
    sqlite3* database_{};
    sqlite3_stmt* statement_{};
};

} // namespace

Database::Database(const std::filesystem::path& path) {
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());

    const auto filename = path.string();
    const int open_result = sqlite3_open_v2(
        filename.c_str(), &db_,
        SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);

    if (open_result != SQLITE_OK) {
        const std::string error = db_ ? sqlite3_errmsg(db_) : "SQLite allocation failure";
        if (db_) sqlite3_close(db_);
        db_ = nullptr;
        throw std::runtime_error(error);
    }

    try {
        sqlite3_busy_timeout(db_, 3000);
        execute(
            "PRAGMA journal_mode=WAL;"
            "PRAGMA synchronous=NORMAL;"
            "PRAGMA temp_store=MEMORY;"
            "CREATE TABLE IF NOT EXISTS users("
            "id INTEGER PRIMARY KEY,"
            "balance INTEGER NOT NULL DEFAULT 1000,"
            "experience INTEGER NOT NULL DEFAULT 0,"
            "level INTEGER NOT NULL DEFAULT 1,"
            "last_bonus INTEGER NOT NULL DEFAULT 0"
            ");"
            "CREATE INDEX IF NOT EXISTS idx_users_balance ON users(balance DESC);");
    } catch (...) {
        sqlite3_close(db_);
        db_ = nullptr;
        throw;
    }
}

Database::~Database() {
    if (db_) sqlite3_close(db_);
}

void Database::execute(const char* sql) {
    char* error = nullptr;
    const int result = sqlite3_exec(db_, sql, nullptr, nullptr, &error);
    if (result != SQLITE_OK) {
        const std::string message = error ? error : sqlite3_errmsg(db_);
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

UserState Database::get_user(std::int64_t id) {
    if (id <= 0) throw std::invalid_argument("user ID must be positive");

    std::lock_guard lock(mutex_);
    Statement insert(db_, "INSERT OR IGNORE INTO users(id) VALUES(?1)");
    check(db_, sqlite3_bind_int64(insert.get(), 1, id), "bind user ID");
    check(db_, sqlite3_step(insert.get()), "insert user");

    Statement query(db_, "SELECT id,balance,experience,level FROM users WHERE id=?1");
    check(db_, sqlite3_bind_int64(query.get(), 1, id), "bind user ID");
    check(db_, sqlite3_step(query.get()), "select user");

    return {
        sqlite3_column_int64(query.get(), 0),
        sqlite3_column_int64(query.get(), 1),
        sqlite3_column_int64(query.get(), 2),
        sqlite3_column_int64(query.get(), 3)
    };
}

std::int64_t Database::daily_bonus(std::int64_t id, std::int64_t amount) {
    if (id <= 0 || amount <= 0) return 0;

    std::lock_guard lock(mutex_);
    execute("BEGIN IMMEDIATE");
    try {
        Statement insert(db_, "INSERT OR IGNORE INTO users(id) VALUES(?1)");
        check(db_, sqlite3_bind_int64(insert.get(), 1, id), "bind user ID");
        check(db_, sqlite3_step(insert.get()), "insert user");

        Statement update(
            db_,
            "UPDATE users "
            "SET balance=balance+?1,last_bonus=unixepoch()/86400 "
            "WHERE id=?2 AND last_bonus<unixepoch()/86400 "
            "AND balance<=9223372036854775807-?1");
        check(db_, sqlite3_bind_int64(update.get(), 1, amount), "bind bonus amount");
        check(db_, sqlite3_bind_int64(update.get(), 2, id), "bind user ID");
        check(db_, sqlite3_step(update.get()), "grant daily bonus");

        const bool granted = sqlite3_changes(db_) == 1;
        execute("COMMIT");
        return granted ? amount : 0;
    } catch (...) {
        execute("ROLLBACK");
        throw;
    }
}

bool Database::work(std::int64_t id, std::int64_t reward) {
    if (id <= 0 || reward < 0) return false;

    std::lock_guard lock(mutex_);
    Statement insert(db_, "INSERT OR IGNORE INTO users(id) VALUES(?1)");
    check(db_, sqlite3_bind_int64(insert.get(), 1, id), "bind user ID");
    check(db_, sqlite3_step(insert.get()), "insert user");

    Statement update(
        db_,
        "UPDATE users "
        "SET balance=balance+?1,experience=experience+2,level=1+(experience+2)/100 "
        "WHERE id=?2 AND balance<=9223372036854775807-?1 "
        "AND experience<=9223372036854775805");
    check(db_, sqlite3_bind_int64(update.get(), 1, reward), "bind work reward");
    check(db_, sqlite3_bind_int64(update.get(), 2, id), "bind user ID");
    check(db_, sqlite3_step(update.get()), "complete work shift");
    return sqlite3_changes(db_) == 1;
}

bool Database::transfer(std::int64_t from, std::int64_t to, std::int64_t amount) {
    if (from <= 0 || to <= 0 || from == to || amount <= 0) return false;

    std::lock_guard lock(mutex_);
    execute("BEGIN IMMEDIATE");
    try {
        Statement sender(db_, "INSERT OR IGNORE INTO users(id) VALUES(?1)");
        check(db_, sqlite3_bind_int64(sender.get(), 1, from), "bind sender ID");
        check(db_, sqlite3_step(sender.get()), "insert sender");

        Statement receiver(db_, "INSERT OR IGNORE INTO users(id) VALUES(?1)");
        check(db_, sqlite3_bind_int64(receiver.get(), 1, to), "bind receiver ID");
        check(db_, sqlite3_step(receiver.get()), "insert receiver");

        Statement debit(
            db_, "UPDATE users SET balance=balance-?1 WHERE id=?2 AND balance>=?1");
        check(db_, sqlite3_bind_int64(debit.get(), 1, amount), "bind transfer amount");
        check(db_, sqlite3_bind_int64(debit.get(), 2, from), "bind sender ID");
        check(db_, sqlite3_step(debit.get()), "debit sender");
        if (sqlite3_changes(db_) != 1) {
            execute("ROLLBACK");
            return false;
        }

        Statement credit(
            db_,
            "UPDATE users SET balance=balance+?1 "
            "WHERE id=?2 AND balance<=9223372036854775807-?1");
        check(db_, sqlite3_bind_int64(credit.get(), 1, amount), "bind transfer amount");
        check(db_, sqlite3_bind_int64(credit.get(), 2, to), "bind receiver ID");
        check(db_, sqlite3_step(credit.get()), "credit receiver");
        if (sqlite3_changes(db_) != 1) {
            execute("ROLLBACK");
            return false;
        }

        execute("COMMIT");
        return true;
    } catch (...) {
        execute("ROLLBACK");
        throw;
    }
}

std::string Database::top(std::size_t limit) {
    std::lock_guard lock(mutex_);
    const auto bounded_limit = std::min<std::size_t>(limit, 100);
    Statement query(db_, "SELECT id,balance FROM users ORDER BY balance DESC LIMIT ?1");
    check(db_, sqlite3_bind_int64(query.get(), 1, static_cast<sqlite3_int64>(bounded_limit)),
          "bind leaderboard limit");

    std::ostringstream output;
    std::size_t rank = 1;
    while (true) {
        const int result = sqlite3_step(query.get());
        if (result == SQLITE_DONE) break;
        check(db_, result, "read leaderboard");
        output << rank++ << ". ID " << sqlite3_column_int64(query.get(), 0)
               << " — " << sqlite3_column_int64(query.get(), 1) << " coins\n";
    }

    return output.str().empty() ? "Пока рейтинг пуст." : output.str();
}

} // namespace vkaether
