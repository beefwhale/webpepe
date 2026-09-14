#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <iomanip>
#include <cstddef>
#include <array>
#include <span>

using std::vector;
using std::string;
using std::cout;
using std::byte;
using std::array;

static constexpr string IMAGE{"dawg.png"};
static constexpr array<uint8_t,8> PNG_SIGNATURE{0x89,0x50,0x4E,0x47,0x0D,0x0A,0x1A,0x0A};

class Object{
    public:
        string ext;
        vector<uint8_t> signature;
        int length;
        int chunk_type;
        int chunk_data;
        int crc;
};

// reads file as bytes into buffer
vector<char> readImageBytes(const string& filename){
    // open file stream as binary
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()){
        std::cerr << "Failed to open iamge file:" << filename << std::endl;
        return {}; 
    }

    // get file size and allowcate vector size
    std::streamsize size = file.tellg();
    if (size < 0)
    {
        std::cerr << "Something went wrong!" << std::endl;
    }
    vector<char> buffer(static_cast<size_t>(size));
    file.seekg(0,std::ios::beg);

    // read raw bytes into buffer
    if (file.read(buffer.data(),size)){
        cout << "Read file: " << size << " bytes" << std::endl;
    }

    return buffer;
}

void displayBytes(vector<char>& buffer){
    for (size_t i=0; i<8; i++){
        cout << std::hex
             << std::setw(2)
             << std::setfill('0')
             << (static_cast<unsigned int>(
                static_cast<unsigned char>(buffer[i])
             ))
             << ' ';
    }
    cout << "\n";
}

int main()
{
    // convert file into raw bytes
    vector<char> imageBytes = readImageBytes(IMAGE);
    displayBytes(imageBytes);
    return 0;
}

//2. Verify PNG signature

//3. Parse chunks:
   //[length][type][data][CRC]

//4. Read IHDR
   //→ width
   //→ height
   //→ bit depth
   //→ color type

//5. Concatenate IDAT chunks

//6. zlib/DEFLATE decompress IDAT

//7. Reverse PNG scanline filters

//8. Produce something like:
   //vector<uint8_t> pixels

//9. Feed those pixels into your JPEG encode
