#include <unordered_set>
#include <tclap/CmdLine.h>

#include "core/BatchStatus.hpp"
#include "core/Simulation.hpp"
#include "core/SimulationBuilderXml.hpp"
#include "core/SimulationReader.hpp"
#include "statistics/TransmissionSummaryStats.hpp"
#include "utility/Utility.hpp"
#include "utility/filesystem.hpp"

namespace {

std::vector<std::string> find_input_files(const path &batch_directory)
{
    static const std::unordered_set<std::string> known_extensions = {".xml", ".json"};
    std::vector<std::string> input_files;

    if(filesystem::exists(batch_directory) && filesystem::is_directory(batch_directory))
    {
        auto all_files = filesystem::listdir(batch_directory);
        auto new_end = std::remove_if(all_files.begin(), all_files.end(), [](const path &p)
        {
            return known_extensions.find(p.extension().string()) == known_extensions.end();
        });
        std::transform(all_files.begin(), new_end,
            std::back_inserter(input_files), [](const path &p)
        { return p.stem().string(); });
    }
    else
    {
        throw std::runtime_error("not a directory");
    }

    return input_files;
}

int run_simulation(const std::string &batch_name, std::function<void(const std::string &)> message_callback)
{
    auto workingDirectory = filesystem::current_path();

    auto batches_directory = Utility::get_batches_directory();
    auto batch_directory = batches_directory / batch_name;

    CepacUtil::inputsDirectory = batch_directory.string();
    CepacUtil::changeDirectoryToInputs();
    //Call this so that relative directories can be used as input (i.e. "../")
    CepacUtil::useCurrentDirectoryForInputs();

    CepacUtil::createResultsDirectory();

    SummaryStats cepac_summary("cepacPopstats.out");
    TransmissionSummaryStats transmission_summary("summaryStats.out");

    BatchStatus status(batch_name);

    auto task_names = find_input_files(batch_directory);
    status.initialize(task_names);

    for(auto task_name : task_names)
    {
        /*
        SimStatus current_task_status(task_name, status);
        Simulation s(current_task_status, message_callback);

        auto task_filename = batches_directory / batch_name / path(task_name);
        s.load_inputs(task_filename.string());

        s.run();
        */

        //Changing back to the input directory because over the course of Sim->run, the directory gets changed to results
        CepacUtil::changeDirectoryToInputs();
        std::cout << "Running File: " << task_name << std::endl;

        SimulationBuilderXml builder(status);
        SimulationReader reader(builder);

        auto task_filename = batches_directory / batch_name / path(task_name + ".xml");
        reader.ConstructSimulation(task_filename.string());

        auto &simulation = builder.GetResult();

        auto message_callback = [](const std::string &s) { std::cout << s; };
        auto outputs = simulation.Run(message_callback);

        cepac_summary.addRunStats(&simulation.GetCEPACRunStats());
        transmission_summary.addPopulationStatistics(simulation.GetPopulationStatistics(), simulation.GetEventParams());
    }

    cepac_summary.finalizeStats();
    cepac_summary.writeSummariesFile();
    transmission_summary.writeSummariesFile();

    return 0;
}

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

void print_version(const std::string &executable)
{
    std::cout << executable << " version " << Version::to_string(Utility::get_model_version()) << std::endl;
}

} // namespace

int main(int argc, char *argv[])
{
    auto executable = path(argv[0]).filename().string();
    auto message_callback = [](const std::string &message) { std::cout << message << std::endl; };

    try
    {
        std::string program_description = "CEPAC Dynamic Model is an "
            "individual-based simulation of HIV transmission. For more info, "
            "see the User Guide or \"Development, Calibration and Performance "
            "of an HIV Transmission Model Incorporating Natural History and "
            "Behavioral Patterns: Application in South Africa\", PLOSone, 2014";
        auto version_string = Version::to_string(Utility::get_model_version());
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
            if(run_simulation(batch, message_callback) != 0)
            {
                std::cout.flush();
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
