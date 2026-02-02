#ifndef TRANSM_HPP
#define TRANSM_HPP

#include "core/batchstatus.hpp"
#include "core/simulation.hpp"
#include "parameters/simulationparameters.hpp"
#include "statistics/transmissionsummarystats.hpp"
#include "utility/utility.hpp"
#include "utility/filesystem.hpp"


//#include <pthread.h>

using namespace transm;

/**
 * Search batch_directory and return list of XML and JSON files.
 **/
std::vector<path> find_input_files(const path &batch_path) {
    static const std::unordered_set<std::string> known_extensions = {".xml", ".json"};
    auto is_not_known_extension = [](const path &p) -> bool {
        return known_extensions.find(p.extension().string()) == known_extensions.end();
    };

    std::vector<path> input_files;
    if (filesystem::is_regular_file(batch_path) && !is_not_known_extension(batch_path)) {
        input_files.push_back(batch_path);
    } else if (filesystem::is_directory(batch_path)) {
        auto all_files = filesystem::listdir(batch_path);
        auto new_end = std::remove_if(all_files.begin(), all_files.end(), is_not_known_extension);
        input_files = std::vector<path>(all_files.begin(), new_end);
    }

    return input_files;
}

/**
 * Find all XML files in batch_directory and CEPAC .in files in cepac_directory
 * using --cepac. Load parameters from each XML file and run
 * the model using those parameters.
 **/
int run_simulation(const path &batch_path, const path &cepac_directory, bool focus_enabled = false) {
    path batch_directory;
    if (filesystem::is_regular_file(batch_path)) {
        batch_directory = batch_path.parent_path();
    } else {
        batch_directory = batch_path;
    }

    Utility::setInputsDirectory(batch_directory.string());
    Utility::changeDirectoryToInputs();

    /* Set the cepac input directory */
    CepacUtil::inputsDirectory = cepac_directory.string();

    SummaryStats cepac_summary("cepacPopstats.out");
    TransmissionSummaryStats transmission_summary("summaryStats.out");

    auto batch_name = batch_directory.stem().string();
    BatchStatus status(batch_name);

    auto input_files = find_input_files(batch_path);
    status.initialize(input_files);

    if (input_files.empty()) {
        cout << "No input files found in batch" << batch_path.string() << std::endl;
        return 1;
    }

    for (const auto &input_file : input_files) {

        // Changing back to the input directory because over the course of Sim->run,
        // the directory gets changed to results
        Utility::changeDirectoryToInputs();

        /* Retrieve a copy of the original path to batch directory */
        path copy_batch_dir = batch_directory;

        /* Drooping the extension from filename */
        size_t lastIndex = input_file.filename().string().find_last_of('.');
        string rawFileName = input_file.filename().string().substr(0, lastIndex);

        /* Create the results_<INPUT_FILE_NAME> directory for results */
        std::string results_str = "results_" + rawFileName;
        path results_directory(copy_batch_dir.append(results_str));
        Utility::createResultsDirectory(results_directory.string());

        std::cout << "Running File: " << input_file.stem().string() << std::endl;

        /* Storing the parsed simulation parameters */
        // cepac_direcotory is also passed to the parser for accurate access to CEPAC files
        SimulationParametersXml parameters(input_file, cepac_directory);


        Simulation simulation(status);

        //XXX: we shouldn't have to do this
        parameters.SetRandomNumberGenerator(simulation.GetEventParams().randomNums);
        simulation.GetEventParams().focusEnabled = focus_enabled;
        simulation.Initialize(parameters);

        if (simulation.GetEventParams().calibrationInputs.useCalibration) {
            std::cout << std::endl;
            std::cout << "*****Warning: Calibration Enabled******" << std::endl;
            std::cout << "This will make things run slowly" << std::endl;
            std::cout << "If this is not expected hit Ctrl+c to abort" << std::endl;
            std::cout << std::endl;
            sleep(4);
        }

        // Start running models!
        auto outputs = simulation.Run();

        cepac_summary.addRunStats(&simulation.GetCEPACRunStats());
        transmission_summary.addPopulationStatistics(simulation.GetPopulationStatistics(), simulation.GetEventParams());
    }

    cepac_summary.finalizeStats();
    cepac_summary.writeSummariesFile();
    transmission_summary.writeSummariesFile();

    return 0;
}

/**
 * Print to standard output how this model can be used.
 **/
void print_usage(const std::string &executable) {
    std::cout << "usage: " << executable;
    std::cout << " [--version] [--help] --cepac [cepac_directory] [--focus on|off] [input(s)...]" << std::endl;
}

/**
 * Print a brief helping statement.
 **/
void print_help(const std::string &executable) {
    print_usage(executable);
    std::cout << std::endl;
    std::cout <<
    "   Input should be a directory containing one or more parameter files (XML or     \n"
    "   JSON) or a specific parameter file to be simulated. In the case of a        \n"
    "   directory, input files will be simulated sequentially in an arbitrary order using:\n\n"
    "   <TRANSM_BINARY> --cepac <PATH_TO_INFILES> ./\n\n"
    "   If multiple inputs are specified, they will be simulated in the order they  \n"
    "   are given.\n\n"
    "   Options:\n"
    "     --focus on|off    Enable or disable FOCUS module (default: off)\n"
    << std::endl;
}

/**
 * Print to standard output a description of this model's version.
 **/
void print_version() {
    auto version_string = Version::to_string(Utility::get_model_version());
    std::cout << "transm version " << version_string << std::endl;
}



#endif /* TRANSM_HPP */