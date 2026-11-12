#include "vkaether/commands.hpp"

#include <charconv>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vkaether {
namespace {

bool parse_int(std::string_view value, std::int64_t& result) {
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), result);
    return error == std::errc{} && end == value.data() + value.size();
}

std::mt19937_64& random_engine() {
    static thread_local std::mt19937_64 engine(std::random_device{}());
    return engine;
}

int random_int(int low, int high) {
    return std::uniform_int_distribution<int>(low, high)(random_engine());
}

std::string join(const std::vector<std::string>& values, std::string_view separator = " ") {
    std::string result;
    for (const auto& value : values) {
        if (!result.empty()) result += separator;
        result += value;
    }
    return result;
}

} // namespace

void CommandRegistry::register_extended_commands() {
    // 50 additional commands. These are self-contained mini-games and utilities;
    // none of them claim to mutate a player's balance or persistent inventory.
    const add_choice = [this](const char* name, std::vector<std::string> options) {
        add(name, [options = std::move(options)](const Message&, const auto&) {
            const auto index = static_cast<std::size_t>(random_int(0, static_cast<int>(options.size() - 1)));
            return Reply{options[index]};
        });
    };

    add("орёл", [](const Message&, const auto&) {
        return Reply{random_int(0, 1) ? "🪙 Орёл!" : "🪙 Решка!"};
    });
    add("решка", [](const Message&, const auto&) {
        return Reply{random_int(0, 1) ? "🪙 Решка!" : "🪙 Орёл!"};
    });
    add("бросок", [](const Message&, const auto& args) {
        int sides = 6;
        if (!args.empty()) {
            std::int64_t parsed{};
            if (args.size() != 1 || !parse_int(args[0], parsed) || parsed < 2 || parsed > 1000000)
                return Reply{"Использование: бросок [количество граней от 2 до 1000000]."};
            sides = static_cast<int>(parsed);
        }
        return Reply{"🎲 Результат броска d" + std::to_string(sides) + ": " +
                     std::to_string(random_int(1, sides))};
    });
    add("кости", [](const Message&, const auto&) {
        const int first = random_int(1, 6);
        const int second = random_int(1, 6);
        return Reply{"🎲 Выпало: " + std::to_string(first) + " + " + std::to_string(second) +
                     " = " + std::to_string(first + second)};
    });
    const auto add_die = [this](const char* name, int sides) {
        add(name, [sides](const Message&, const auto&) {
            return Reply{"🎲 d" + std::to_string(sides) + ": " + std::to_string(random_int(1, sides))};
        });
    };
    add_die("d4", 4);
    add_die("d6", 6);
    add_die("d8", 8);
    add_die("d10", 10);
    add_die("d12", 12);
    add_die("d20", 20);
    add_die("d100", 100);

    add("даилинет", [](const Message&, const auto&) {
        return Reply{random_int(0, 1) ? "✅ Да." : "❌ Нет."};
    });
    add("совместимость", [](const Message&, const auto& args) {
        if (args.size() < 2) return Reply{"Использование: совместимость имя1 имя2"};
        const auto pair = args[0] < args[1] ? args[0] + "|" + args[1] : args[1] + "|" + args[0];
        const auto score = std::hash<std::string>{}(pair) % 101U;
        return Reply{"💞 Совместимость " + args[0] + " и " + args[1] + ": " +
                     std::to_string(score) + "% (развлекательный расчёт)."};
    });
    add("переверни", [](const Message&, const auto& args) {
        if (args.empty()) return Reply{"Использование: переверни [текст]"};
        const auto input = join(args);
        std::vector<std::string> characters;
        for (std::size_t i = 0; i < input.size();) {
            const auto lead = static_cast<unsigned char>(input[i]);
            std::size_t length = lead < 0x80 ? 1 : (lead & 0xE0) == 0xC0 ? 2 :
                                 (lead & 0xF0) == 0xE0 ? 3 : (lead & 0xF8) == 0xF0 ? 4 : 1;
            if (i + length > input.size()) length = 1;
            characters.emplace_back(input.substr(i, length));
            i += length;
        }
        std::string output;
        for (auto it = characters.rbegin(); it != characters.rend(); ++it) output += *it;
        return Reply{"↩️ " + output};
    });
    add("повтори", [](const Message&, const auto& args) {
        if (args.empty()) return Reply{"Использование: повтори [текст]"};
        auto value = join(args);
        if (value.size() > 3000) value.resize(3000);
        return Reply{"🔁 " + value};
    });
    add("процент", [](const Message&, const auto& args) {
        if (args.size() != 2) return Reply{"Использование: процент [процент] [число]"};
        std::int64_t percent{}, number{};
        if (!parse_int(args[0], percent) || !parse_int(args[1], number))
            return Reply{"❌ Оба аргумента должны быть целыми числами."};
        const long double result = static_cast<long double>(percent) *
                                   static_cast<long double>(number) / 100.0L;
        std::ostringstream out;
        out.precision(2);
        out << std::fixed << "📊 " << percent << "% от " << number << " = " << result;
        return Reply{out.str()};
    });
    add("делить", [](const Message&, const auto& args) {
        if (args.size() != 2) return Reply{"Использование: делить [делимое] [делитель]"};
        std::int64_t a{}, b{};
        if (!parse_int(args[0], a) || !parse_int(args[1], b))
            return Reply{"❌ Укажи два целых числа."};
        if (b == 0) return Reply{"❌ На ноль делить нельзя."};
        std::ostringstream out;
        out.precision(10);
        out << "➗ " << a << " / " << b << " = "
            << static_cast<long double>(a) / static_cast<long double>(b);
        return Reply{out.str()};
    });
    add("остаток", [](const Message&, const auto& args) {
        if (args.size() != 2) return Reply{"Использование: остаток [число] [делитель]"};
        std::int64_t a{}, b{};
        if (!parse_int(args[0], a) || !parse_int(args[1], b) || b == 0)
            return Reply{"❌ Нужны два целых числа, делитель не должен быть нулём."};
        if (a == std::numeric_limits<std::int64_t>::min() && b == -1)
            return Reply{"➗ Остаток: 0"};
        return Reply{"➗ Остаток: " + std::to_string(a % b)};
    });
    add("степень", [](const Message&, const auto& args) {
        if (args.size() != 2) return Reply{"Использование: степень [основание] [показатель от 0 до 63]"};
        std::int64_t base{}, exponent{};
        if (!parse_int(args[0], base) || !parse_int(args[1], exponent) || exponent < 0 || exponent > 63)
            return Reply{"❌ Основание должно быть целым, показатель — от 0 до 63."};
        long double result = 1;
        for (std::int64_t i = 0; i < exponent; ++i) result *= static_cast<long double>(base);
        const long double signed_limit = std::ldexp(1.0L, 63);
        if (!std::isfinite(result) || result >= signed_limit || result < -signed_limit)
            return Reply{"❌ Результат выходит за диапазон 64-битного целого числа."};
        std::ostringstream out;
        out << "🔢 " << base << "^" << exponent << " = " << static_cast<std::int64_t>(result);
        return Reply{out.str()};
    });
    add("факториал", [](const Message&, const auto& args) {
        if (args.size() != 1) return Reply{"Использование: факториал [число от 0 до 20]"};
        std::int64_t n{};
        if (!parse_int(args[0], n) || n < 0 || n > 20)
            return Reply{"❌ Факториал поддерживает целые числа от 0 до 20."};
        std::uint64_t result = 1;
        for (std::uint64_t i = 2; i <= static_cast<std::uint64_t>(n); ++i) result *= i;
        return Reply{"🔢 " + std::to_string(n) + "! = " + std::to_string(result)};
    });
    add("простое", [](const Message&, const auto& args) {
        if (args.size() != 1) return Reply{"Использование: простое [число]"};
        std::int64_t n{};
        if (!parse_int(args[0], n) || n < 0) return Reply{"❌ Укажи неотрицательное целое число."};
        bool prime = n >= 2;
        for (std::int64_t divisor = 2; prime && divisor <= n / divisor; ++divisor)
            if (n % divisor == 0) prime = false;
        return Reply{prime ? "✅ Число простое." : "❌ Число составное или меньше двух."};
    });
    add("четное", [](const Message&, const auto& args) {
        if (args.size() != 1) return Reply{"Использование: четное [число]"};
        std::int64_t n{};
        if (!parse_int(args[0], n)) return Reply{"❌ Укажи целое число."};
        return Reply{n % 2 == 0 ? "🔢 Число чётное." : "🔢 Число нечётное."};
    });
    add("нечетное", [](const Message&, const auto& args) {
        if (args.size() != 1) return Reply{"Использование: нечетное [число]"};
        std::int64_t n{};
        if (!parse_int(args[0], n)) return Reply{"❌ Укажи целое число."};
        return Reply{n % 2 != 0 ? "🔢 Число нечётное." : "🔢 Число чётное."};
    });
    add("цветкод", [](const Message&, const auto&) {
        std::ostringstream out;
        out << "🎨 Случайный HEX-цвет: #" << std::uppercase << std::hex
            << std::setw(6) << std::setfill('0') << random_int(0, 0xFFFFFF);
        return Reply{out.str()};
    });
    add("uuid", [](const Message&, const auto&) {
        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (int i = 0; i < 32; ++i) {
            if (i == 8 || i == 12 || i == 16 || i == 20) out << '-';
            int digit = random_int(0, 15);
            if (i == 12) digit = 4;
            if (i == 16) digit = (digit & 0x3) | 0x8;
            out << digit;
        }
        return Reply{"🆔 UUID (случайный, не криптографический): " + out.str()};
    });

    add_choice("загадка", {"🧩 Что можно увидеть с закрытыми глазами? Ответ: сон.",
                           "🧩 Чем больше из неё берёшь, тем больше она становится? Ответ: яма.",
                           "🧩 Без рук, без ног, а рисовать умеет? Ответ: мороз."});
    add_choice("викторина", {"🧠 Сколько планет в Солнечной системе? Ответ: 8.",
                              "🧠 Как называется самая большая планета? Ответ: Юпитер.",
                              "🧠 Какой газ растения поглощают при фотосинтезе? Ответ: углекислый газ."});
    add_choice("слово", {"🌿 Рассвет", "🌊 Горизонт", "✨ Искра", "🌌 Созвездие", "🍃 Листопад"});
    add_choice("имя", {"Александр", "Михаил", "Артём", "София", "Анна", "Мария", "Алиса", "Ева"});
    add_choice("никнейм", {"PixelFox", "NightByte", "AetherWolf", "NeonLeaf", "QuietStorm", "NovaCore"});
    add_choice("планета", {"🪐 Меркурий", "🪐 Венера", "🌍 Земля", "🔴 Марс", "🪐 Юпитер", "🪐 Сатурн", "🪐 Уран", "🪐 Нептун"});
    add_choice("стихия", {"🔥 Огонь", "💧 Вода", "🌪 Воздух", "🌱 Земля", "⚡ Молния", "❄️ Лёд"});
    add_choice("эмодзи", {"😀", "🚀", "🎮", "🌌", "🍀", "🦊", "⚡", "💎", "🎯"});
    add_choice("квест", {"🗺 Квест: найди заброшенную башню и выясни, кто зажёг в ней свет.",
                          "🗺 Квест: доставь послание через лес до наступления темноты.",
                          "🗺 Квест: разгадай три символа на древних воротах."});
    add_choice("миссия", {"📜 Миссия: исследуй северный сектор и вернись с координатами.",
                          "📜 Миссия: защити караван на пути через перевал.",
                          "📜 Миссия: найди источник сигнала в старом комплексе."});
    add_choice("локация", {"🏞️ Локация: Туманный перевал", "🏰 Локация: Крепость на утёсе",
                            "🌲 Локация: Шёпот древнего леса", "🏜️ Локация: Стеклянная пустыня"});
    add_choice("оружие", {"⚔️ Оружие: стальной клинок", "🏹 Оружие: длинный лук",
                           "🔨 Оружие: молот грома", "🪄 Оружие: посох искр"});
    add_choice("броня", {"🛡 Броня: кожаный нагрудник", "🛡 Броня: кольчуга странника",
                         "🛡 Броня: обсидиановый щит", "🛡 Броня: плащ тумана"});
    add_choice("заклинание", {"✨ Заклинание: искра света", "🌪 Заклинание: порыв ветра",
                               "❄️ Заклинание: ледяная печать", "🔥 Заклинание: огненный круг"});
    add_choice("зелье", {"🧪 Зелье: малого исцеления", "🧪 Зелье: ясного разума",
                          "🧪 Зелье: ночного зрения", "🧪 Зелье: бодрости"});
    add_choice("ресурс", {"⛏️ Ресурс: железная руда", "💎 Ресурс: кварцевый кристалл",
                           "🌲 Ресурс: древняя древесина", "🪨 Ресурс: обсидиан"});
    add_choice("редкость", {"🟢 Обычный предмет", "🔵 Редкий предмет", "🟣 Эпический предмет", "🟡 Легендарный предмет"});
    add_choice("класс", {"🛡 Класс: страж", "🏹 Класс: следопыт", "🧙 Класс: маг",
                          "🗡 Класс: дуэлянт", "❤️ Класс: целитель"});
    add_choice("раса", {"🧝 Эльф", "⛏️ Дварф", "🧑 Человек", "🧚 Фейри", "🐉 Драконорождённый"});
    add_choice("способность", {"⚡ Способность: рывок", "🛡 Способность: отражение",
                                "🌫 Способность: маскировка", "✨ Способность: восстановление"});
    add_choice("персонаж", {"🧭 Картограф-исследователь", "🛡 Страж забытого города",
                             "🧙 Архивариус магической башни", "🏹 Следопыт северных земель"});
    add_choice("испытание", {"🏆 Испытание: пройди путь, не наступив на светлые плиты.",
                              "🏆 Испытание: найди выход из лабиринта за ограниченное время.",
                              "🏆 Испытание: победи тренировочного голема без подсказок."});
    add_choice("достижение", {"🏅 Достижение: Первый шаг", "🏅 Достижение: Исследователь",
                               "🏅 Достижение: Мастер головоломок", "🏅 Достижение: Не сдающийся"});
    add_choice("трофей", {"🏆 Трофей: Кубок искателя", "🏆 Трофей: Знак храбрости",
                           "🏆 Трофей: Медаль точности", "🏆 Трофей: Кристалл знаний"});
    add_choice("событие", {"🎉 Событие: ярмарка в старом городе",
                            "🌠 Событие: метеоритный дождь над долиной",
                            "🎭 Событие: маскарад в королевском дворце",
                            "🏁 Событие: гонка воздушных кораблей"});
    add_choice("погодаигра", {"🌤 Игровая погода: ясное небо", "🌧 Игровая погода: тёплый дождь",
                               "🌨 Игровая погода: снежная метель", "🌫 Игровая погода: густой туман"});


}

} // namespace vkaether
