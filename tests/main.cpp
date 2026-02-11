#include <cstdlib>
#include <iostream>

bool runTimeWindowResolverTests();
bool runBandAnalyzerTests();
bool runChannelViewsTests();

int main()
{
    const auto timeOk = runTimeWindowResolverTests();
    const auto bandOk = runBandAnalyzerTests();
    const auto channelOk = runChannelViewsTests();

    if (timeOk && bandOk && channelOk)
    {
        std::cout << "All tests passed." << std::endl;
        return EXIT_SUCCESS;
    }

    std::cerr << "At least one test suite failed." << std::endl;
    return EXIT_FAILURE;
}
