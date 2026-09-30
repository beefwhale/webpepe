#include <array>
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

using std::array;
using std::byte;
using std::cout;
using std::endl;
using std::string;
using std::vector;

class Chunk {
  private:
    void parse_chunk(vector<uint8_t> &imageBytes) {
        //  [data_len : 4 bytes ]
        //  [type : 4 bytes  ]
        //  [data : 0 to max]
        //  [CRC : 4 bytes]

        // chunk data_len
        for (size_t i = start; i < start + 4; ++i) {
            data_len = (data_len << 8) | imageBytes[i]; // shift by 8 bits every byte to combine them into one value
        }

        // cycle through bytes to parse chunk
        end = data_len + 12;

        size_t counter = 0;
        for (size_t i = start + 4; i < start + 4 + end; ++i) {
            // chunk type
            if (counter < 4) {
                type[counter] = imageBytes[i];
            }
            // chunk data
            else if (counter < (4 + data_len) && data_len > 0) {
                data.push_back(imageBytes[i]);
            }
            // CRC
            else {
                crc[counter - 4 - data_len] = imageBytes[i];
            }
            ++counter;
        }
    }

  public:
    size_t start;
    size_t end;
    size_t data_len = 0;
    vector<int> data;
    array<uint8_t, 4> type;
    array<uint8_t, 4> crc;

    // constructor
    Chunk(vector<uint8_t> &imageBytes, size_t chunk_index) {
        start = chunk_index;
        parse_chunk(imageBytes);
    }
};

class Image {
  private:
    inline static const std::unordered_map<string, array<uint8_t, 8>> supported_ext{
        {"png", {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A}}};
    // extract and verify file signature
    string extractFileExt(const string &filename) {
        std::filesystem::path path(filename);
        string unverified_ext = path.extension().string();
        if (!unverified_ext.empty()) {
            unverified_ext.erase(0, 1); // remove the dot at the front
        }
        // verify file ext
        if (verifySignature(unverified_ext) > 0) {
            return ext;
        }
        return "";
    }
    bool verifySignature(string &unverified_ext) {
        for (size_t i = 0; i < supported_ext.at(unverified_ext).size(); ++i) {
            if (raw_image_bytes[i] != supported_ext.at(unverified_ext)[i]) {
                cout << raw_image_bytes[i] << " " << supported_ext.at(unverified_ext)[i] << endl;
                return false;
            }
        }
        return true;
    }
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

  public:
    string ext;
    vector<uint8_t> signature;
    vector<uint8_t> raw_image_bytes;
    vector<Chunk> chunk_collection;

    // constructors
    Image(const string &filename) {
        raw_image_bytes = readImageBytes(filename);
        ext = extractFileExt(filename);
        // parse chunks
        if (raw_image_bytes.size() > 0) {
            size_t chunk_index = 8;
            while (chunk_index < raw_image_bytes.size()) {
                Chunk chunk = Chunk(raw_image_bytes, chunk_index);
                chunk_collection.push_back(chunk);
                chunk_index = chunk.end;
            }
        }
    }
};

int main() {
    // create image object
    string demo = {"dawg.png"};
    Image image_obj = Image(demo);

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
