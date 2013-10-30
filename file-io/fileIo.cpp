#include <opencv2/core/core.hpp>
#include <iostream>
#include <string>

static void showUsage(const char *av0)
{
    std::cout
        << std::endl
        << av0 << ": Demonstrate serializing data to and from files."
        << std::endl << std::endl
        << "Usage: " <<  av0 << " <file><ext>" << std::endl
        << std::endl
        << "Where: <file><ext> is the name of a file to read and write."
        << std::endl
        << "       The <ext> extension may be: '.xml' or '.yaml'" << std::endl
        << "       to serialize data as XML or as YAML, respectively."
        << "       The default is YAML if <ext> neither '.xml' nor '.yaml'."
        << std::endl << std::endl
        << "       A '.gz' suffix designates compression such that:"
        << std::endl
        << "           <file>.xml.gz  means use gzipped XML." << std::endl
        << "           <file>.yaml.gz means use gzipped YAML." << std::endl
        << "           '<file>.gz' is equivalent to '<file>.yaml.gz'."
        << std::endl
        << std::endl
        << "Example: " << av0 << " somedata.xml.gz" << std::endl
        << std::endl;
}

struct SomeData
{
    int anInt;
    double aDouble;
    std::string aString;

    SomeData(): anInt(1), aDouble(1.1), aString("default SomeData ctor") {}

    SomeData(int): anInt(97), aDouble(CV_PI), aString("mydata1234") {}

    void write(cv::FileStorage &fs) const
    {
        fs << "{"
           << "anInt"   << anInt
           << "aDouble" << aDouble
           << "aString" << aString
           << "}";
    }
    void read(const cv::FileNode &node)
    {
        anInt   = int(        node["anInt"  ]);
        aDouble = double(     node["aDouble"]);
        aString = std::string(node["aString"]);
    }
};

// Write x to FileStorage.
//
static void write(cv::FileStorage &fs, const std::string &, const SomeData &x)
{
    x.write(fs);
}

// Read x from node using default_value if node is not a SomeData.
//
static void read(const cv::FileNode &node, SomeData &x,
                 const SomeData &default_value = SomeData())
{
    if (node.empty()) {
        x = default_value;
    } else {
        x.read(node);
    }
}

static std::ostream &operator<<(std::ostream &out, const SomeData &m)
{
    out << "anInt"   << " = "         << m.anInt
        << ", "
        << "aDouble" << " = "         << m.aDouble
        << ", "
        << "aString" << " = " << "\"" << m.aString << "\"";

    return out;
}

static bool writeSomeStuff(const char *filename)
{
    std::cout << std::endl << "Writing " << filename << " ... ";
    cv::Mat ucharEye = cv::Mat_<uchar>::eye(3, 3);
    cv::Mat doubleZeros = cv::Mat_<double>::zeros(3, 1);
    SomeData someData(1);
    cv::FileStorage fs(filename, cv::FileStorage::WRITE);
    fs << "someInteger" << 100
       << "stringSequence" << "[" << "image.jpg" << "wild" << "lena.jpg" << "]"
       << "stringToIntMap" << "{" << "One" << 1 << "Two" << 2 << "}"
       << "ucharEye" << ucharEye
       << "doubleZeros" << doubleZeros
       << "someData" << someData;
    fs.release();                       // This should not be necessary.
    std::cout << "done." << std::endl;
    return true;
}


static bool readMatAndSomeData(cv::FileStorage &fs)
{
    cv::Mat ucharEye;
    fs["ucharEye"]    >> ucharEye;
    cv::Mat doubleZeros;
    fs["doubleZeros"] >> doubleZeros;
    SomeData someData;
    fs["someData"]    >> someData;
    std::cout << std::endl
              << "ucharEye"    " = " << std::endl << ucharEye    << std::endl
              << "doubleZeros" " = " << std::endl << doubleZeros << std::endl
              << std::endl
              << "someData"    " = " << someData << std::endl
              << std::endl;
    std::cout << "Read 'no thing' into a SomeData for default." << std::endl;
    fs["no thing"] >> someData;
    std::cout << "someData: " << someData << std::endl;
    return true;
}


static bool readSomeInteger(cv::FileStorage &fs)
{
    const cv::FileNode &fnSomeInteger = fs["someInteger"];
    if (fnSomeInteger.isInt()) {
        const int someInteger = fnSomeInteger;
        std::cout << "someInteger" << " = " << someInteger << std::endl;
        return true;
    }
    std::cerr << "someInteger is not an integer" << std::endl;
    return false;
}


static bool readStringSequence(cv::FileStorage &fs)
{
    const cv::FileNode &fnStringSequence = fs["stringSequence"];
    if (fnStringSequence.isSeq()) {
        const cv::FileNodeIterator pEnd = fnStringSequence.end();
        cv::FileNodeIterator p = fnStringSequence.begin();
        const char *separator = "stringSequence" " = " "[ ";
        for (; p != pEnd; ++p) {
            if ((*p).type() == cv::FileNode::STRING) {
                const std::string s(*p);
                std::cout << separator << "\"" << s << "\"";
                separator = " ";
            } else {
                std::cerr << "stringSequence element is not a string!"
                          << std::endl;
                return false;
            }
        }
        std::cout << " ]" << std::endl;
        return true;
    }
    std::cerr << "stringSequence is not a sequence!" << std::endl;
    return false;
}


static bool readStringToIntMap(cv::FileStorage &fs)
{
    const cv::FileNode &fnStringToIntMap = fs["stringToIntMap"];
    if (fnStringToIntMap.isMap()) {
        const cv::FileNodeIterator pEnd = fnStringToIntMap.end();
        cv::FileNodeIterator p = fnStringToIntMap.begin();
        const char *separator = "stringToIntMap" " = " "{ ";
        for (; p != pEnd; ++p) {
            if ((*p).isNamed()) {
                const std::string n((*p).name());
                const int v = *p;
                std::cout << separator << "\"" << n << "\"" << " " << v;
                separator = ", ";
            } else {
                std::cerr << "stringToIntMap node is not named!" << std::endl;
                return false;
            }
        }
        std::cout << " }" << std::endl;
        return true;
    }
    std::cerr << "stringToIntMap is not a map!" << std::endl;
    return false;
}

static bool readSomeStuff(const char *filename)
{
    std::cout << "Reading " << filename << " back now."
              << std::endl << std::endl;
    cv::FileStorage fs(filename, cv::FileStorage::READ);
    bool ok = fs.isOpened();
    if (!ok) std::cerr << "Failed to open " << filename << std::endl;
    ok = ok
        && readSomeInteger(fs)
        && readStringSequence(fs)
        && readStringToIntMap(fs)
        && readMatAndSomeData(fs);
    return ok;
}

int main(int ac, char *av[])
{
    const bool ok
        =  ac == 2
        && writeSomeStuff(av[1])
        && readSomeStuff(av[1]);
    if (ok) {
        std::cout << std::endl
                  << "Tip: Open " << av[0]
                  << " with a text editor to see the serialized data."
                  << std::endl;
        return 0;
    }
    showUsage(av[0]);
    return 1;
}