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

    auto data = Data(Iterator(in), Iterator<char>());
    in.close();

    return data;
}

std::string getDirectory(const std::string& path)
{
    auto pos = path.find_last_of("/\\");

    if (pos != std::string::npos)
        return path.substr(0, pos);

    return ".";
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

struct GenerateArgs
{
    std::string outputDir;
    std::string namespaceName;
    std::string category;
    std::vector<std::string> inputFiles;
};

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

std::string generateEntriesCpp(const GenerateArgs& args)
{
    auto out = std::ostringstream();

    out << "#include <ResEmbed/ResEmbed.h>\n\n";

    out << "extern \"C\"\n{\n";
    for (size_t i = 0; i < args.inputFiles.size(); ++i)
    {
        auto varPrefix = args.namespaceName + "_" + std::to_string(i);
        out << "extern const unsigned char " << varPrefix << "_data[];\n";
        out << "extern const unsigned long " << varPrefix << "_size;\n";
    }
    out << "}\n";

    out << "\nnamespace " << args.namespaceName << "\n";
    out << "{\n";
    out << "const ResEmbed::Entries& getResourceEntries()\n";
    out << "{\n";
    out << "    static const ResEmbed::Entries entries = {\n";

    for (size_t i = 0; i < args.inputFiles.size(); ++i)
    {
        auto varPrefix = args.namespaceName + "_" + std::to_string(i);
        auto resourceName = getFilename(args.inputFiles[i]);
        out << "        {" << varPrefix << "_data, " << varPrefix << "_size, \""
            << resourceName << "\", \""
            << args.category << "\"}";

        if (i + 1 < args.inputFiles.size())
            out << ",";

        out << "\n";
    }

    out << "    };\n\n";
    out << "    return entries;\n";
    out << "}\n";
    out << "}\n";

    return out.str();
}

std::string generateInitHeader(const GenerateArgs& args)
{
    auto out = std::ostringstream();

    out << "#pragma once\n\n";
    out << "#include <ResEmbed/ResEmbed.h>\n\n";
    out << "namespace " << args.namespaceName << "\n";
    out << "{\n";
    out << "const ResEmbed::Entries& getResourceEntries();\n";
    out << "static const ResEmbed::Initializer resourceInitializer "
        << "{getResourceEntries()};\n";
    out << "}\n";

    return out.str();
}

GenerateArgs getGenerateArgs(int argc, char* argv[])
{
    if (argc < 4)
    {
        throw std::runtime_error(
            "Usage: ResourceGenerator generate <output_dir> <namespace> "
            "<category> file1 [file2 ...]");
    }

    auto args = GenerateArgs();

    args.outputDir = argv[0];
    args.namespaceName = argv[1];
    args.category = argv[2];

    for (int i = 3; i < argc; ++i)
        args.inputFiles.emplace_back(argv[i]);

    if (args.inputFiles.empty())
        throw std::runtime_error("Error: no input files specified");

    return args;
}

void runGenerate(const GenerateArgs& args)
{
    for (size_t i = 0; i < args.inputFiles.size(); ++i)
    {
        auto varPrefix = args.namespaceName + "_" + std::to_string(i);
        auto outputPath = args.outputDir + "/BinaryResource"
            + std::to_string(i) + ".c";
        auto content = generateDataFile(args.inputFiles[i], varPrefix);
        writeFileIfChanged(outputPath, content);
    }

    auto headerContent = generateInitHeader(args);
    writeFileIfChanged(
        args.outputDir + "/" + args.namespaceName + ".h", headerContent);

    auto cppContent = generateEntriesCpp(args);
    writeFileIfChanged(
        args.outputDir + "/" + args.namespaceName + ".cpp", cppContent);
}

std::string parseCommand(int argc, char* argv[])
{
    if (argc < 2)
    {
        throw std::runtime_error(
            "Usage: ResourceGenerator generate ...");
    }

    return {argv[1]};
}

void run(int argc, char* argv[])
{
    auto command = parseCommand(argc, argv);

    if (command == "generate")
    {
        auto args = getGenerateArgs(argc - 2, argv + 2);
        runGenerate(args);
    }
    else
    {
        throw std::runtime_error(
            "Unknown command: " + command +
            "\nUsage: ResourceGenerator generate ...");
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
