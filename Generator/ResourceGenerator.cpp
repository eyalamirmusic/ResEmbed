#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <string>

using Data = std::vector<unsigned char>;

template <typename T>
using Iterator = std::istreambuf_iterator<T>;

Data readDataFrom(const std::string& path)
{
    auto in = std::ifstream(path, std::ios::binary);

    if (!in)
        throw std::runtime_error("Error: cannot open input file: " + path);

    auto data = Data(Iterator<char>(in), Iterator<char>());
    in.close();

    return data;
}

std::string getFilename(const std::string& path)
{
    auto pos = path.find_last_of("/\\");

    if (pos != std::string::npos)
        return path.substr(pos + 1);

    return path;
}

bool writeFileIfChanged(const std::string& path, const std::string& content)
{
    auto in = std::ifstream(path, std::ios::binary);

    if (in)
    {
        auto existing = std::string(
            std::istreambuf_iterator<char>(in),
            std::istreambuf_iterator<char>());
        in.close();

        if (existing == content)
            return false;
    }

    auto out = std::ofstream(path, std::ios::binary);

    if (!out)
        throw std::runtime_error("Error: cannot open output file: " + path);

    out << content;

    if (!out)
        throw std::runtime_error("Error: failed to write output file: " + path);

    return true;
}

std::string generateDataFile(const std::string& input,
                             const std::string& varPrefix)
{
    auto data = readDataFrom(input);
    auto out = std::ostringstream();

    out << "const unsigned char " << varPrefix << "_data[] = {\n";

    for (size_t i = 0; i < data.size(); ++i)
    {
        if (i % 16 == 0)
            out << "    ";

        out << static_cast<unsigned int>(data[i]);

        if (i + 1 < data.size())
            out << ",";

        if (i % 16 == 15 || i + 1 == data.size())
            out << "\n";
        else
            out << " ";
    }

    out << "};\n\n";
    out << "const unsigned long " << varPrefix
        << "_size = sizeof(" << varPrefix << "_data);\n";

    return out.str();
}

std::string generateEntriesCpp(const std::string& namespaceName,
                               const std::string& category,
                               const std::vector<std::string>& inputFiles)
{
    auto out = std::ostringstream();

    out << "#include <ResEmbed/Entries.h>\n\n";

    out << "extern \"C\"\n{\n";
    for (size_t i = 0; i < inputFiles.size(); ++i)
    {
        auto varPrefix = namespaceName + "_" + std::to_string(i);
        out << "extern const unsigned char " << varPrefix << "_data[];\n";
        out << "extern const unsigned long " << varPrefix << "_size;\n";
    }
    out << "}\n";

    out << "\nnamespace " << namespaceName << "\n";
    out << "{\n";
    out << "const ResEmbed::Entries& getResourceEntries()\n";
    out << "{\n";
    out << "    static const ResEmbed::Entries entries = {\n";

    for (size_t i = 0; i < inputFiles.size(); ++i)
    {
        auto varPrefix = namespaceName + "_" + std::to_string(i);
        auto resourceName = getFilename(inputFiles[i]);
        out << "        {" << varPrefix << "_data, " << varPrefix << "_size, \""
            << resourceName << "\", \""
            << category << "\"}";

        if (i + 1 < inputFiles.size())
            out << ",";

        out << "\n";
    }

    out << "    };\n\n";
    out << "    return entries;\n";
    out << "}\n";
    out << "}\n";

    return out.str();
}

std::string generateInitHeader(const std::string& namespaceName)
{
    auto out = std::ostringstream();

    out << "#pragma once\n\n";
    out << "#include <ResEmbed/ResEmbed.h>\n";
    out << "#include <ResEmbed/Entries.h>\n\n";
    out << "namespace " << namespaceName << "\n";
    out << "{\n";
    out << "const ResEmbed::Entries& getResourceEntries();\n";
    out << "static const ResEmbed::Initializer resourceInitializer "
        << "{getResourceEntries()};\n";
    out << "}\n";

    return out.str();
}

struct ConfigFile
{
    std::string outputDir;
    std::string namespaceName;
    std::string category;
    std::vector<std::string> inputFiles;
};

ConfigFile readConfigFile(const std::string& path)
{
    auto in = std::ifstream(path);

    if (!in)
        throw std::runtime_error("Error: cannot open config file: " + path);

    auto config = ConfigFile();
    auto line = std::string();

    if (!std::getline(in, config.outputDir) || config.outputDir.empty())
        throw std::runtime_error("Error: config file missing output directory");

    if (!std::getline(in, config.namespaceName) || config.namespaceName.empty())
        throw std::runtime_error("Error: config file missing namespace");

    if (!std::getline(in, config.category) || config.category.empty())
        throw std::runtime_error("Error: config file missing category");

    while (std::getline(in, line))
    {
        if (!line.empty())
            config.inputFiles.push_back(line);
    }

    if (config.inputFiles.empty())
        throw std::runtime_error("Error: config file contains no input files");

    return config;
}

void runGenerateData(const std::string& outputPath,
                     const std::string& varPrefix,
                     const std::string& inputFile)
{
    auto content = generateDataFile(inputFile, varPrefix);
    writeFileIfChanged(outputPath, content);
}

void runGenerateRegistry(const std::string& configPath)
{
    auto config = readConfigFile(configPath);

    writeFileIfChanged(
        config.outputDir + "/" + config.namespaceName + ".h",
        generateInitHeader(config.namespaceName));

    writeFileIfChanged(
        config.outputDir + "/" + config.namespaceName + ".cpp",
        generateEntriesCpp(config.namespaceName, config.category,
                           config.inputFiles));
}

std::string parseCommand(int argc, char* argv[])
{
    if (argc < 2)
    {
        throw std::runtime_error(
            "Usage: ResourceGenerator <command> ...\n"
            "Commands:\n"
            "  generate-data <output.c> <var_prefix> <input_file>\n"
            "  generate-registry <config_file>");
    }

    return {argv[1]};
}

void run(int argc, char* argv[])
{
    auto command = parseCommand(argc, argv);

    if (command == "generate-data")
    {
        if (argc != 5)
        {
            throw std::runtime_error(
                "Usage: ResourceGenerator generate-data "
                "<output.c> <var_prefix> <input_file>");
        }

        runGenerateData(argv[2], argv[3], argv[4]);
    }
    else if (command == "generate-registry")
    {
        if (argc != 3)
        {
            throw std::runtime_error(
                "Usage: ResourceGenerator generate-registry <config_file>");
        }

        runGenerateRegistry(argv[2]);
    }
    else
    {
        throw std::runtime_error("Unknown command: " + command);
    }
}

int main(int argc, char* argv[])
{
    try
    {
        run(argc, argv);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
