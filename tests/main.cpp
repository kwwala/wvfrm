#include <cstdlib>
#include <iostream>

bool runTimeWindowResolverTests();
bool runBandAnalyzerTests();
bool runChannelViewsTests();
bool runAnalysisRingBufferTests();
bool runLoopClockTests();
bool runParametersTests();
bool runThemeEngineTests();

int main()
{
    const auto ringOk = runAnalysisRingBufferTests();
    const auto clockOk = runLoopClockTests();
    const auto timeOk = runTimeWindowResolverTests();
    const auto bandOk = runBandAnalyzerTests();
    const auto channelOk = runChannelViewsTests();
    const auto parametersOk = runParametersTests();
    const auto themeEngineOk = runThemeEngineTests();

    if (ringOk && clockOk && timeOk && bandOk && channelOk && parametersOk && themeEngineOk)
    {
        std::cout << "All tests passed." << std::endl;
        return EXIT_SUCCESS;
    }

    std::cerr << "At least one test suite failed." << std::endl;
    return EXIT_FAILURE;
}
