#include "vkaether/commands.hpp"
#include <algorithm>
#include <charconv>
#include <cctype>
#include <random>
namespace vkaether {
namespace {
std::vector<std::string> split(std::string_view s){std::vector<std::string> v;std::string t;bool quote=false;for(char c:s){if(c=='"'){quote=!quote;continue;}if(!quote&&(c==' '||c=='\\t'||c=='\\n')){if(!t.empty()){v.push_back(std::move(t));t.clear();}}else t.push_back(c);}if(!t.empty())v.push_back(std::move(t));return v;}
bool parse(std::string_view s,std::int64_t& n){auto[p,e]=std::from_chars(s.data(),s.data()+s.size(),n);return e==std::errc{}&&p==s.data()+s.size();}
}
void CommandRegistry::add(std::string n,Handler h){commands_.insert_or_assign(std::move(n),std::move(h));}
CommandRegistry::CommandRegistry(Database& db):database_(db){
 auto help=[](const Message&,const auto&){return Reply{"🎮 Aether Arcade\\nКоманды: помощь, профиль, баланс, бонус, работа, топ, монетка, кубик, rps, передать.\\nПоддерживаются команды с / . ! или без префикса."};};
 add("помощь",help);add("команды",help);add("help",help);
 add("профиль",[this](const Message&m,const auto&){auto u=database_.get_user(m.from_id);return Reply{"👤 Профиль\\nID: "+std::to_string(u.id)+"\\nБаланс: "+std::to_string(u.balance)+"\\nУровень: "+std::to_string(u.level)+"\\nОпыт: "+std::to_string(u.experience)};});
 add("баланс",[this](const Message&m,const auto&){return Reply{"💰 Баланс: "+std::to_string(database_.get_user(m.from_id).balance)+" монет."};});
 add("бонус",[this](const Message&m,const auto&){auto n=database_.daily_bonus(m.from_id,500);return Reply{n?"🎁 Ежедневный бонус: +500 монет!":"⏳ Бонус уже получен сегодня."};});
 add("работа",[this](const Message&m,const auto&){database_.work(m.from_id,250);return Reply{"🧰 Смена завершена: +250 монет и +2 опыта."};});
 add("топ",[this](const Message&,const auto&){return Reply{"🏆 Топ игроков\\n"+database_.top(10)};});
 add("монетка",[](const Message&,const auto&){static thread_local std::mt19937_64 r(std::random_device{}());return Reply{(r()&1U)?"🪙 Орёл!":"🪙 Решка!"};});
 add("кубик",[](const Message&,const auto&){static thread_local std::mt19937 r(std::random_device{}());return Reply{"🎲 Выпало: "+std::to_string(std::uniform_int_distribution<int>(1,6)(r))};});
 add("передать",[this](const Message&m,const auto&a){if(a.size()!=2)return Reply{"Использование: передать [ID] [сумма]"};std::int64_t id{},sum{};if(!parse(a[0],id)||!parse(a[1],sum)||id<=0||sum<=0)return Reply{"Некорректные ID или сумма."};return Reply{database_.transfer(m.from_id,id,sum)?"💸 Перевод выполнен.":"❌ Недостаточно средств или неверные параметры."};});
 add("rps",[](const Message&,const auto&a){if(a.empty())return Reply{"Использование: rps камень|ножницы|бумага"};const char* c[]={"камень","ножницы","бумага"};int u=-1;for(int i=0;i<3;++i)if(a[0]==c[i])u=i;if(u<0)return Reply{"Выбери камень, ножницы или бумагу."};static thread_local std::mt19937 r(std::random_device{}());int b=std::uniform_int_distribution<int>(0,2)(r);return Reply{"Ты: "+a[0]+"\\nБот: "+c[b]+"\\n"+(u==b?"Ничья!":((u-b+3)%3==1?"Ты победил!":"Победил бот."))};});
 const std::pair<const char*,const char*> extra[]={
 {"магазин","🛒 Магазин готовится."},{"инвентарь","🎒 Инвентарь пока пуст."},{"предметы","📦 Каталог предметов готовится."},{"питомец","🐾 Питомцев пока нет."},{"питомцы","🐾 Система питомцев готовится."},{"ферма","🌾 Ферма готовится."},{"бизнес","🏢 Бизнес-система готовится."},{"работы","Используй: работа."},{"казино","🎰 Казино готовится."},{"слоты","🎰 Слоты готовятся."},{"рулетка","🎡 Рулетка готовится."},{"дуэль","⚔️ Дуэли готовятся."},{"репутация","⭐ Репутационная система готовится."},{"жениться","💍 Система отношений готовится."},{"брак","💍 Система отношений готовится."},{"клан","🛡️ Кланы готовятся."},{"кланы","🛡️ Кланы готовятся."},{"рыбалка","🎣 Рыбалка готовится."},{"охота","🏹 Охота готовится."},{"шахта","⛏️ Шахта готовится."},{"банк","🏦 Банк готовится."},{"депозит","🏦 Депозиты готовятся."},{"перевод","Используй: передать [ID] [сумма]."},{"ник","✏️ Смена ника готовится."},{"аватар","🖼️ Настройка аватара готовится."},{"статистика","📊 Используй: профиль."},{"онлайн","🟢 VK Long Poll активен."},{"пинг","🏓 pong"},{"версия","Aether Arcade 0.1.0 (C++20)"},{"правила","📜 Соблюдай правила сообщества."},{"ежедневный","Используй: бонус."},{"daily","Use: бонус."},{"profile","Use: профиль."},{"balance","Use: баланс."},{"bonus","Use: бонус."},{"work","Use: работа."},{"top","Use: топ."},{"coin","Use: монетка."},{"dice","Use: кубик."},{"shop","Use: магазин."},{"inventory","Use: инвентарь."},{"pet","Use: питомец."},{"clan","Use: клан."},{"duel","Use: дуэль."},{"fish","Use: рыбалка."},{"hunt","Use: охота."},{"bank","Use: банк."},{"ping","pong"},{"stats","Use: профиль."},{"rules","Read the community rules."},{"commands","Use: помощь."}};
 for(auto [n,r]:extra)add(n,[r](const Message&,const auto&){return Reply{r};});
}
Reply CommandRegistry::dispatch(const Message&m)const{auto a=split(m.text);if(a.empty())return{};auto n=a.front();while(!n.empty()&&(n.front()=='/'||n.front()=='.'||n.front()=='!'||n.front()==';'||n.front()==':'))n.erase(n.begin());if(n.empty())return{};std::transform(n.begin(),n.end(),n.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});auto it=commands_.find(n);if(it==commands_.end())return{};a.erase(a.begin());return it->second(m,a);}
}