#include "vkaether/commands.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cctype>
#include <limits>
#include <random>
#include <sstream>
#include <string_view>
#include <utility>

namespace vkaether {
namespace {

std::vector<std::string> split(std::string_view s) {
    std::vector<std::string> values;
    std::string token;
    bool quoted = false;
    for (char c : s) {
        if (c == '"') {
            quoted = !quoted;
            continue;
        }
        if (!quoted && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
            if (!token.empty()) {
                values.push_back(std::move(token));
                token.clear();
            }
        } else {
            token.push_back(c);
        }
    }
    if (!token.empty()) values.push_back(std::move(token));
    return values;
}

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

void CommandRegistry::add(std::string name, Handler handler) {
    commands_.insert_or_assign(std::move(name), std::move(handler));
}

CommandRegistry::CommandRegistry(Database& db) : database_(db) {
    const auto help = [this](const Message&, const auto&) {
        return Reply{
            "🎮 Aether Arcade — игровые команды\n"
            "Экономика: профиль, баланс, бонус, работа, топ, передать [ID] [сумма]\n"
            "Игры: монетка, кубик, rps, число, шанс, выбрать, угадай, дуэль, рыбалка, охота, шахта\n"
            "Мини-игры: казино, слоты, рулетка, дартс, баскетбол, футбол, боулинг, лотерея, битва, босс\n"
            "Утилиты: эхо, длина, сумма, умножить, квадрат, корень, кто, рандом\n"
            "Развлечения: предсказание, гороскоп, анекдот, факт, комплимент, настроение, совет\n"
            "Команды принимают префиксы / . ! ; : или вводятся без них."
        };
    };
    add("помощь", help);
    add("команды", help);
    add("help", help);

    add("профиль", [this](const Message& message, const auto&) {
        const auto user = database_.get_user(message.from_id);
        return Reply{"👤 Профиль\nID: " + std::to_string(user.id) +
                     "\nБаланс: " + std::to_string(user.balance) +
                     "\nУровень: " + std::to_string(user.level) +
                     "\nОпыт: " + std::to_string(user.experience)};
    });
    add("баланс", [this](const Message& message, const auto&) {
        return Reply{"💰 Баланс: " + std::to_string(database_.get_user(message.from_id).balance) + " монет."};
    });
    add("бонус", [this](const Message& message, const auto&) {
        const auto amount = database_.daily_bonus(message.from_id, 500);
        return Reply{amount ? "🎁 Ежедневный бонус: +" + std::to_string(amount) + " монет!"
                            : "⏳ Ежедневный бонус уже получен. Возвращайся завтра."};
    });
    add("ежедневный", [this](const Message& message, const auto& args) {
        return commands_.at("бонус")(message, args);
    });
    add("daily", [this](const Message& message, const auto& args) {
        return commands_.at("бонус")(message, args);
    });
    add("работа", [this](const Message& message, const auto&) {
        database_.work(message.from_id, 250);
        return Reply{"🧰 Смена завершена: +250 монет и +2 опыта."};
    });
    add("work", [this](const Message& message, const auto& args) {
        return commands_.at("работа")(message, args);
    });
    add("топ", [this](const Message&, const auto&) {
        return Reply{"🏆 Топ игроков\n" + database_.top(10)};
    });
    add("top", [this](const Message& message, const auto& args) {
        return commands_.at("топ")(message, args);
    });
    add("статистика", [this](const Message& message, const auto& args) {
        return commands_.at("профиль")(message, args);
    });
    add("stats", [this](const Message& message, const auto& args) {
        return commands_.at("профиль")(message, args);
    });
    add("передать", [this](const Message& message, const auto& args) {
        if (args.size() != 2) return Reply{"Использование: передать [ID] [сумма]"};
        std::int64_t target{}, amount{};
        if (!parse_int(args[0], target) || !parse_int(args[1], amount) || target <= 0 || amount <= 0)
            return Reply{"❌ Укажи корректные ID получателя и положительную сумму."};
        return Reply{database_.transfer(message.from_id, target, amount)
                         ? "💸 Перевод выполнен."
                         : "❌ Перевод не выполнен: проверь баланс, ID и сумму."};
    });
    add("перевод", [this](const Message& message, const auto& args) {
        return commands_.at("передать")(message, args);
    });

    add("монетка", [](const Message&, const auto&) {
        return Reply{random_int(0, 1) ? "🪙 Орёл!" : "🪙 Решка!"};
    });
    add("coin", [](const Message&, const auto&) {
        return Reply{random_int(0, 1) ? "🪙 Heads!" : "🪙 Tails!"};
    });
    add("кубик", [](const Message&, const auto&) {
        return Reply{"🎲 Выпало: " + std::to_string(random_int(1, 6))};
    });
    add("dice", [](const Message&, const auto&) {
        return Reply{"🎲 Rolled: " + std::to_string(random_int(1, 6))};
    });
    add("rps", [](const Message&, const auto& args) {
        if (args.empty()) return Reply{"Использование: rps камень|ножницы|бумага"};
        static constexpr const char* choices[]{"камень", "ножницы", "бумага"};
        int player = -1;
        for (int i = 0; i < 3; ++i) if (args.front() == choices[i]) player = i;
        if (player < 0) return Reply{"Выбери: камень, ножницы или бумага."};
        const int bot = random_int(0, 2);
        const std::string result = player == bot ? "Ничья!" :
            ((player - bot + 3) % 3 == 1 ? "Ты победил!" : "Победил бот.");
        return Reply{"Ты: " + args.front() + "\nБот: " + choices[bot] + "\n" + result};
    });

    add("число", [](const Message&, const auto& args) {
        int low = 1, high = 100;
        if (args.size() == 2) {
            std::int64_t a{}, b{};
            if (!parse_int(args[0], a) || !parse_int(args[1], b) ||
                a < std::numeric_limits<int>::min() || a > std::numeric_limits<int>::max() ||
                b < std::numeric_limits<int>::min() || b > std::numeric_limits<int>::max() ||
                a >= b || (b - a) > 1000000)
                return Reply{"Использование: число [от] [до], диапазон не более 1 000 000."};
            low = static_cast<int>(a);
            high = static_cast<int>(b);
        } else if (!args.empty()) {
            return Reply{"Использование: число [от] [до]. По умолчанию: 1–100."};
        }
        return Reply{"🔢 Случайное число: " + std::to_string(random_int(low, high))};
    });
    add("шанс", [](const Message&, const auto&) {
        return Reply{"🎯 Вероятность успеха: " + std::to_string(random_int(0, 100)) + "%"};
    });
    add("выбрать", [](const Message&, const auto& args) {
        if (args.size() < 2) return Reply{"Использование: выбрать вариант1 вариант2 [вариант3 ...]"};
        return Reply{"🤔 Мой выбор: " + args[static_cast<std::size_t>(random_int(0, static_cast<int>(args.size() - 1)))]};
    });
    add("кто", [](const Message&, const auto& args) {
        if (args.empty()) return Reply{"Использование: кто имя1 имя2 [имя3 ...]"};
        return Reply{"👑 Сегодня удача на стороне: " +
                     args[static_cast<std::size_t>(random_int(0, static_cast<int>(args.size() - 1)))]};
    });
    add("эхо", [](const Message&, const auto& args) {
        return Reply{args.empty() ? "Напиши текст после команды: эхо [текст]." : "🗣 " + join(args)};
    });
    add("длина", [](const Message&, const auto& args) {
        if (args.empty()) return Reply{"Использование: длина [текст]"};
        const auto value = join(args);
        return Reply{"📏 Длина текста: " + std::to_string(value.size()) + " байт."};
    });
    add("сумма", [](const Message&, const auto& args) {
        if (args.size() < 2) return Reply{"Использование: сумма число1 число2 [число3 ...]"};
        std::int64_t total = 0;
        for (const auto& arg : args) {
            std::int64_t value{};
            if (!parse_int(arg, value) ||
                (value > 0 && total > std::numeric_limits<std::int64_t>::max() - value) ||
                (value < 0 && total < std::numeric_limits<std::int64_t>::min() - value))
                return Reply{"❌ Нужны целые числа без переполнения."};
            total += value;
        }
        return Reply{"➕ Сумма: " + std::to_string(total)};
    });
    add("умножить", [](const Message&, const auto& args) {
        if (args.size() != 2) return Reply{"Использование: умножить число1 число2"};
        std::int64_t a{}, b{};
        if (!parse_int(args[0], a) || !parse_int(args[1], b))
            return Reply{"❌ Укажи два целых числа."};
        bool overflow = false;
        if (a > 0) {
            overflow = b > 0 ? a > std::numeric_limits<std::int64_t>::max() / b
                             : b < std::numeric_limits<std::int64_t>::min() / a;
        } else if (a < 0) {
            overflow = b > 0 ? a < std::numeric_limits<std::int64_t>::min() / b
                             : (b < 0 && a < std::numeric_limits<std::int64_t>::max() / b);
        }
        if (overflow) return Reply{"❌ Результат выходит за диапазон 64-битного целого числа."};
        return Reply{"✖️ Результат: " + std::to_string(a * b)};
    });
    add("квадрат", [](const Message&, const auto& args) {
        if (args.size() != 1) return Reply{"Использование: квадрат число"};
        std::int64_t value{};
        if (!parse_int(args[0], value)) return Reply{"❌ Укажи целое число."};
        if (value != 0 && (value == std::numeric_limits<std::int64_t>::min() ||
            std::abs(value) > 3037000499LL))
            return Reply{"❌ Квадрат выходит за диапазон 64-битного целого числа."};
        return Reply{"🔢 Квадрат: " + std::to_string(value * value)};
    });
    add("корень", [](const Message&, const auto& args) {
        if (args.size() != 1) return Reply{"Использование: корень число"};
        std::int64_t value{};
        if (!parse_int(args[0], value) || value < 0) return Reply{"❌ Укажи неотрицательное целое число."};
        std::ostringstream output;
        output.precision(10);
        output << "√" << value << " = " << std::sqrt(static_cast<double>(value));
        return Reply{output.str()};
    });

    const auto add_random = [this](const char* name, std::vector<std::string> outcomes) {
        add(name, [outcomes = std::move(outcomes)](const Message&, const auto&) {
            const auto index = static_cast<std::size_t>(random_int(0, static_cast<int>(outcomes.size() - 1)));
            return Reply{outcomes[index]};
        });
    };

    add_random("угадай", {"🧩 Я загадал число от 1 до 10. Попробуй угадать: напиши число!",
                           "🧩 Загадано число от 1 до 10. Твоя попытка — команда «число» не поможет ;)",
                           "🧩 Число уже загадано: от 1 до 10. Сможешь назвать его?"});
    add_random("дуэль", {"⚔️ Дуэль окончена: победа за тобой!", "⚔️ Соперник оказался быстрее. В следующий раз повезёт!",
                         "⚔️ Оба бойца промахнулись. Ничья!"});
    add_random("рыбалка", {"🎣 Улов: золотая рыбка! Отличный день.", "🎣 Поймана большая щука!", "🎣 Рыба сорвалась с крючка.",
                           "🎣 Сегодня клёв слабый, но опыт получен."});
    add_random("охота", {"🏹 Удачная охота! Следы привели к добыче.", "🏹 Добыча ушла — попробуй ещё раз.",
                         "🏹 Ты нашёл редкий след в лесу."});
    add_random("шахта", {"⛏️ В породе нашлась медная жила.", "⛏️ Ты добыл немного железа.", "⛏️ Редкая находка: кусочек кристалла!",
                         "⛏️ Обвал! Хорошо, что ты успел отойти."});
    add_random("казино", {"🎰 Сегодня казино не на твоей стороне. Попробуй ещё раз.", "🎰 Почти джекпот! Не хватило одного символа.",
                          "🎰 Три символа совпали — виртуальный джекпот!"});
    add_random("слоты", {"🎰 🍒 | 🍋 | 🍒 — мимо!", "🎰 ⭐ | ⭐ | ⭐ — джекпот!", "🎰 🍀 | 🍀 | 💎 — почти!"});
    add_random("рулетка", {"🎡 Шар остановился на красном.", "🎡 Шар остановился на чёрном.", "🎡 Шар остановился на зелёном зеро."});
    add_random("дартс", {"🎯 Попадание в яблочко — 50 очков!", "🎯 Отличный бросок — 35 очков.", "🎯 Мимо мишени. Ещё попытка!"});
    add_random("баскетбол", {"🏀 Трёхочковый! Попал точно в кольцо.", "🏀 Данк! Зал получил удовольствие.", "🏀 Мяч отскочил от кольца."});
    add_random("футбол", {"⚽ ГОООЛ! Мощный удар в девятку.", "⚽ Вратарь отбил пенальти.", "⚽ Удар прошёл рядом со штангой."});
    add_random("боулинг", {"🎳 Страйк! Все кегли сбиты.", "🎳 Спэр! Осталась только одна кегля.", "🎳 Не лучший бросок — попробуй ещё."});
    add_random("лотерея", {"🎟️ Удача рядом: один номер не совпал.", "🎟️ Поздравляем! Воображаемый суперприз твой.",
                           "🎟️ Билет не выиграл. Попробуй в следующий раз."});
    add_random("предсказание", {"🔮 Сегодня хороший день для смелых идей.", "🔮 Скоро тебя ждёт приятный сюрприз.",
                                 "🔮 Не торопись с решением — скоро появится ясность."});
    add_random("гороскоп", {"♈ Овен: действуй решительно, но проверяй детали.", "♉ Телец: удели время отдыху.",
                            "♊ Близнецы: разговор принесёт полезную идею.", "♋ Рак: доверься здравому смыслу.",
                            "♌ Лев: прояви инициативу.", "♍ Дева: порядок поможет добиться цели.",
                            "♎ Весы: найди баланс между делами и отдыхом.", "♏ Скорпион: не раскрывай все планы.",
                            "♐ Стрелец: попробуй новый маршрут.", "♑ Козерог: последовательность даст результат.",
                            "♒ Водолей: нестандартный подход сработает.", "♓ Рыбы: прислушайся к интуиции."});
    add_random("анекдот", {"— Доктор, я всё время разговариваю сам с собой. — И давно? — С тех пор как стал работать удалённо.",
                           "Программист: «Работает на моей машине» — это не баг, это описание окружения.",
                           "— Почему сервер не спал? — Потому что у него были активные процессы."});
    add_random("факт", {"🧠 У осьминога три сердца.", "🧠 Мёд при правильном хранении может сохраняться очень долго.",
                        "🧠 У бананов ботанически ягоды, а клубника — нет."});
    add_random("комплимент", {"✨ У тебя отличный вкус на интересные идеи.", "✨ С тобой даже сложные задачи выглядят решаемыми.",
                               "✨ Твоя настойчивость заслуживает уважения."});
    add_random("настроение", {"🌤 Настроение дня: спокойно и продуктивно.", "⚡ Настроение дня: энергия на максимум.",
                              "🌙 Настроение дня: не спеши, всё успеется."});
    add_random("совет", {"💡 Разбей большую задачу на маленькие проверяемые шаги.", "💡 Сначала проверь самое простое и вероятное.",
                         "💡 Делай резервные копии до рискованных изменений."});
    add_random("профессия", {"🧑‍💻 Сегодня ты разработчик игровых миров.", "🧭 Сегодня ты исследователь неизведанного.",
                              "🛠 Сегодня ты инженер решений."});
    add_random("животное", {"🐺 Волк", "🦊 Лиса", "🐼 Панда", "🦉 Сова", "🐬 Дельфин", "🐉 Дракон"});
    add_random("цвет", {"🎨 Бирюзовый", "🎨 Изумрудный", "🎨 Графитовый", "🎨 Янтарный", "🎨 Лавандовый"});
    add_random("монстр", {"👾 Слайм — 10 HP", "👹 Каменный голем — 80 HP", "🐉 Дракон — 250 HP"});
    add_random("сокровище", {"💎 Ты нашёл старинную монету.", "🗝 Найден ключ от забытого сундука.",
                             "📦 Сундук оказался пустым. Но крышка красивая."});
    add_random("путешествие", {"🗺 Ты отправился в затерянный город.", "🚀 Экспедиция достигла новой планеты.",
                                "🏝 Ты обнаружил тихий остров."});
    add_random("магия", {"✨ Заклинание сработало: вокруг вспыхнули искры.", "🪄 Магия дала сбой — палочка чихнула.",
                         "🔮 Ты получил +1 к удаче на воображаемый день."});
    add_random("битва", {"⚔️ Победа! Противник повержен.", "🛡 Ты успешно заблокировал атаку.", "💥 Бой завершился вничью."});
    add_random("босс", {"🐲 Босс повержен! Команда ликует.", "👹 Босс призвал подкрепление — битва продолжается.",
                        "🛡 Босс нанёс мощный удар. Нужна стратегия!"});
    add_random("арена", {"🏟️ Арена встречает нового чемпиона!", "🏟️ Ты прошёл квалификацию.", "🏟️ Сегодня сильнее оказался соперник."});
    add_random("крит", {"💥 Критический удар! Урон x2.", "⚔️ Обычная атака — без критического эффекта.", "✨ Критический шанс сработал!"});
    add_random("защита", {"🛡️ Блок успешен: урон предотвращён.", "🛡️ Щит выдержал удар.", "🛡️ Защита пробита — отступи!"});
    add_random("атака", {"⚔️ Сильная атака: цель отброшена!", "⚔️ Быстрый удар достиг цели.", "⚔️ Противник увернулся."});
    add("рандом", [](const Message&, const auto&) {
        return Reply{"🎲 Случайное число: " + std::to_string(random_int(1, 100))};
    });


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
        if (!std::isfinite(result) || result > static_cast<long double>(std::numeric_limits<std::int64_t>::max()) ||
            result < static_cast<long double>(std::numeric_limits<std::int64_t>::min()))
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

    // English aliases intentionally point to the same real handlers.
    add("profile", [this](const Message& m, const auto& a) { return commands_.at("профиль")(m, a); });
    add("balance", [this](const Message& m, const auto& a) { return commands_.at("баланс")(m, a); });
    add("bonus", [this](const Message& m, const auto& a) { return commands_.at("бонус")(m, a); });
    add("shop", [this](const Message&, const auto&) { return Reply{"🛒 Экономический магазин появится в следующем обновлении. Баланс и переводы уже работают."}; });
    add("inventory", [this](const Message& m, const auto& a) { return commands_.at("профиль")(m, a); });
    add("pet", [](const Message&, const auto&) { return Reply{"🐾 Система питомцев ещё не включена."}; });
    add("clan", [](const Message&, const auto&) { return Reply{"🛡️ Система кланов ещё не включена."}; });
    add("duel", [this](const Message& m, const auto& a) { return commands_.at("дуэль")(m, a); });
    add("fish", [this](const Message& m, const auto& a) { return commands_.at("рыбалка")(m, a); });
    add("hunt", [this](const Message& m, const auto& a) { return commands_.at("охота")(m, a); });
    add("bank", [this](const Message& m, const auto& a) { return commands_.at("баланс")(m, a); });
    add("ping", [](const Message&, const auto&) { return Reply{"🏓 pong"}; });
    add("online", [](const Message&, const auto&) { return Reply{"🟢 VK Long Poll подключён."}; });
    add("пинг", [](const Message&, const auto&) { return Reply{"🏓 pong"}; });
    add("версия", [](const Message&, const auto&) { return Reply{"Aether Arcade 0.2.0 (C++20)"}; });
    add("правила", [](const Message&, const auto&) { return Reply{"📜 Играй честно, уважай других участников и не используй уязвимости."}; });
    add("rules", [](const Message&, const auto&) { return Reply{"📜 Play fair and respect other players."}; });
}

Reply CommandRegistry::dispatch(const Message& message) const {
    auto args = split(message.text);
    if (args.empty()) return {};
    auto name = args.front();
    while (!name.empty() && (name.front() == '/' || name.front() == '.' || name.front() == '!' ||
                             name.front() == ';' || name.front() == ':')) name.erase(name.begin());
    if (name.empty()) return {};
    std::transform(name.begin(), name.end(), name.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const auto command = commands_.find(name);
    if (command == commands_.end()) return {};
    args.erase(args.begin());
    return command->second(message, args);
}

} // namespace vkaether
