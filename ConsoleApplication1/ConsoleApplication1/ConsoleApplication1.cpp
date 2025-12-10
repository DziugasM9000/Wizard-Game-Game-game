#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <limits>

using namespace std;

// ---------------- Configuration ----------------

namespace Config {
    constexpr int INITIAL_HEALTH = 100;
    constexpr int INITIAL_MANA = 50;

    constexpr int FIREBALL_DAMAGE = 25;
    constexpr int FIREBALL_COST = 10;

    constexpr int ICE_SPIKE_DAMAGE = 15;
    constexpr int ICE_SPIKE_COST = 6;

    constexpr int HEAL_AMOUNT = 20;
    constexpr int HEAL_COST = 8;

    constexpr int SHIELD_AMOUNT = 18;
    constexpr int SHIELD_COST = 7;

    constexpr int MANA_DRAIN_AMOUNT = 12;
    constexpr int MANA_DRAIN_COST = 5;

    constexpr int MANA_REGEN_PER_TURN = 4;

    constexpr int MANA_REGEN_SPELL_AMOUNT = 15;
    constexpr int MANA_REGEN_SPELL_COST = 1;
}

// forward declaration
class Spell;

// ---------------- Wizard -----------------------

class Wizard {
public:
    explicit Wizard(string name)
        : name_(std::move(name)),
        health_(Config::INITIAL_HEALTH),
        mana_(Config::INITIAL_MANA),
        shield_(0) {}

    const string& name() const { return name_; }
    int  health() const { return health_; }
    int  mana()   const { return mana_; }
    int  shield() const { return shield_; }
    bool isAlive() const { return health_ > 0; }

    bool hasEnoughMana(int cost) const {
        return mana_ >= cost;
    }

    void spendMana(int cost) {
        mana_ -= cost;
        if (mana_ < 0) mana_ = 0;
    }

    void regenerateMana(int amount) {
        mana_ += amount;
        if (mana_ > Config::INITIAL_MANA) {
            mana_ = Config::INITIAL_MANA;
        }
    }

    void receiveDamage(int amount) {
        if (amount <= 0 || !isAlive()) return;

        int remaining = amount;

        if (shield_ > 0) {
            int absorbed = min(shield_, remaining);
            shield_ -= absorbed;
            remaining -= absorbed;
        }

        if (remaining > 0) {
            health_ -= remaining;
            if (health_ < 0) {
                health_ = 0;
            }
        }
    }

    void heal(int amount) {
        if (!isAlive()) return;
        health_ += amount;
        if (health_ > Config::INITIAL_HEALTH) {
            health_ = Config::INITIAL_HEALTH;
        }
    }

    void addShield(int amount) {
        if (!isAlive()) return;
        shield_ += amount;
    }

    void changeMana(int delta) {
        mana_ += delta;
        if (mana_ < 0) mana_ = 0;
        if (mana_ > Config::INITIAL_MANA) {
            mana_ = Config::INITIAL_MANA;
        }
    }

    void addSpell(shared_ptr<Spell> spell) {
        spellBook_.push_back(spell);
    }

    const vector<shared_ptr<Spell>>& spellBook() const {
        return spellBook_;
    }

private:
    string name_;
    int health_;
    int mana_;
    int shield_;
    vector<shared_ptr<Spell>> spellBook_;
};

// ---------------- Spells -----------------------

class Spell {
public:
    Spell(string name, int manaCost)
        : name_(std::move(name)), manaCost_(manaCost) {}

    virtual ~Spell() = default;

    const string& name() const { return name_; }
    int manaCost() const { return manaCost_; }

    virtual void cast(Wizard& caster, Wizard& target) = 0;

protected:
    void payMana(Wizard& caster) const {
        caster.spendMana(manaCost_);
    }

private:
    string name_;
    int manaCost_;
};

class DamageSpell : public Spell {
public:
    DamageSpell(const string& name, int cost, int damage)
        : Spell(name, cost), damage_(damage) {}

    void cast(Wizard& caster, Wizard& target) override {
        if (!caster.hasEnoughMana(manaCost())) {
            cout << caster.name() << " does not have enough mana for "
                << name() << "!\n";
            return;
        }

        payMana(caster);
        target.receiveDamage(damage_);

        cout << caster.name() << " casts " << name()
            << " and deals " << damage_ << " damage.\n";
    }

private:
    int damage_;
};

class HealSpell : public Spell {
public:
    HealSpell(const string& name, int cost, int healAmount)
        : Spell(name, cost), healAmount_(healAmount) {}

    void cast(Wizard& caster, Wizard& target) override {
        (void)target; // unused

        if (!caster.hasEnoughMana(manaCost())) {
            cout << caster.name() << " does not have enough mana for "
                << name() << "!\n";
            return;
        }

        payMana(caster);
        caster.heal(healAmount_);

        cout << caster.name() << " casts " << name()
            << " and heals " << healAmount_ << " HP.\n";
    }

private:
    int healAmount_;
};

class ShieldSpell : public Spell {
public:
    ShieldSpell(const string& name, int cost, int shieldAmount)
        : Spell(name, cost), shieldAmount_(shieldAmount) {}

