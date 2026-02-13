#include <cstdlib>
#include <fstream>
#include <iostream>
#include <vector>



int main(int argc, char* argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: ResourceGenerator <input_file> <output_cpp> <resource_name>\n";
        return EXIT_FAILURE;
    }

    const char* inputPath = argv[1];
    const char* outputPath = argv[2];
    const char* resourceName = argv[3];

    std::ifstream in(inputPath, std::ios::binary);
    if (!in)
    {
        std::cerr << "Error: cannot open input file: " << inputPath << "\n";
        return EXIT_FAILURE;
    }

    std::vector<unsigned char> data(
        (std::istreambuf_iterator<char>(in)),
        std::istreambuf_iterator<char>());
    in.close();

    std::ofstream out(outputPath);
    if (!out)
    {
        std::cerr << "Error: cannot open output file: " << outputPath << "\n";
        return EXIT_FAILURE;
    }

    out << "#include \"ResourceEmbedLib.h\"\n\n";
    out << "static const auto resource = Resources::Data(\"" << resourceName << "\", {\n";

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
    {
        std::cerr << "Error: failed to write output file: " << outputPath << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
