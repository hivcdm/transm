#include <fstream>
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
std::vector<transm::path> find_input_files(const transm::path &batch_path)
{
    static const std::unordered_set<std::string> known_extensions = {".xml", ".json"};
    auto is_not_known_extension = [](const transm::path &p) -> bool
    {
        return known_extensions.find(p.extension().string()) == known_extensions.end();
    };

    std::vector<transm::path> input_files;
    if (transm::filesystem::is_regular_file(batch_path) &&
	!is_not_known_extension(batch_path)) {
	input_files.push_back(batch_path);
    } else if(transm::filesystem::is_directory(batch_path)) {
        auto all_files = transm::filesystem::listdir(batch_path);
        auto new_end = std::remove_if(all_files.begin(), all_files.end(),
				      is_not_known_extension);
        input_files = std::vector<transm::path>(all_files.begin(), new_end);
    }

    return input_files;
}

/// <summary>
/// Find all XML files in batch_directory and CEPAC .in files in cepac_directory
/// (Might be the same directory). Load parameters from each XML file and run
/// the model using those parameters.
/// </summary>
int run_simulation(const transm::path &batch_path, const transm::path &cepac_directory)
{
    int result;

    transm::path batch_directory;
    if (transm::filesystem::is_regular_file(batch_path)) {
	batch_directory = batch_path.parent_path();
    } else {
	batch_directory = batch_path;
    }
    transm::Utility::setInputsDirectory(batch_directory.string());
    transm::Utility::changeDirectoryToInputs();

    transm::path results("results");
    transm::path results_directory(batch_directory.append(results));
    result = transm::Utility::createResultsDirectory(results_directory.string());
    if (result != 0) {
	cout << "Failed to create the results directory: " +
	    results_directory.string() << std::endl;
	return 1;
    }

    /* Set the Cepac input directory */
    CepacUtil::inputsDirectory = cepac_directory.string();

    SummaryStats cepac_summary("cepacPopstats.out");
    transm::TransmissionSummaryStats transmission_summary("summaryStats.out");

    auto batch_name = batch_directory.stem().string();
    transm::BatchStatus status(batch_name);

    auto input_files = find_input_files(batch_path);
    status.initialize(input_files);

    if (input_files.empty()) {
	cout << "No input files found in batch" << batch_path.string() << std::endl;
	return 1;
    }

    for(auto input_file : input_files)
    {
        //Changing back to the input directory because over the course of Sim->run,
	//the directory gets changed to results
	    transm::Utility::changeDirectoryToInputs();

        std::cout << "Running File: " << input_file.stem().string() << std::endl;

        transm::SimulationParametersXml parameters(input_file);

        transm::Simulation simulation(status);
        //XXX: we shouldn't have to do this
        parameters.SetRandomNumberGenerator(simulation.GetEventParams().randomNums);
        simulation.Initialize(parameters);

	if (simulation.GetEventParams().calibrationInputs.useCalibration) {
	    std::cout << std::endl;
	    std::cout << "*****Warning: Calibration Enabled******" << std::endl;
	    std::cout << "This will make things run slowly" << std::endl;
	    std::cout << "If this is not expected hit Ctrl+c to abort" << std::endl;
	    std::cout << std::endl;
	    sleep(4);
	}

	auto outputs = simulation.Run();

        cepac_summary.addRunStats(&simulation.GetCEPACRunStats());
        transmission_summary.addPopulationStatistics(simulation.GetPopulationStatistics(), simulation.GetEventParams());
    }

    cepac_summary.finalizeStats();
    cepac_summary.writeSummariesFile();
    transmission_summary.writeSummariesFile();

    return 0;
}

/// <summary>
/// Print to standard output how this model can be used.
/// </summary>
void print_usage(const std::string &executable)
{
    std::cout << "usage: " << executable;
    std::cout << " [--version] [--help] --cepac [directory] [input...]" << std::endl;
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

/// <summary>
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

        auto cepac_dir_description = "Directory containing CEPAC .in files";
        TCLAP::ValueArg<std::string> cepac_dir_arg("d", "cepac",
	    cepac_dir_description, false, "", "directory", nullptr);

	auto batch_files_description = "One or more XML files or a directory"
	    "containing XML files.";
	TCLAP::UnlabeledMultiArg<std::string> batch_files_arg("batch",
	    batch_files_description, true, "XML file, files or directory", false, nullptr);

	/* cmd.xorAdd cmdline arguments that are required to make them mutually exclusive */
	std::vector<TCLAP::Arg *> xor_list;
	xor_list.push_back(&help_switch);
        xor_list.push_back(&version_switch);
	xor_list.push_back(&batch_files_arg);
        cmd.xorAdd(xor_list);

	/* cmd.add additional args */
	cmd.add(cepac_dir_arg);

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

	bool use_cmdline_cepac_directory = false;
	transm::path cepac_directory;
	std::string cepac_dir_str = cepac_dir_arg.getValue();
	if (!cepac_dir_str.empty()) {
	    /* Use the absolute path to the cepac directory */
	    cepac_directory = transm::filesystem::real_path(cepac_dir_str);
	    if (!transm::filesystem::exists(cepac_directory)) {
		/* bail if the directory is bogus */
		std::cout << "Check the path to the cepac directory: " <<
		    cepac_dir_str.c_str() << std::endl;
		return 1;
	    }
	    use_cmdline_cepac_directory = true;
	}

	transm::path batch_path;
	for(auto batch : batch_files_arg.getValue())
        {
            /* Check that the file or directory exists using the absolute path */
	    batch_path = transm::filesystem::real_path(batch);
	    if (!transm::filesystem::exists(batch_path)) {
                /* bail if the directory is bogus */
		std::cout << "Check the path to the batch directory: " <<
		    batch.c_str() << std::endl;
		return 1;
	    }

	    if (!use_cmdline_cepac_directory) {
		/* Set the cepac_directory to the batch dir */
		if (transm::filesystem::is_regular_file(batch_path))
		    cepac_directory = batch_path.parent_path();
		else
		    cepac_directory = batch_path;
	    }

	    auto result = run_simulation(batch_path, cepac_directory);
	    // added because sometimes we don't see all output if buffered
	    std::cout.flush();

	    if (result != 0)
		return result;
	}

        return 0;
    }
    catch(TCLAP::ArgException &e)
    {
        print_usage(executable);
        return 1;
    }
}
