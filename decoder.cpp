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
extern "C" {
#include <zlib.h>
}

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
        end = start + data_len + 12;
        data_index = start + 8;

        size_t counter = 0;
        for (size_t i = start + 4; i < end; ++i) {
            // chunk type
            if (counter < 4) {
                type[counter] = imageBytes[i];
            }
            // crc
            else if (counter > (4 + data_len)) {
                crc[counter - 4 - data_len] = imageBytes[i];
            }
            ++counter;
        }
    }

  public:
    size_t start;
    size_t end;
    size_t data_index; // TODO: should i store indexes as size_t or uint32_t?
    uint32_t data_len = 0;
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
    std::unordered_map<string, vector<size_t>> chunk_collection_types;

    // constructors
    Image(const string &filename) {
        raw_image_bytes = readImageBytes(filename);
        ext = extractFileExt(filename);
        // parse chunks
        size_t counter = 0;
        if (raw_image_bytes.size() > 0) {
            size_t chunk_index = 8;
            while (chunk_index < raw_image_bytes.size()) {
                Chunk chunk = Chunk(raw_image_bytes, chunk_index);
                chunk_collection.push_back(chunk);

                // get chunk type and save index inside chunk collection
                string chunk_type_str(chunk.type.begin(), chunk.type.end());
                chunk_collection_types[chunk_type_str].push_back(counter);

                // increment indexes
                chunk_index = chunk.end;
                ++counter;
            }
        }
    }
};

array<size_t, 7> readHeaderDataBytes(const vector<uint8_t> &imageBytes, const size_t &start) {
    //    Width:              4 bytes
    //    Height:             4 bytes
    //    Bit depth:          1 byte
    //    Color type:         1 byte
    //    Compression method: 1 byte
    //    Filter method:      1 byte
    //    Interlace method:   1 byte
    array<size_t, 7> data{};
    for (size_t i = 0; i < 13; ++i) {
        if (i < 4) { // width
            data[0] = (data[0] << 8) | imageBytes[start + i];
        } else if (i < 8) { // height
            data[1] = (data[1] << 8) | imageBytes[start + i];
        } else {
            data[i - 6] = imageBytes[start + i];
        }
    }
    return data;
}

vector<uint8_t> zlib(int &width, int &height, vector<uint8_t> &idat_chunks) {
    // each line has 1 filter and 4 bytes per pixel
    vector<uint8_t> scanlines(height * (1 + (4 * width)));
    // produce the length of output buffer in zlibs format: unsigned long int
    // zlib updates this values after decompressions
    uLongf output_size = static_cast<uLongf>(scanlines.size());

    // decompress
    int status = uncompress(scanlines.data(), &output_size, idat_chunks.data(), static_cast<uLongf>(idat_chunks.size()));

    if (status != Z_OK) {
        throw std::runtime_error("Zlib decompression failed: " + std::to_string(status));
    }

    scanlines.resize(output_size);
    return scanlines;
}

int paeth_predictor(int &a, int &b, int &c) {
    int p = a + b - c;
    int pa = std::abs(p - a);
    int pb = std::abs(p - b);
    int pc = std::abs(p - c);
    if (pa <= pb && pa <= pc) {
        return a;
    } else if (pb <= pa && pb <= pc) {
        return b;
    } else {
        return c;
    }
}

