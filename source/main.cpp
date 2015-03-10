#include <functional>
#include <unordered_set>
#include <tclap/CmdLine.h>

#include "core/simulation.h"
#include "core/simulationbuilderxml.h"
#include "core/simulationreader.h"
#include "statistics/transmissionsummarystats.h"
#include "utility/utility.h"
#include "utility/filesystem.h"

namespace {

/// <summary>
/// Search batch_directory and return list of XML and JSON files.
/// </summary>
std::vector<transm::path> find_input_files(const transm::path &batch_directory)
{
    static const std::unordered_set<std::string> known_extensions = {".xml", ".json"};
    auto is_not_known_extension = [](const transm::path &p) -> bool
    {
        return known_extensions.find(p.extension().string()) == known_extensions.end();
    };
    std::vector<transm::path> input_files;

    // return all files in the directory with known extensions
    if(transm::filesystem::exists(batch_directory) 
        && transm::filesystem::is_directory(batch_directory))
    {
        auto all_files = transm::filesystem::listdir(batch_directory);
        auto new_end = std::remove_if(all_files.begin(), all_files.end(), is_not_known_extension);
        input_files = std::vector<transm::path>(all_files.begin(), new_end);
    }
    // a specific file was given, return it as a singular element in a list
    else if (transm::filesystem::exists(batch_directory)
        && transm::filesystem::is_regular_file(batch_directory)
        && !is_not_known_extension(batch_directory)) //double negative!
    {
        input_files.push_back(batch_directory);
    }
    else
    {
        auto message = std::string("not a directory ") + batch_directory.string();
        throw std::runtime_error(message);
    }

    return input_files;
}

/// <sumary>
/// Find all XML and JSON files in batch_directory. Load parameters from each
/// file and run the model using those parameters.
/// </summary>
int run_simulation(const transm::path &batch_directory)
{
    auto workingDirectory = transm::filesystem::current_path();

    CepacUtil::inputsDirectory = batch_directory.string();
    CepacUtil::changeDirectoryToInputs();
    //Call this so that relative directories can be used as input (i.e. "../")
    CepacUtil::useCurrentDirectoryForInputs();
    CepacUtil::createResultsDirectory();

    SummaryStats cepac_summary("cepacPopstats.out");
    TransmissionSummaryStats transmission_summary("summaryStats.out");

    auto batch_name = batch_directory.stem().string();
    auto input_files = find_input_files(batch_directory);

    for(auto input_file : input_files)
    {
        //Changing back to the input directory because over the course of Sim->run, the directory gets changed to results
        CepacUtil::changeDirectoryToInputs();
        std::cout << "Running File: " << input_file.stem().string() << std::endl;

        SimulationBuilderXml sim_builder;
        SimulationReader sim_reader(sim_builder);
        sim_reader.ConstructSimulation(input_file.string());

        Simulation &simulation = sim_builder.GetResult();

        auto outputs = simulation.Run([](const std::string &s) { std::cout << s; });

        cepac_summary.addRunStats(&simulation.GetCEPACRunStats());
        transmission_summary.addPopulationStatistics(simulation.GetPopulationStatistics(), simulation.GetEventParams());
    }

    cepac_summary.finalizeStats();
    cepac_summary.writeSummariesFile();
    transmission_summary.writeSummariesFile();

    return 0;
}

/// <sumary>
/// Print to standard output how this model can be used.
/// </summary>
void print_usage(const std::string &executable)
{
    std::cout << "usage: " << executable;
    std::cout << " [--version] [--help] input [input...]" << std::endl;
    std::cout << std::endl;
    std::cout << "input should be a directory containing one or more JSON " << std::endl;
    std::cout << "   files or a specific JSON file to be simulated. In the case of a " << std::endl;
    std::cout << "   directory, input files will be simulated sequentially in an arbitrary" << std::endl;
    std::cout << "   order. If multiple inputs are specified, they will be simulated in " << std::endl;
    std::cout << "   the order they are given." << std::endl;
}

/// <sumary>
/// Print to standard output a description of this model's version.
/// </summary>
void print_version(const std::string &executable)
{
    auto version_string = Version::ToString(Utility::get_model_version());
    std::cout << executable << " version " << version_string << std::endl;
}

} // namespace <unnamed>

/// <summary>
/// Process provided arguments and execute the simulation as specified by those arguments.
/// </summary>
int main(int argc, char *argv[])
{
    auto executable = transm::path(argv[0]).filename().string();

    try
    {
        std::string program_description = "CEPAC Dynamic Model is an "
            "individual-based simulation of HIV transmission. For more info, "
            "see the User Guide or \"Development, Calibration and Performance "
            "of an HIV Transmission Model Incorporating Natural History and "
            "Behavioral Patterns: Application in South Africa\", PLOSone, 2014";
        auto version_string = Version::ToString(Utility::get_model_version());
        TCLAP::CmdLine cmd(program_description, ' ', version_string, false);
        cmd.setExceptionHandling(false);

        std::string help_description = "help";
        TCLAP::SwitchArg help_switch("h", "help", help_description, false);
        cmd.add(help_switch);

        std::string version_description = "version";
        TCLAP::SwitchArg version_switch("v", "version", version_description, false);
        cmd.add(version_switch);

        auto input_files_description = "One or more JSON files or directories "
            "containing JSON files that will be simulated in the given order.";
        TCLAP::UnlabeledMultiArg<std::string> input_files_arg("input",
            input_files_description, true, "something", false, nullptr);
        cmd.add(input_files_arg);

        cmd.parse(argc, argv);

        if(help_switch.getValue())
        {
            print_usage(executable);
            return 0;
        }

        if(version_switch.getValue())
        {
            print_version(executable);
            return 0;
        }

        for(auto batch : input_files_arg.getValue())
        {
			auto result = run_simulation(batch);
			// added because sometimes we don't see all output if buffered
			std::cout.flush();

			// stop if any batch fails
            if(result != 0)
            {
                return 1;
            }
        }

        return 0;
    }
    catch(TCLAP::ArgException &/*e*/)
    {
        print_usage(executable);
        return 1;
    }
}
