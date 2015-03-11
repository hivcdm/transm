#include <functional>
#include <unordered_set>
#include <tclap/CmdLine.h>

#include "core/batchstatus.hpp"
#include "core/simulation.hpp"
#include "parameters/simulationparameters.hpp"
#include "statistics/transmissionsummarystats.hpp"
#include "utility/utility.hpp"
#include "utility/filesystem.hpp"

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
    transm::TransmissionSummaryStats transmission_summary("summaryStats.out");

    auto batch_name = batch_directory.stem().string();
    transm::BatchStatus status(batch_name);

    auto input_files = find_input_files(batch_directory);
    status.initialize(input_files);

    for(auto input_file : input_files)
    {
        //Changing back to the input directory because over the course of Sim->run, the directory gets changed to results
        CepacUtil::changeDirectoryToInputs();
        std::cout << "Running File: " << input_file.stem().string() << std::endl;

        transm::SimulationParametersXml parameters(input_file);

        transm::Simulation simulation(status);
        //XXX: we shouldn't have to do this
        parameters.SetRandomNumberGenerator(simulation.GetEventParams().randomNums);
        simulation.Initialize(parameters);

        auto outputs = simulation.Run();

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
}

void print_help(const std::string &executable)
{
    print_usage(executable);
    std::cout << std::endl;
    std::cout <<
"input should be a directory containing one or more parameter files (XML or     \n"
"   JSON) or a specific parameter file to be simulated. In the case of a        \n"
"   directory, input files will be simulated sequentially in an arbitrary order.\n"
"   If multiple inputs are specified, they will be simulated in the order they  \n"
"   are given." << std::endl;
}

/// <sumary>
/// Print to standard output a description of this model's version.
/// </summary>
void print_version()
{
    auto version_string = transm::Version::to_string(transm::Utility::get_model_version());
    std::cout << "transm version " << version_string << std::endl;
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
        auto version_string = transm::Version::to_string(transm::Utility::get_model_version());
        TCLAP::CmdLine cmd(program_description, ' ', version_string, false);
        cmd.setExceptionHandling(false);

        std::string help_description = "help";
        TCLAP::SwitchArg help_switch("h", "help", help_description, false);

        std::string version_description = "version";
        TCLAP::SwitchArg version_switch("v", "version", version_description, false);

        auto input_files_description = "One or more JSON files or directories "
            "containing JSON files that will be simulated in the given order.";
        TCLAP::UnlabeledMultiArg<std::string> input_files_arg("input",
            input_files_description, true, "something", false, nullptr);

	std::vector<TCLAP::Arg *> xor_list;
	xor_list.push_back(&help_switch);
        xor_list.push_back(&version_switch);
        xor_list.push_back(&input_files_arg);
        cmd.xorAdd(xor_list);

        cmd.parse(argc, argv);

        if(help_switch.getValue())
        {
            print_help(executable);
            return 0;
        }

        if(version_switch.getValue())
        {
            print_version();
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
    catch(TCLAP::ArgException &e)
    {
        print_usage(executable);
        return 1;
    }
}
