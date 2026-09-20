#include "fake/Fixture.hpp"
#include <clocale>

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    if (argc > 1)
    {
        if (!std::setlocale(LC_NUMERIC, argv[1]) || std::localeconv()->decimal_point[0] != ',') return 2;
    }
    return RUN_ALL_TESTS();
}
