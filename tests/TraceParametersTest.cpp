#include <gtest/gtest.h>
#include <cstdio>
#include <fstream>
#include <string>

#include "parameters/simulationparameters.hpp"

namespace transm {

namespace {

/* A scenario containing only the <traceFiles> block the tracing parameters read. */
std::string Scenario(const std::string &focus_element)
{
    return "<simulation version=\"4.5\">\n"
           "  <traceFiles>\n"
           "    <singlePerson enabled=\"true\" tossIfCalibFail=\"false\">\n"
           "      <extension>SinglePerson.xls</extension>\n"
           "      <numberToTracePerAgeRange>15</numberToTracePerAgeRange>\n"
           "      <numberNewbornsToTrace>500</numberNewbornsToTrace>\n"
           "      <monthTraceNewborns>600</monthTraceNewborns>\n"
           "      <tracePrevalentCases>1</tracePrevalentCases>\n"
           + focus_element +
           "    </singlePerson>\n"
           "    <lifeExpectancy enabled=\"false\" tossIfCalibFail=\"true\">\n"
           "      <extension>LE.xls</extension>\n"
           "      <medianConfidenceInterval>0.95</medianConfidenceInterval>\n"
           "    </lifeExpectancy>\n"
           "  </traceFiles>\n"
           "</simulation>\n";
}

SimulationParameters::TracingParameters Parse(const std::string &xml)
{
    const std::string filename = std::string(std::tmpnam(nullptr)) + ".xml";
    std::ofstream(filename) << xml;
    SimulationParametersXml parameters(path(filename), path("."));
    auto tracing = parameters.GetTracingParameters();
    std::remove(filename.c_str());
    return tracing;
}

} // namespace

TEST(TraceParametersTest, FocusTraceCountDefaultsToZeroWhenAbsent)
{
    auto tracing = Parse(Scenario(""));
    EXPECT_EQ(0, tracing.num_focus_to_trace);
    EXPECT_EQ(500, tracing.num_newborns_to_trace);
    EXPECT_TRUE(tracing.trace_prevalent_cases);
}

TEST(TraceParametersTest, FocusTraceCountIsRead)
{
    auto tracing = Parse(Scenario("      <numberFocusToTrace>100</numberFocusToTrace>\n"));
    EXPECT_EQ(100, tracing.num_focus_to_trace);
}

} // namespace transm
