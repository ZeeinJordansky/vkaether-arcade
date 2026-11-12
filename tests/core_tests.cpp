#include "vkaether/commands.hpp"

#include <cassert>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

int main() {
    const auto path = std::filesystem::temp_directory_path() / "vkaether-test.sqlite3";
    std::filesystem::remove(path);

    {
        vkaether::Database database(path);
        vkaether::CommandRegistry commands(database);

        assert(commands.size() >= 125);
        assert(commands.dispatch({1, 42, "помощь"}).text.find("Aether Arcade") != std::string::npos);

        // All 50 added commands must be registered and return a response.
        const std::vector<std::string> added{
            "орёл", "решка", "бросок", "кости", "d4", "d6", "d8", "d10", "d12", "d20", "d100",
            "даилинет", "совместимость", "переверни", "повтори", "процент", "делить", "остаток",
            "степень", "факториал", "простое", "четное", "нечетное", "цветкод", "uuid", "загадка",
            "викторина", "слово", "имя", "никнейм", "планета", "стихия", "эмодзи", "квест", "миссия",
            "локация", "оружие", "броня", "заклинание", "зелье", "ресурс", "редкость", "класс", "раса",
            "способность", "персонаж", "испытание", "достижение", "трофей", "событие"
        };
        for (const auto& name : added) {
            const auto reply = commands.dispatch({1, 42, name});
            assert(!reply.text.empty());
        }

        assert(commands.dispatch({1, 42, "делить 10 0"}).text.find("ноль") != std::string::npos);
        assert(commands.dispatch({1, 42, "остаток -9223372036854775808 -1"}).text.find("0") != std::string::npos);
        assert(commands.dispatch({1, 42, "факториал 20"}).text.find("2432902008176640000") != std::string::npos);
        assert(commands.dispatch({1, 42, "простое 97"}).text.find("простое") != std::string::npos);
        assert(commands.dispatch({1, 42, "умножить 9223372036854775807 2"}).text.find("переполнения") != std::string::npos);
        assert(commands.dispatch({1, 42, "число -2147483649 10"}).text.find("Использование") != std::string::npos);
        assert(commands.dispatch({1, 42, "переверни \"Привет мир\""}).text.find("рим тевирП") != std::string::npos);

        assert(database.daily_bonus(42, 500) == 500);
        assert(database.daily_bonus(42, 500) == 0);
        assert(database.transfer(42, 43, 100));
        assert(database.get_user(42).balance == 1400);
        assert(database.get_user(43).balance == 1100);
    }

    std::filesystem::remove(path);
    std::filesystem::remove(path.string() + "-wal");
    std::filesystem::remove(path.string() + "-shm");
}
