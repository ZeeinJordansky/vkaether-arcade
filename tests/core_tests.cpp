#include "vkaether/commands.hpp"
#include "../src/native/validation.h"
#include <cstdint>
extern "C" int aether_policy_validate_message(const unsigned char* data, int length);
extern "C" int vkaether_rust_rps(int player, int bot);

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main() {
    const auto path = std::filesystem::temp_directory_path() / "vkaether-test.sqlite3";
    std::filesystem::remove(path);

    try {
        vkaether::Database database(path);
        vkaether::CommandRegistry commands(database);

        require(commands.size() >= 125, "expected at least 125 registered command names");
        require(vkaether_rust_rps(0, 0) == 0, "Rust RPS draw resolution failed");
        require(vkaether_rust_rps(1, 0) == 1, "Rust RPS player win resolution failed");
        require(vkaether_rust_rps(0, 1) == 2, "Rust RPS bot win resolution failed");
        require(vkaether_rust_rps(9, 0) == -1, "Rust RPS accepted invalid choice");
        require(vkaether_valid_identity(2000000001, 42) == 1, "C identity validator rejected valid IDs");
        require(vkaether_valid_identity(0, 42) == 0, "C identity validator accepted zero peer ID");
        const unsigned char valid_message[] = {'h', 'i'};
        const unsigned char invalid_message[] = {'h', 0, 'i'};
        const unsigned char invalid_utf8[] = {0xff};
        require(aether_policy_validate_message(valid_message, 2) == 0, "C# policy rejected valid UTF-8");
        require(aether_policy_validate_message(invalid_utf8, 1) == 1, "C# policy accepted invalid UTF-8");
        require(aether_policy_validate_message(invalid_message, 3) == 1, "C# policy accepted embedded NUL");
        require(aether_policy_validate_message(valid_message, 4097) == 2, "C# policy accepted oversized input");

        require(commands.dispatch({1, 42, "помощь"}).text.find("Aether Arcade") != std::string::npos,
                "help command did not return expected output");

        require(commands.dispatch({1, 42, "rsum 20 22"}).text.find("42") != std::string::npos,
                "Rust FFI addition failed");
        require(commands.dispatch({1, 42, "rsum 9223372036854775807 1"}).text.find("переполнение") != std::string::npos,
                "Rust FFI overflow was not rejected");
        require(commands.dispatch({1, 42, "lua return 6 * 7"}).text.find("42") != std::string::npos,
                "Lua expression execution failed");
        require(commands.dispatch({1, 42, "lua return os.execute('id')"}).text.find("ошибка") != std::string::npos,
                "Lua sandbox exposed OS facilities");

        // Every one of the 50 newly added commands must be registered and executable.
        const std::vector<std::string> added{
            "орёл", "решка", "бросок", "кости", "d4", "d6", "d8", "d10", "d12", "d20", "d100",
            "даилинет", "совместимость", "переверни", "повтори", "процент", "делить", "остаток",
            "степень", "факториал", "простое", "четное", "нечетное", "цветкод", "uuid", "загадка",
            "викторина", "слово", "имя", "никнейм", "планета", "стихия", "эмодзи", "квест", "миссия",
            "локация", "оружие", "броня", "заклинание", "зелье", "ресурс", "редкость", "класс", "раса",
            "способность", "персонаж", "испытание", "достижение", "трофей", "событие"
        };
        require(added.size() == 50, "test list must contain exactly 50 new commands");
        for (const auto& name : added) {
            const auto reply = commands.dispatch({1, 42, name});
            require(!reply.text.empty(), ("command returned empty reply: " + name).c_str());
        }

        require(commands.dispatch({1, 42, "делить 10 0"}).text.find("ноль") != std::string::npos,
                "division by zero was not rejected");
        require(commands.dispatch({1, 42, "остаток -9223372036854775808 -1"}).text.find("0") != std::string::npos,
                "INT64_MIN modulo -1 was not handled safely");
        require(commands.dispatch({1, 42, "факториал 20"}).text.find("2432902008176640000") != std::string::npos,
                "factorial result is incorrect");
        require(commands.dispatch({1, 42, "простое 97"}).text.find("простое") != std::string::npos,
                "prime number check failed");
        require(commands.dispatch({1, 42, "умножить 9223372036854775807 2"}).text.find("диапазон") != std::string::npos,
                "multiplication overflow was not rejected");
        require(commands.dispatch({1, 42, "число -2147483649 10"}).text.find("Использование") != std::string::npos,
                "out-of-range random number bounds were accepted");
        require(commands.dispatch({1, 42, "переверни \"Привет мир\""}).text.find("рим тевирП") != std::string::npos,
                "UTF-8 text reversal failed");

        require(database.work(44, 250), "valid work shift failed");
        require(database.get_user(44).balance == 1250, "work reward was not credited");
        require(!database.work(44, -1), "negative work reward was accepted");
        require(!database.transfer(44, 44, 1), "self-transfer was accepted");

        require(database.daily_bonus(42, 500) == 500, "first daily bonus was not granted");
        require(database.daily_bonus(42, 500) == 0, "duplicate daily bonus was granted");
        require(database.transfer(42, 43, 100), "valid transfer failed");
        require(database.get_user(42).balance == 1400, "sender balance is incorrect after transfer");
        require(database.get_user(43).balance == 1100, "receiver balance is incorrect after transfer");
    } catch (...) {
        std::filesystem::remove(path);
        std::filesystem::remove(path.string() + "-wal");
        std::filesystem::remove(path.string() + "-shm");
        throw;
    }

    std::filesystem::remove(path);
    std::filesystem::remove(path.string() + "-wal");
    std::filesystem::remove(path.string() + "-shm");
    return 0;
}
