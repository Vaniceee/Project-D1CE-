#pragma once

#include <random>

class Dice {
public:
    static int Roll();

private:
    static std::mt19937& GetGenerator();
};