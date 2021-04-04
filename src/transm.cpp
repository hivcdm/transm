#include <fstream>
#include <functional>
#include <tclap/CmdLine.h>

#include "transm.hpp"
/**
 * Process provided arguments and execute the simulation as specified by those arguments.
 */
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
	for(const auto& batch : batch_files_arg.getValue())
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
