#include <boost/filesystem.hpp>
#include <include.h>

#include "core/SimulationBuilderXml.h"
#include "core/SimulationReader.h"
#include "statistics/TransmissionSummaryStats.h"
#include "util/Utility.h"

// unreachable code in main()'s top-level for-loop for some reason
#if defined(_MSC_VER) && _MSC_VER >= 1800
#pragma warning ( disable : 4702 )
#endif

namespace {

struct ArgumentToken
{
	enum class ArgumentTokenType
	{
		unknown,
		option,
		cepacDirectory,
		inputDirectory
	};

	ArgumentTokenType type;
	std::string value;
};

std::vector<ArgumentToken> ParseArgumentTokens(int argc, char *argv[])
{
	std::vector<ArgumentToken> tokens;

	for(int i = 1; i < argc; i++)
	{
		std::string tokenString = argv[i];

		if(tokenString.length() > 0)
		{
			ArgumentToken token;
			token.value = tokenString;
			if (token.value == "--cepac")
			{
				token.type = ArgumentToken::ArgumentTokenType::cepacDirectory;
				token.value = argv[i+1];
				i++;
			}
			else if(token.value.length() > 2 && token.value.substr(0, 1) == "-")
			{
				token.type = ArgumentToken::ArgumentTokenType::option;
			}
			else
			{
				token.type = ArgumentToken::ArgumentTokenType::inputDirectory;
			}

			tokens.push_back(token);
		}
	}

	return tokens;
};

void PrintUsage()
{
	std::cout << "usage: transm [--version] [--help] [--cepac <dir>] <input_directory>" << std::endl;
}

void PrintVersion()
{
	std::cout << "transm version " << Version::ToString(Utility::MODEL_VERSION) << std::endl;
}

void PrintBadOption(const std::string &option)
{
	std::cout << "Unknown option: " << option << std::endl;
	PrintUsage();
}

void Simulate(const std::string &filename, SummaryStats &cepac_summary, TransmissionSummaryStats &transmission_summary)
{
	//Changing back to the input directory because over the course of Sim->run, the directory gets changed to results
	CepacUtil::changeDirectoryToInputs();
	std::cout << "Running File: " << filename << std::endl;

	SimulationBuilderXml builder;
	auto name = boost::filesystem::path(filename).stem().string();
	SimulationReader reader(builder);
	reader.ConstructSimulation(filename);
	auto &simulation = builder.GetResult();

	auto message_callback = [](const std::string &s) { std::cout << s; };
	auto outputs = simulation.Run(message_callback);

	cepac_summary.addRunStats(&simulation.GetCEPACRunStats());
	transmission_summary.addPopulationStatistics(simulation.GetPopulationStatistics(), simulation.GetEventParams());
}

int RunSimulation(const std::string &directory = "", const std::string &cepacDirectory = "")
{
	auto workingDirectory = boost::filesystem::current_path();

	CepacUtil::inputsDirectory = cepacDirectory;
	CepacUtil::changeDirectoryToInputs();
	//Call this so that relative directories can be used as input (i.e. "../")
	CepacUtil::useCurrentDirectoryForInputs();

	Utility::findInputFiles(directory, workingDirectory.string());
	auto &input_files = Utility::transmFilesToRun;

	CepacUtil::createResultsDirectory();

	SummaryStats cepacSummaryStats("cepacPopstats.out");
	TransmissionSummaryStats transSummaryStats("summaryStats.out");

	std::for_each(input_files.begin(), input_files.end(), [&](const std::string &s)
	{
		Simulate(s, cepacSummaryStats, transSummaryStats);
	});

	//Finalize CEPAC summary stats and print the popstats file
	cepacSummaryStats.finalizeStats();

	cepacSummaryStats.writeSummariesFile();
	transSummaryStats.writeSummariesFile();

	return 0;
}

} // namespace

int main(int argc, char *argv[])
{
	std::vector<std::string> simulations;
	std::string cepacDirectory;
	auto tokens = ParseArgumentTokens(argc, argv);
	for(const auto &token : tokens)
	{
		if(token.type == ArgumentToken::ArgumentTokenType::option)
		{
			if(token.value == "--version")
			{
				PrintVersion();
				return 0;
			}
			else if(token.value == "--help")
			{
				PrintUsage();
				return 0;
			}
		}
		else if(token.type == ArgumentToken::ArgumentTokenType::cepacDirectory)
		{
			cepacDirectory = token.value;
		}
		else if(token.type == ArgumentToken::ArgumentTokenType::inputDirectory)
		{
			simulations.push_back(token.value);
		}
		else
		{
			PrintBadOption(token.value);
			return 1;
		}
	}

	for (auto simulation : simulations)
	{
		if (cepacDirectory.empty())
			cepacDirectory=simulation;
		RunSimulation(simulation, cepacDirectory);
	}

	return 0;
}