    void cast(Wizard& caster, Wizard& target) override {
        (void)target; // unused

        if (!caster.hasEnoughMana(manaCost())) {
            cout << caster.name() << " does not have enough mana for "
                << name() << "!\n";
            return;
        }

        payMana(caster);
        caster.addShield(shieldAmount_);

        cout << caster.name() << " casts " << name()
            << " and gains a shield of " << shieldAmount_
            << " points.\n";
    }

private:
    int shieldAmount_;
};

class ManaDrainSpell : public Spell {
public:
    ManaDrainSpell(const string& name, int cost, int drainAmount)
        : Spell(name, cost), drainAmount_(drainAmount) {}

    void cast(Wizard& caster, Wizard& target) override {
        if (!caster.hasEnoughMana(manaCost())) {
            cout << caster.name() << " does not have enough mana for "
                << name() << "!\n";
            return;
        }

        payMana(caster);

        int actualDrain = min(drainAmount_, target.mana());
        target.changeMana(-actualDrain);
        caster.changeMana(actualDrain / 2);

        cout << caster.name() << " casts " << name()
            << " and drains " << actualDrain
            << " mana from " << target.name()
            << " (half is restored to the caster).\n";
    }

private:
    int drainAmount_;
};

class ManaRegenSpell : public Spell {
public:
    ManaRegenSpell(const string& name, int cost, int manaRestore)
        : Spell(name, cost), manaRestore_(manaRestore) {}

    void cast(Wizard& caster, Wizard& target) override {
        (void)target; // not used

        if (!caster.hasEnoughMana(manaCost())) {
            cout << caster.name() << " does not have enough mana for "
                << name() << "!\n";
            return;
        }

        payMana(caster);
        caster.changeMana(manaRestore_);

        cout << caster.name() << " casts " << name()
            << " and restores " << manaRestore_
            << " mana.\n";
    }

private:
    int manaRestore_;
};

// --------------- SpellFactory ---------------

class SpellFactory {
public:
    static vector<shared_ptr<Spell>> createDefaultSpellBook() {
        vector<shared_ptr<Spell>> spells;

        spells.push_back(make_shared<DamageSpell>(
            "Fireball", Config::FIREBALL_COST, Config::FIREBALL_DAMAGE));

        spells.push_back(make_shared<DamageSpell>(
            "Ice Spike", Config::ICE_SPIKE_COST, Config::ICE_SPIKE_DAMAGE));

        spells.push_back(make_shared<HealSpell>(
            "Healing Light", Config::HEAL_COST, Config::HEAL_AMOUNT));

        spells.push_back(make_shared<ShieldSpell>(
            "Magic Shield", Config::SHIELD_COST, Config::SHIELD_AMOUNT));

        spells.push_back(make_shared<ManaDrainSpell>(
            "Mana Drain", Config::MANA_DRAIN_COST, Config::MANA_DRAIN_AMOUNT));

        spells.push_back(make_shared<ManaRegenSpell>(
            "Mana Surge",
            Config::MANA_REGEN_SPELL_COST,
            Config::MANA_REGEN_SPELL_AMOUNT));
        return spells;
    }
};

// --------------- Turn Strategy ---------------

class TurnStrategy {
public:
    virtual ~TurnStrategy() = default;

    virtual int chooseSpellIndex(
        const Wizard& self,
        const Wizard& opponent,
        const vector<shared_ptr<Spell>>& spells) = 0;
};

class HumanTurnStrategy : public TurnStrategy {
public:
    int chooseSpellIndex(
        const Wizard& self,
        const Wizard& opponent,
        const vector<shared_ptr<Spell>>& spells) override
    {
        printStatus(self, opponent);
        printMenu(self, spells);

        int choice = readInt(1, static_cast<int>(spells.size()));
        system("cls");
        return choice - 1;
    }

private:
    static void printStatus(const Wizard& self, const Wizard& opponent) {
        cout << "\n===== Duel =====\n";
        cout << self.name()
            << " | HP: " << self.health()
            << " | Mana: " << self.mana()
            << " | Shield: " << self.shield() << "\n";

        cout << opponent.name()
            << " | HP: " << opponent.health()
            << " | Mana: " << opponent.mana()
            << " | Shield: " << opponent.shield() << "\n\n";
    }

    static void printMenu(const Wizard& self,
        const vector<shared_ptr<Spell>>& spells)
    {
        cout << "Choose your spell:\n";
        for (size_t i = 0; i < spells.size(); ++i) {
            const auto& s = spells[i];
            cout << (i + 1) << ") " << s->name()
                << " (cost: " << s->manaCost() << " mana)";
            if (!self.hasEnoughMana(s->manaCost())) {
                cout << " [too expensive]";
            }
            cout << "\n";
        }
    }

    static int readInt(int min, int max) {
        int value;
        while (true) {
            cout << "Enter a number (" << min << "-" << max << "): ";
            cin >> value;

            if (!cin) {
                cin.clear();
                cin.ignore(numeric_limits<streamsize>::max(), '\n');
                cout << "Invalid input. Try again.\n";
                continue;
            }

            if (value < min || value > max) {
                cout << "Number must be between " << min
                    << " and " << max << ".\n";
                continue;
            }

            return value;
        }
    }
};

