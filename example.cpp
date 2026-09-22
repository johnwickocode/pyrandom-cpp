#include <iostream>
#include <string>
#include "pyrandom.hpp"

struct Fighter {
    std::string name;
    int hp;
    int max_hp;
    int attack;
};

void show_status(const Fighter& player, const Fighter& boss) {
    std::cout << "\n"
              << player.name << ": " << player.hp << "/" << player.max_hp << " HP\n"
              << boss.name   << ": " << boss.hp << "/" << boss.max_hp << " HP\n";
}

int random_damage(int attack) {
    // 75%–125% of base attack.
    int min_damage = attack * 3 / 4;
    int max_damage = attack * 5 / 4;
    return pyrandom::randint(min_damage, max_damage);
}

void player_turn(Fighter& player, Fighter& boss, int& potions) {
    std::cout << "\nYour turn!\n"
              << "1. Attack\n"
              << "2. Heal (" << potions << " potion(s))\n"
              << "> ";

    int choice;
    std::cin >> choice;

    if (choice == 2 && potions > 0) {
        int healing = pyrandom::randint(18, 30);
        player.hp += healing;

        if (player.hp > player.max_hp)
            player.hp = player.max_hp;

        --potions;

        std::cout << "You drink a potion and recover "
                  << healing << " HP!\n";
        return;
    }

    if (choice != 1) {
        std::cout << "You hesitate! The boss gets an opening.\n";
        return;
    }

    int damage = random_damage(player.attack);

    // 20% critical-hit chance.
    if (pyrandom::chance(20)) {
        damage *= 2;
        std::cout << "CRITICAL HIT! ";
    }

    boss.hp -= damage;

    if (boss.hp < 0)
        boss.hp = 0;

    std::cout << "You hit " << boss.name
              << " for " << damage << " damage!\n";
}

void boss_turn(Fighter& player, Fighter& boss) {
    std::cout << "\n" << boss.name << "'s turn!\n";

    // The boss has three possible behaviors.
    int move = pyrandom::randint(1, 3);

    if (move == 1) {
        int damage = random_damage(boss.attack);
        player.hp -= damage;

        if (player.hp < 0)
            player.hp = 0;

        std::cout << boss.name << " swings its axe for "
                  << damage << " damage!\n";
    }
    else if (move == 2) {
        int damage = random_damage(boss.attack + 5);
        player.hp -= damage;

        if (player.hp < 0)
            player.hp = 0;

        std::cout << boss.name
                  << " unleashes a devastating strike for "
                  << damage << " damage!\n";
    }
    else {
        std::cout << boss.name
                  << " roars and prepares for its next attack!\n";
    }
}

int main() {
    // Seed pyrandom from the current time.
    pyrandom::seed();

    Fighter player{"Hero", 100, 100, 22};
    Fighter boss{"The Ancient Dragon", 160, 160, 18};

    int potions = 3;

    std::cout << "====================================\n";
    std::cout << "       THE ANCIENT DRAGON\n";
    std::cout << "====================================\n";
    std::cout << "A massive dragon blocks your path!\n";

    while (player.hp > 0 && boss.hp > 0) {
        show_status(player, boss);

        player_turn(player, boss, potions);

        if (boss.hp <= 0)
            break;

        boss_turn(player, boss);
    }

    std::cout << "\n====================================\n";

    if (player.hp > 0) {
        std::cout << "VICTORY!\n";
        std::cout << "The Ancient Dragon has been defeated!\n";
    } else {
        std::cout << "DEFEAT!\n";
        std::cout << "The dragon claims another challenger...\n";
    }

    std::cout << "====================================\n";
}