int main() {
    // create image object
    string demo = {"dawg.png"};
    Image image_obj = Image(demo);

    // Read IHDR chunk
    const Chunk &header_chunk = image_obj.chunk_collection[0];
    cout << "Header Chunk Data Length: " << header_chunk.data_len << endl;
    array<size_t, 7> header_chunk_data = readHeaderDataBytes(image_obj.raw_image_bytes, header_chunk.data_index);
    cout << "Width: " << header_chunk_data[0] << " Height: " << header_chunk_data[1] << endl;

    // find out sizes of IDAT chunks
    size_t total_idat_chunks_len{0};
    for (size_t c : image_obj.chunk_collection_types["IDAT"]) {
        total_idat_chunks_len += image_obj.chunk_collection[c].data_len;
    }
    vector<uint8_t> idat_chunks{};
    idat_chunks.reserve(total_idat_chunks_len); // to prevent constant realloc

    // Concatenate IDAT chunks
    for (size_t c : image_obj.chunk_collection_types["IDAT"]) {
        Chunk &chunk = image_obj.chunk_collection[c];
        // read chunk and copy data bytes of each chunk
        auto start = image_obj.raw_image_bytes.begin() + chunk.data_index; // create an iterator that points to the object we want
        idat_chunks.insert(idat_chunks.end(), start, start + chunk.data_len);
    }
    cout << "IDAT chunks data lengths : " << idat_chunks.size() << endl;
    cout << "Filter Method: " << header_chunk_data[5] << endl;
    cout << "Interlacing: " << header_chunk_data[6] << endl;

    int colour_type = header_chunk_data[3];
    int width = header_chunk_data[0];
    int height = header_chunk_data[1];
    int bit_depth = header_chunk_data[2];

    // zlib/DEFLATE decompress IDAT
    vector<uint8_t> pixels{};
    if (header_chunk_data[4] == 0) {
        pixels = zlib(width, height, idat_chunks);
    } else {
        throw std::runtime_error("Compression method invalid");
    }

    // TODO: replace the strange size_t types on the len and index vars
    // check for filters
    if (header_chunk_data[5] == 0) {
        // calculate len of each row based on colourtype of image
        // id      supported bit depth
        // 0       1,2,4,8,16  Each pixel is a grayscale sample.
        // 2       8,16        Each pixel is an R,G,B triple.
        // 3       1,2,4,8     Each pixel is a palette index; a PLTE chunk must appear.
        // 4       8,16        Each pixel is a grayscale sample, followed by an alpha sample.
        // 6       8,16        Each pixel is an R,G,B triple, followed by an alpha sample.
        int row_bytes = 0;
        int channels = 0;
        if (colour_type == 0) { // Greyscale
            row_bytes = 1 + width;
            channels = 1;
        } else if (colour_type == 2) { // RGB
            row_bytes = 1 + width * 3;
            channels = 3;
        } else if (colour_type == 6) { // RGBA
            row_bytes = 1 + width * 4;
            channels = 4;
        } else if (colour_type == 3 || colour_type == 4) {
            throw std::runtime_error("Im too lazy to support images with bytes that are not directly RGB");
        }
        int bpp = (channels * bit_depth + 7) / 8; // byte per pixel

        // TODO: implement filter reversal
        // erase filter byte of every row by traversing backwards
        for (int row = height - 1; row >= 0; --row) {
            int index = row * row_bytes; // filter byte
            int end = index + row_bytes; // end of row
            switch (pixels[index]) {
            case 0: // None
                break;
            case 1: // Sub
                // stores the difference between each byte and the corresponding byte of the pixel to the left
                for (int x = index + 1 + bpp; x < end; ++x) {
                    pixels[x] = static_cast<uint8_t>(pixels[x] + pixels[x - bpp]);
                }
                break;
            case 2: // Up
                if (row > 0) {
                    for (int x = index + 1; x < end; ++x) {
                        pixels[x] = static_cast<uint8_t>(pixels[x] + pixels[x - row_bytes]);
                    }
                }
                break;
            case 3: // Average
                // stores the avg btwn the top and left pixels
                for (int x = index + 1; x < end; ++x) {
                    int left = 0;
                    int up = 0;
                    if (x >= index + 1 + bpp) {
                        left = pixels[x - bpp];
                    }
                    if (row > 0) {
                        up = pixels[x - row_bytes];
                    }
                    pixels[x] = static_cast<uint8_t>(pixels[x] + std::floor((left + up) / 2));
                }
                break;
            case 4: // Paeth
                    // simple linear function of the 3 neighbouring pixels left, , up, upper left
                for (int x = index + 1; x < end; ++x) {
                    int left = 0;
                    int up = 0;
                    int upleft = 0;
                    if (x >= index + 1 + bpp) {
                        left = pixels[x - bpp];
                    }
                    if (row > 0) {
                        up = pixels[x - row_bytes];
                    }
                    if (row > 0 && x >= index + 1 + bpp) {
                        upleft = pixels[x - row_bytes - bpp];
                    }
                    pixels[x] = pixels[x] + static_cast<uint8_t>(paeth_predictor(left, up, upleft));
                }
                break;
            default:
                throw std::runtime_error("This filter byte is funky");
            }
            // cleanup pixel after reversal
            pixels.erase(pixels.begin() + index);
        }
    } else {
        throw std::runtime_error("Filtering method unsupported");
    }
    // TOOD: interlacing

    // Produce final pixels

    return 0;
}

// 9. Feed those pixels into your JPEG encode