class AiTurnStrategy : public TurnStrategy {
public:
    int chooseSpellIndex(
        const Wizard& self,
        const Wizard& opponent,
        const vector<shared_ptr<Spell>>& spells) override
    {
        // maziau uz 40hp ir gali - pasihealina
        int healIndex = findByName(spells, "Healing Light");
        if (self.health() <= 40 &&
            healIndex != -1 &&
            self.hasEnoughMana(spells[healIndex]->manaCost()))
        {
            return healIndex;
        }

        // jei neturi skydo ir gali, skydas
        int shieldIndex = findByName(spells, "Magic Shield");
        if (self.shield() == 0 &&
            shieldIndex != -1 &&
            self.hasEnoughMana(spells[shieldIndex]->manaCost()))
        {
            return shieldIndex;
        }

        // Jei priesininkas turi daug manos - drain
        int drainIndex = findByName(spells, "Mana Drain");
        if (opponent.mana() >= 15 &&
            drainIndex != -1 &&
            self.hasEnoughMana(spells[drainIndex]->manaCost()))
        {
            return drainIndex;
        }

        // kitu atveju pigiausias spell
        int damageIndex = findCheapestAffordableDamage(spells, self);
        if (damageIndex != -1) return damageIndex;

        // 5) Fallback: cheapest spell in general
        return findCheapestSpell(spells);
    }

private:
    static int findByName(const vector<shared_ptr<Spell>>& spells,
        const string& name)
    {
        for (size_t i = 0; i < spells.size(); ++i) {
            if (spells[i]->name() == name) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    static int findCheapestAffordableDamage(
        const vector<shared_ptr<Spell>>& spells,
        const Wizard& self)
    {
        int bestIndex = -1;
        int bestCost = numeric_limits<int>::max();

        for (size_t i = 0; i < spells.size(); ++i) {
            const auto& s = spells[i];
            bool isDamage = (s->name() == "Fireball" ||
                s->name() == "Ice Spike");
            if (!isDamage) continue;
            if (!self.hasEnoughMana(s->manaCost())) continue;

            if (s->manaCost() < bestCost) {
                bestCost = s->manaCost();
                bestIndex = static_cast<int>(i);
            }
        }
        return bestIndex;
    }

    static int findCheapestSpell(const vector<shared_ptr<Spell>>& spells) {
        int bestIndex = 0;
        int bestCost = spells[0]->manaCost();

        for (size_t i = 1; i < spells.size(); ++i) {
            if (spells[i]->manaCost() < bestCost) {
                bestCost = spells[i]->manaCost();
                bestIndex = static_cast<int>(i);
            }
        }
        return bestIndex;
    }
};

// GAME

class Game {
public:
    Game()
        : player_("Player"),
        enemy_("Enemy"),
        playerStrategy_(make_unique<HumanTurnStrategy>()),
        enemyStrategy_(make_unique<AiTurnStrategy>())
    {
        auto playerSpells = SpellFactory::createDefaultSpellBook();
        for (auto& s : playerSpells) {
            player_.addSpell(s);
        }

        auto enemySpells = SpellFactory::createDefaultSpellBook();
        for (auto& s : enemySpells) {
            enemy_.addSpell(s);
        }
    }

    void run() {
        cout << "=== Wizard Duel ===\n";

        while (player_.isAlive() && enemy_.isAlive()) {
            playTurn(player_, enemy_, *playerStrategy_);
            if (!enemy_.isAlive()) break;

            playTurn(enemy_, player_, *enemyStrategy_);
        }

        printResult();
    }

private:
    void playTurn(Wizard& current,
        Wizard& opponent,
        TurnStrategy& strategy)
    {
        cout << "\n--- " << current.name() << "'s turn ---\n";

        const auto& spells = current.spellBook();
        if (spells.empty()) {
            cout << current.name() << " has no spells!\n";
            return;
        }

        int index = strategy.chooseSpellIndex(current, opponent, spells);

        if (index < 0 || index >= static_cast<int>(spells.size())) {
            cout << "Invalid spell index. Turn skipped.\n";
            return;
        }

        spells[static_cast<size_t>(index)]->cast(current, opponent);

        current.regenerateMana(Config::MANA_REGEN_PER_TURN);
        cout << current.name() << " regenerates "
            << Config::MANA_REGEN_PER_TURN << " mana.\n";
    }

    void printResult() const {
        cout << "\n=== Duel Over ===\n";
        if (player_.isAlive() && !enemy_.isAlive()) {
            cout << "Player wins!\n";
        }
        else if (!player_.isAlive() && enemy_.isAlive()) {
            cout << "Enemy wins!\n";
        }
        else {
            cout << "Both wizards have fallen. It's a draw.\n";
        }
    }

    Wizard player_;
    Wizard enemy_;
    unique_ptr<TurnStrategy> playerStrategy_;
    unique_ptr<TurnStrategy> enemyStrategy_;
};

// MAIN

int main() {
    Game game;
    game.run();
    return 0;
}
