#include "Simulation.h"
#include "cepac/include.h"
#include "statistics/TransmissionSummaryStats.h"
#include "util/Util.h"

struct ArgumentToken
{
	enum class ArgumentTokenType
	{
		unknown,
		option,
		inputDirectory
	};

	ArgumentTokenType type;
	std::string value;

	static ArgumentToken FromString(const std::string &tokenString)
	{
		ArgumentToken token;

		token.value = tokenString;

		if(token.value.length() > 2 && token.value.substr(0, 1) == "-")
		{
			token.type = ArgumentTokenType::option;
		}
		else
		{
			token.type = ArgumentTokenType::inputDirectory;
		}

		return token;
	}
};

std::vector<ArgumentToken> ParseArgumentTokens(int argc, char *argv[])
{
	std::vector<ArgumentToken> tokens;

	for(int i = 1; i < argc; i++)
	{
		std::string tokenString = argv[i];

		if(tokenString.length() > 0)
		{
			tokens.push_back(ArgumentToken::FromString(tokenString));
		}
	}

	return tokens;
};

void PrintUsage()
{
	std::cout << "usage: transm [--version] [--help] <input_directory>" << std::endl;
}

void PrintVersion()
{
	std::cout << "transm version " << Version::ToString(Util::MODEL_VERSION) << std::endl;
}

int RunSimulation(const std::string &directory = "")
{
	CepacUtil::inputsDirectory = directory;
	CepacUtil::changeDirectoryToInputs();
	//Call this so that relative directories can be used as input (i.e. "../")
	CepacUtil::useCurrentDirectoryForInputs();

	Util::findInputFiles(directory);
	CepacUtil::createResultsDirectory();
	SummaryStats cepacSummaryStats("cepacPopstats.out");
	TransmissionSummaryStats transSummaryStats("summaryStats.out");

	for(auto xml : Util::transmFilesToRun)
	{
		//Changing back to the input directory because over the course of Sim->run, the directory gets changed to results
		CepacUtil::changeDirectoryToInputs();
		std::cout << "Running File: " << xml << std::endl;
		//Console version will not use GraphViz and will use random seed by result
		Simulation s(xml);

		s.SetMessageCallback([](const std::string &s) { std::cout << s; });

		s.Initialize();
		while(s.Step()) {}

		cepacSummaryStats.addRunStats(s.GetCEPACRunStats());
		transSummaryStats.addPopStats(s.GetPopStats(), s.GetEventParams());
	}

	//Finalize CEPAC summary stats and print the popstats file
	cepacSummaryStats.finalizeStats();

	try
	{
		cepacSummaryStats.writeSummariesFile();
		transSummaryStats.writeSummariesFile();
	}
	catch(std::string errorString)
	{
		cout << errorString << "\n";
	}

	return 0;
}

void PrintBadOption(const std::string &option)
{
	std::cout << "Unknown option: " << option << std::endl;
	PrintUsage();
}

int main(int argc, char *argv[])
{
	auto tokens = ParseArgumentTokens(argc, argv);

	if(tokens.size() == 0)
	{
		PrintUsage();
	}

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
		else if(token.type == ArgumentToken::ArgumentTokenType::inputDirectory)
		{
			return RunSimulation(token.value);
		}

		PrintBadOption(token.value);
		return 1;
	}

	return RunSimulation();
}
