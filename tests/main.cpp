#include <cstdlib>
#include <iostream>

bool runTimeWindowResolverTests();
bool runBandAnalyzerTests();
bool runChannelViewsTests();
bool runAnalysisRingBufferTests();
bool runLoopClockTests();

int main()
{
    const auto ringOk = runAnalysisRingBufferTests();
    const auto clockOk = runLoopClockTests();
    const auto timeOk = runTimeWindowResolverTests();
    const auto bandOk = runBandAnalyzerTests();
    const auto channelOk = runChannelViewsTests();

    if (ringOk && clockOk && timeOk && bandOk && channelOk)
    {
        std::cout << "All tests passed." << std::endl;
        return EXIT_SUCCESS;
    }

    std::cerr << "At least one test suite failed." << std::endl;
    return EXIT_FAILURE;
}
