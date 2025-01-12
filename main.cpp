#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <queue>
#include <bitset>

struct HuffmanNode {
    char data;
    unsigned frequency;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(char data, unsigned frequency)
        : data(data), frequency(frequency), left(nullptr), right(nullptr) {}

    bool operator>(const HuffmanNode& other) const {
        return frequency > other.frequency;
    }
};

void generateHuffmanCodes(HuffmanNode* root, const std::string& str, std::unordered_map<char, std::string>& huffmanCodes) {
    if (!root)
        return;

    if (!root->left && !root->right) {
        huffmanCodes[root->data] = str;
    }

    generateHuffmanCodes(root->left, str + "0", huffmanCodes);
    generateHuffmanCodes(root->right, str + "1", huffmanCodes);
}

std::pair<std::string, std::unordered_map<char, std::string>> compressData(const std::vector<unsigned char>& data) {
    std::unordered_map<char, unsigned> frequencyMap;
    for (unsigned char byte : data) {
        frequencyMap[byte]++;
    }

    std::priority_queue<HuffmanNode*, std::vector<HuffmanNode*>, std::greater<>> minHeap;
    for (auto& pair : frequencyMap) {
        minHeap.push(new HuffmanNode(pair.first, pair.second));
    }

    while (minHeap.size() > 1) {
        HuffmanNode* left = minHeap.top();
        minHeap.pop();
        HuffmanNode* right = minHeap.top();
        minHeap.pop();

        HuffmanNode* newNode = new HuffmanNode('\0', left->frequency + right->frequency);
        newNode->left = left;
        newNode->right = right;

        minHeap.push(newNode);
    }

    std::unordered_map<char, std::string> huffmanCodes;
    generateHuffmanCodes(minHeap.top(), "", huffmanCodes);

    std::string compressedData;
    for (unsigned char byte : data) {
        compressedData += huffmanCodes[byte];
    }

    return {compressedData, huffmanCodes};
}

std::vector<unsigned char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    std::vector<unsigned char> fileData;

    if (file) {
        fileData.assign((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());
    }

    return fileData;
}

void writeHuffmanArchive(const std::string& filename, const std::vector<std::string>& filenames) {
    std::ofstream archive(filename + std::string(".hfc"), std::ios::binary);

    if (!archive.is_open()) {
        std::cerr << "Failed to open archive file!" << std::endl;
        return;
    }

    size_t fileCount = filenames.size();
    archive.write(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

    for (const std::string& filename : filenames) {
        std::vector<unsigned char> fileData = readFile(filename);

        auto [compressedData, huffmanCodes] = compressData(fileData);

        size_t nameLength = filename.size();
        archive.write(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        archive.write(filename.c_str(), nameLength);

        size_t compressedDataSize = compressedData.size();
        archive.write(reinterpret_cast<char*>(&compressedDataSize), sizeof(compressedDataSize));
        archive.write(compressedData.c_str(), compressedDataSize);

        size_t codeCount = huffmanCodes.size();
        archive.write(reinterpret_cast<char*>(&codeCount), sizeof(codeCount));
        for (const auto& pair : huffmanCodes) {
            size_t codeLength = pair.second.size();
            archive.write(reinterpret_cast<char*>(&codeLength), sizeof(codeLength));
            archive.write(pair.second.c_str(), codeLength);
            archive.write(&pair.first, sizeof(pair.first));
        }
    }

    archive.close();
    std::cout << "Archive created successfully: " + filename + std::string(".hfc") << std::endl;
}

std::vector<unsigned char> decompressData(const std::string& compressedData, const std::unordered_map<std::string, char>& huffmanDecodeTable) {
    std::vector<unsigned char> decompressedData;
    std::string currentCode;
    for (char bit : compressedData) {
        currentCode += bit;

        if (huffmanDecodeTable.find(currentCode) != huffmanDecodeTable.end()) {
            decompressedData.push_back(huffmanDecodeTable.at(currentCode));
            currentCode.clear();
        }
    }

    return decompressedData;
}

void extractHuffmanArchive(const std::string& archiveFile) {
    std::ifstream archive(archiveFile + std::string(".hfc"), std::ios::binary);

    if (!archive.is_open()) {
        std::cerr << "Failed to open archive file!" << std::endl;
        return;
    }

    size_t fileCount;
    archive.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

    for (size_t i = 0; i < fileCount; ++i) {
        size_t nameLength;
        archive.read(reinterpret_cast<char*>(&nameLength), sizeof(nameLength));
        std::vector<char> nameBuffer(nameLength);
        archive.read(nameBuffer.data(), nameLength);
        std::string filename(nameBuffer.begin(), nameBuffer.end());

        size_t compressedDataSize;
        archive.read(reinterpret_cast<char*>(&compressedDataSize), sizeof(compressedDataSize));
        std::vector<char> compressedDataBuffer(compressedDataSize);
        archive.read(compressedDataBuffer.data(), compressedDataSize);
        std::string compressedData(compressedDataBuffer.begin(), compressedDataBuffer.end());

        size_t codeCount;
        archive.read(reinterpret_cast<char*>(&codeCount), sizeof(codeCount));

        std::unordered_map<std::string, char> huffmanDecodeTable;
        for (size_t j = 0; j < codeCount; ++j) {
            size_t codeLength;
            archive.read(reinterpret_cast<char*>(&codeLength), sizeof(codeLength));
            std::vector<char> codeBuffer(codeLength);
            archive.read(codeBuffer.data(), codeLength);
            std::string code(codeBuffer.begin(), codeBuffer.end());

            char symbol;
            archive.read(&symbol, sizeof(symbol));

            huffmanDecodeTable[code] = symbol;
        }

        std::vector<unsigned char> decompressedData = decompressData(compressedData, huffmanDecodeTable);

        std::ofstream outputFile(filename, std::ios::binary);
        if (outputFile.is_open()) {
            outputFile.write(reinterpret_cast<char*>(decompressedData.data()), decompressedData.size());
            outputFile.close();
            std::cout << "Extracted: " << filename << std::endl;
        } else {
            std::cerr << "Failed to extract file: " << filename << std::endl;
        }
    }

    archive.close();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "[1] Create HFC Archive: ./hfc compress <archive_name> <file1> <file2> ..." << std::endl;
        std::cerr << "[2] Extract HFC Archive: ./hfc extract <archive_name>" << std::endl;
        return 1;
    }

    std::string command = argv[1];

    if (command == "compress") {
        if (argc < 4) {
            std::cerr << "Usage: compress <archive_name> <file1> <file2> ..." << std::endl;
            return 1;
        }

        std::string archiveName = argv[2];
        std::vector<std::string> filenames;
        for (int i = 3; i < argc; ++i) {
            filenames.push_back(argv[i]);
        }

        writeHuffmanArchive(archiveName, filenames);

    } else if (command == "extract") {
        if (argc != 3) {
            std::cerr << "Usage: extract <archive_name>" << std::endl;
            return 1;
        }

        std::string archiveName = argv[2];
        extractHuffmanArchive(archiveName);

    } else {
        std::cerr << "Unknown command: " << command << std::endl;
        return 1;
    }

    return 0;
}