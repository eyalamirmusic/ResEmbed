#include <fstream>
#include <iostream>
#include <vector>

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

void run(const std::string& input,
         const std::string& output,
         const std::string& resource)
{
    auto data = readDataFrom(input);

    auto out = std::ofstream(output);

    if (!out)
        throw std::runtime_error("Error: cannot open output file: " + output);

    out << "#include \"ResourceEmbedLib.h\"\n\n";
    out << "static const auto resource = Resources::Data(\"" << resource
        << "\", {\n";

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

    out << "});\n";

    if (!out)
        throw std::runtime_error("Error: failed to write output file: " + output);
}

void validateArgs(int argc)
{
    if (argc != 4)
    {
        throw std::runtime_error(
            "Usage: ResourceGenerator <input_file> <output_cpp> <resource_name>");
    }
}

int main(int argc, char* argv[])
{
    try
    {
        validateArgs(argc);
        run(argv[1], argv[2], argv[3]);
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}