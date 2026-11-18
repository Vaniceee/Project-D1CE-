#include "Dice.h"
#include <random>

// Генератор випадкових чисел
std::mt19937& Dice::GetGenerator() {
    static std::mt19937 generator(std::random_device{}());
    return generator;
}

// Кидок одного нечесного кубика
int Dice::Roll() {
    static std::discrete_distribution<int> distribution{
        9, 12, 12, 9, 9, 9
    };

    return distribution(GetGenerator()) + 1;
}