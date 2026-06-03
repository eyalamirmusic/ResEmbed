#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

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

// Key under which a file is registered in the runtime ResourceMap. When
// baseDir is empty, falls back to the historical basename-only behavior
// (collapses directory structure). When baseDir is set, returns the path
// relative to baseDir using forward slashes — so consumers can address
// nested files (e.g. "assets/index-abc.js") with stable, cross-platform
// keys that match URL paths.
std::string resourceKey(const std::string& path, const std::string& baseDir)
{
    if (baseDir.empty())
        return getFilename(path);

    auto relative = fs::relative(path, baseDir);
    return relative.generic_string();
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

// One bucket's .c file: the byte arrays for every resource round-robined
// into this bucket (global index i where i % bucketCount == bucket). The
// array names are keyed by the *global* index (namespace_<i>_data) so the
// registry's extern declarations line up regardless of how resources are
// distributed across buckets. With bucketCount == file count this collapses
// to one resource per file. An empty bucket still has to be a well-formed,
// non-empty translation unit, hence the placeholder typedef.
std::string generateBucketDataC(const std::string& namespaceName,
                                const std::vector<std::string>& inputFiles,
                                size_t bucket,
                                size_t bucketCount)
{
    auto out = std::ostringstream();
    auto wroteAny = false;

    for (size_t i = bucket; i < inputFiles.size(); i += bucketCount)
    {
        if (wroteAny)
            out << "\n";

        auto varPrefix = namespaceName + "_" + std::to_string(i);
        out << generateDataFile(inputFiles[i], varPrefix);
        wroteAny = true;
    }

    if (!wroteAny)
        out << "typedef int " << namespaceName << "_" << bucket
            << "_empty_tu;\n";

    return out.str();
}

std::string generateEntriesCpp(const std::string& namespaceName,
                               const std::string& category,
                               const std::string& baseDir,
                               const std::vector<std::string>& inputFiles)
{
    auto out = std::ostringstream();

    out << "#include \"" << namespaceName << ".h\"\n\n";

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
        auto resourceName = resourceKey(inputFiles[i], baseDir);
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
    out << "}\n";

    return out.str();
}

std::string generateRegisterCpp(const std::string& namespaceName)
{
    auto out = std::ostringstream();

    out << "#include \"" << namespaceName << ".h\"\n\n";
    out << "namespace\n";
    out << "{\n";
    out << "const ResEmbed::Initializer "
        << namespaceName << "_resourceInitializer "
        << "{" << namespaceName << "::getResourceEntries()};\n";
    out << "}\n";

    return out.str();
}

// Combined data file: every input's bytes inlined as anonymous-namespace
// arrays, followed by the Entries-returning function — a single TU for the
// whole res_embed_add call. Used as the fallback when the C-bucket layout
// (see --split-count) isn't available: namely C++-only consumers, where a
// single C++ TU is all we can compile.
std::string generateCombinedDataCpp(const std::string& namespaceName,
                                    const std::string& category,
                                    const std::string& baseDir,
                                    const std::vector<std::string>& inputFiles)
{
    auto out = std::ostringstream();

    out << "#include \"" << namespaceName << ".h\"\n\n";
    out << "namespace\n{\n";

    for (size_t i = 0; i < inputFiles.size(); ++i)
    {
        auto data = readDataFrom(inputFiles[i]);
        out << "const unsigned char data_" << i << "[] = {\n";

        for (size_t j = 0; j < data.size(); ++j)
        {
            if (j % 16 == 0)
                out << "    ";

            out << static_cast<unsigned int>(data[j]);

            if (j + 1 < data.size())
                out << ",";

            if (j % 16 == 15 || j + 1 == data.size())
                out << "\n";
            else
                out << " ";
        }

        out << "};\n\n";
    }

    out << "}\n\n";
    out << "namespace " << namespaceName << "\n{\n";
    out << "const ResEmbed::Entries& getResourceEntries()\n{\n";
    out << "    static const ResEmbed::Entries entries = {\n";

    for (size_t i = 0; i < inputFiles.size(); ++i)
    {
        auto resourceName = resourceKey(inputFiles[i], baseDir);
        out << "        {data_" << i << ", sizeof(data_" << i << "), \""
            << resourceName << "\", \"" << category << "\"}";

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

// Make-format depfile so Ninja re-runs `generate` when any embedded
// file's contents change without a full CMake reconfigure. The target
// in the depfile must match the OUTPUT path declared in the CMake
// custom_command (the data .cpp is the conventional choice).
std::string escapeDepfilePath(const std::string& path)
{
    auto out = std::string();
    out.reserve(path.size());

    for (auto c: path)
    {
        if (c == ' ' || c == '\\' || c == '#' || c == '$')
            out.push_back('\\');

        out.push_back(c);
    }

    return out;
}

std::string generateDepfile(const std::string& outputPath,
                            const std::vector<std::string>& inputFiles)
{
    auto out = std::ostringstream();
    out << escapeDepfilePath(outputPath) << ":";

    for (auto& input: inputFiles)
        out << " \\\n  " << escapeDepfilePath(input);

    out << "\n";
    return out.str();
}

struct ConfigFile
{
    std::string outputDir;
    std::string namespaceName;
    std::string category;
    std::string baseDir;
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

    // baseDir may legitimately be empty (opt-in feature: when empty,
    // resourceKey falls back to basename-only naming).
    if (!std::getline(in, config.baseDir))
        throw std::runtime_error("Error: config file missing base directory line");

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
                           config.baseDir, config.inputFiles));

    writeFileIfChanged(
        config.outputDir + "/" + config.namespaceName + "_Register.cpp",
        generateRegisterCpp(config.namespaceName));
}

struct GenerateArgs
{
    std::string scanDir;
    std::string manifestPath;
    std::string baseDir;
    std::string namespaceName;
    std::string category = "Resources";
    std::string outputCpp;
    std::string outputHeader;
    std::string outputRegister;
    std::string depfile;
    int splitCount = 0;
};

std::vector<std::string> readManifest(const std::string& path)
{
    auto in = std::ifstream(path);

    if (!in)
        throw std::runtime_error("Error: cannot open manifest: " + path);

    auto files = std::vector<std::string>();
    auto line = std::string();

    while (std::getline(in, line))
    {
        if (!line.empty())
            files.push_back(line);
    }

    return files;
}

std::vector<std::string> scanDirectory(const std::string& dir)
{
    auto files = std::vector<std::string>();

    if (!fs::exists(dir))
        return files;

    for (auto& entry: fs::recursive_directory_iterator(dir))
    {
        if (entry.is_regular_file())
            files.push_back(entry.path().generic_string());
    }

    return files;
}

GenerateArgs parseGenerateArgs(int argc, char* argv[])
{
    auto args = GenerateArgs();

    auto requireValue = [&](int& i, const char* flag) -> std::string
    {
        if (i + 1 >= argc)
            throw std::runtime_error(std::string("Missing value for ") + flag);

        return argv[++i];
    };

    for (auto i = 2; i < argc; ++i)
    {
        auto flag = std::string(argv[i]);

        if (flag == "--scan-dir")
            args.scanDir = requireValue(i, "--scan-dir");
        else if (flag == "--manifest")
            args.manifestPath = requireValue(i, "--manifest");
        else if (flag == "--base-directory")
            args.baseDir = requireValue(i, "--base-directory");
        else if (flag == "--namespace")
            args.namespaceName = requireValue(i, "--namespace");
        else if (flag == "--category")
            args.category = requireValue(i, "--category");
        else if (flag == "--output-cpp")
            args.outputCpp = requireValue(i, "--output-cpp");
        else if (flag == "--output-h")
            args.outputHeader = requireValue(i, "--output-h");
        else if (flag == "--output-register")
            args.outputRegister = requireValue(i, "--output-register");
        else if (flag == "--depfile")
            args.depfile = requireValue(i, "--depfile");
        else if (flag == "--split-count")
            args.splitCount = std::stoi(requireValue(i, "--split-count"));
        else
            throw std::runtime_error("Unknown flag: " + flag);
    }

    if (args.namespaceName.empty())
        throw std::runtime_error("--namespace is required");
    if (args.outputCpp.empty())
        throw std::runtime_error("--output-cpp is required");
    if (args.outputHeader.empty())
        throw std::runtime_error("--output-h is required");
    if (args.outputRegister.empty())
        throw std::runtime_error("--output-register is required");
    if (args.scanDir.empty() == args.manifestPath.empty())
        throw std::runtime_error(
            "exactly one of --scan-dir / --manifest is required");

    return args;
}

void runGenerate(int argc, char* argv[])
{
    auto args = parseGenerateArgs(argc, argv);

    auto files = args.manifestPath.empty() ? scanDirectory(args.scanDir)
                                           : readManifest(args.manifestPath);

    // Sort for deterministic output regardless of filesystem iteration order
    // (recursive_directory_iterator is unordered on most platforms).
    std::sort(files.begin(), files.end());

    writeFileIfChanged(args.outputHeader,
                       generateInitHeader(args.namespaceName));

    if (args.splitCount > 0)
    {
        // Split mode: the data bytes go into plain-C .c files (compiled by
        // the C front-end, which digests large brace-initializer arrays far
        // faster than C++), and --output-cpp holds only the small Entries
        // registry that references each array via extern "C". This restores
        // the historical "resources are C, not C++" property and lets the
        // build recompile only the bucket that actually changed.
        //
        // Resources are round-robined across exactly splitCount .c files,
        // named <namespace>_<b>.c next to outputCpp. res_embed_add declares
        // the matching OUTPUT/target_sources set — the count is fixed at
        // configure time, which is why even the build-time discovery modes
        // (scan-dir/manifest) can use a fixed bucket count. With splitCount
        // == file count, each bucket holds exactly one resource.
        writeFileIfChanged(args.outputCpp,
                           generateEntriesCpp(args.namespaceName,
                                              args.category,
                                              args.baseDir,
                                              files));

        auto outDir = fs::path(args.outputCpp).parent_path();
        auto buckets = static_cast<size_t>(args.splitCount);

        for (size_t b = 0; b < buckets; ++b)
        {
            auto cPath =
                (outDir
                 / (args.namespaceName + "_" + std::to_string(b) + ".c"))
                    .string();

            writeFileIfChanged(
                cPath,
                generateBucketDataC(args.namespaceName, files, b, buckets));
        }
    }
    else
    {
        writeFileIfChanged(args.outputCpp,
                           generateCombinedDataCpp(args.namespaceName,
                                                   args.category,
                                                   args.baseDir,
                                                   files));
    }

    writeFileIfChanged(args.outputRegister,
                       generateRegisterCpp(args.namespaceName));

    if (!args.depfile.empty())
    {
        auto depInputs = files;

        if (!args.manifestPath.empty())
            depInputs.insert(depInputs.begin(), args.manifestPath);

        // writeFileIfChanged would skip writes when the dep list is
        // unchanged, but ninja stat()'s the depfile every build — write
        // unconditionally so the mtime is always current.
        auto out = std::ofstream(args.depfile, std::ios::binary);

        if (!out)
            throw std::runtime_error("Error: cannot open depfile: " + args.depfile);

        out << generateDepfile(args.outputCpp, depInputs);
    }
}

std::string parseCommand(int argc, char* argv[])
{
    if (argc < 2)
    {
        throw std::runtime_error(
            "Usage: ResourceGenerator <command> ...\n"
            "Commands:\n"
            "  generate-data <output.c> <var_prefix> <input_file>\n"
            "  generate-registry <config_file>\n"
            "  generate --namespace <ns> --output-cpp <p> --output-h <p>\n"
            "           --output-register <p> [--depfile <p>]\n"
            "           [--split-count <n>] [--category <c>]\n"
            "           [--base-directory <d>]\n"
            "           (--scan-dir <d> | --manifest <f>)");
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
    else if (command == "generate")
    {
        runGenerate(argc, argv);
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
