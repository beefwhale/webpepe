#include <array>
#include <cstddef>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <vector>

using std::array;
using std::byte;
using std::cout;
using std::endl;
using std::string;
using std::vector;

static constexpr string IMAGE{"dawg.png"};
static constexpr array<uint8_t, 8> PNG_SIGNATURE{0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A};

class Chunk {
  public:
    size_t start;
    size_t end;
    int data_len;
    int chunk_type;
    int chunk_data;
    int crc;
};

class Object {
  public:
    string ext;
    vector<uint8_t> signature;
    vector<uint8_t> raw_image_bytes;
    vector<Chunk> chunk_collection;
};

// reads file as bytes into buffer vector<uint8_t>
vector<uint8_t> readImageBytes(const string &filename) {
    // open file stream as binary
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    if (!file.is_open()) {
        std::cerr << "Failed to open image file:" << filename << endl;
        return {};
    }

    // get file size and allowcate vector size
    std::streamsize size = file.tellg();
    if (size < 0) {
        std::cerr << "Something went wrong!" << endl;
    }
    vector<uint8_t> buffer(static_cast<size_t>(size));
    file.seekg(0, std::ios::beg);

    // read raw bytes into buffer
    if (file.read(reinterpret_cast<char *>(buffer.data()), size)) {
        cout << "Read file: " << size << " bytes" << endl;
    }

    return buffer;
}

// debugging use
void displayBytes(vector<uint8_t> &buffer, size_t start, size_t end) {
    for (int i = start; i < end; ++i) {
        cout << std::setw(2) << static_cast<unsigned int>(buffer[i]) << ' ';
    }
    cout << "\n";
}

// verify signature
bool verifySignature(vector<uint8_t> &imageBytes,
                     std::span<const uint8_t> sig) {
    for (size_t i = 0; i < sig.size(); ++i) {
        if (imageBytes[i] != sig[i]) {
            cout << imageBytes[i] << " " << sig[i] << endl;
            return false;
        }
    }
    return true;
}

// 3. Parse chunk:
//  [data_len : 4 bytes ]
//  [type : 4 bytes  ]
//  [data : 0 to max]
//  [CRC : 4 bytes]
void parse_chunk(vector<uint8_t> &imageBytes, size_t start) {
    uint32_t data_len = 0;
    vector<int> data;
    array<uint8_t, 4> type;
    array<uint8_t, 4> crc;

    // chunk data_len
    for (size_t i = start; i < start + 4; ++i) {
        data_len = (data_len << 8) | imageBytes[i]; // shift by 8 bits every byte to
                                                    // combine them into one value
    }

    // cycle through bytes to parse chunk
    size_t end = 4 + data_len + 4 + 4;

    int counter = 0;
    for (size_t i = start + 4; i < start + 4 + end; ++i) {
        // chunk type
        if (counter < 4) {
            type[counter] = imageBytes[i];
        }
        // chunk data
        else if (counter < 4 + data_len && data_len > 0) {
            data.push_back(imageBytes[i]);
        }
        // CRC
        else {
            crc[counter - 4 - data_len] = imageBytes[i];
        }
        ++counter;
    }
}

void determine_chunk_type() {}

int main() {
    // convert file into raw bytes and check for signature
    vector<uint8_t> imageBytes = readImageBytes(IMAGE);
    bool is_png = verifySignature(imageBytes, PNG_SIGNATURE);
    //   cout << is_png << endl;

    // IHDR and IEND chunk
    parse_chunk(imageBytes, 8);

    //   displayBytes(imageBytes, 8, 12);

    return 0;
}

// 4. Read IHDR
// → width
// → height
// → bit depth
// → color type

// 5. Concatenate IDAT chunks

// 6. zlib/DEFLATE decompress IDAT

// 7. Reverse PNG scanline filters

// 8. Produce something like:
// vector<uint8_t> pixels

// 9. Feed those pixels into your JPEG encode
