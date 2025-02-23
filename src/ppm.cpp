#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include "PPM.hpp"

// Default constructor
PPM::PPM() {}

// Constructor loads a filename with the .ppm extension
PPM::PPM(std::string fileName){
    // Open file
    std::ifstream inputFile;
    inputFile.open(fileName);

    bool foundP3 = false;
    bool foundDimensions = false;
    bool foundRange = false;
    bool readingPixels = false;

    // If file is open then
    if(inputFile.is_open()) {
        std::string line;

        // Read one line
        while(std::getline(inputFile, line)) {

            // Break line into chunks
            std::stringstream stream(line);
            std::string chunk;

            // Check each chunk if it is a comment, magic no., range, dimension or pixel
            while(stream >> chunk) {
                if (chunk[0] == '#') {
                    break;
                } else if (false == foundP3) {
                    if (chunk == "P3") {
                        foundP3 = true;
                    }
                } else if (false == foundDimensions) {
                    m_width = std::stoi(chunk);
                    stream >> chunk;
                    m_height = std::stoi(chunk);
                    foundDimensions = true;
                } else if (false == foundRange) {
                    m_maxRange = std::stoi(chunk);
                    foundRange = true;
                } else {
                    int data = std::stoi(chunk);
                    m_PixelData.push_back(data);
                }
            }
        }
    }
}

// Destructor deletes(delete or delete[]) any memory that has been allocated
// or otherwise calls any 'shutdown' or 'destroy' routines for this deletion
// to occur.
PPM::~PPM(){
    // No use of dynamic allocation hence no reclamation
}

// Saves a PPM Image to a new file.
void PPM::savePPM(std::string outputFileName) const {
    std::ofstream myFile;
    myFile.open(outputFileName);
    myFile << "P3\n";
    myFile << "# saved by us\n";
    myFile << m_width << " " << m_height << std::endl;
    myFile << m_maxRange << std::endl;

    uint8_t t;
    for(long i = 0; i < m_PixelData.size(); i++) {
        myFile << (int) m_PixelData[i] << "\n";
    }

    myFile.close();
}

// Darken halves (integer division by 2) each of the red, green
// and blue color components of all of the pixels
// in the PPM. Note that no values may be less than
// 0 in a ppm.
void PPM::darken(){
    int data;
    for(int i = 0; i < m_PixelData.size(); i++) {
        data = m_PixelData[i] / 2;
        m_PixelData[i] = std::clamp(data, 0, m_maxRange);
    }
}

// Lighten doubles (integer multiply by 2) each of the red, green
// and blue color components of all of the pixels
// in the PPM. Note that no values may be greater than
// 255 in a ppm.
void PPM::lighten(){
    int data;
    for(int i = 0; i < m_PixelData.size(); i++) {
        data = m_PixelData[i] * 2;
        m_PixelData[i] = std::clamp(data, 0, m_maxRange);
    }
}

// Flips the PPM image
void PPM::flipPPM() {
    // std::vector<uint8_t> flippedPixelData;
    // for (int i = m_height; i >= 0; i--) {
    //     for (int j = m_width; j >= 0; j--) {
    //         int skip = (m_width * i) + j;
    //         flippedPixelData.push_back(m_PixelData[(skip * 3) + 2]);
    //         flippedPixelData.push_back(m_PixelData[(skip * 3) + 1]);
    //         flippedPixelData.push_back(m_PixelData[(skip * 3)]);
    //     }
    // }
    // m_PixelData = flippedPixelData;

    uint8_t* copyData = new uint8_t[m_width*m_height*3];
    for(int i =0; i < m_width*m_height*3; ++i){
        copyData[i]=m_PixelData[i];
    }
    //memcpy(copyData,m_pixelData,(m_width*m_height*3)*sizeof(uint8_t));
    unsigned int pos = (m_width*m_height*3)-1;
    for(int i =0; i < m_width*m_height*3; i+=3){
        m_PixelData[pos]=copyData[i+2];
        m_PixelData[pos-1]=copyData[i+1];
        m_PixelData[pos-2]=copyData[i];
        pos-=3;
    }
    delete[] copyData;
}

// Sets a pixel to a specific R,G,B value 
// Note: You do not *have* to use setPixel in your implementation, but
//       it may be useful to implement.
void PPM::setPixel(int x, int y, uint8_t R, uint8_t G, uint8_t B){
    int skip = (m_width * x) + y;
    m_PixelData[skip * 3] = R;
    m_PixelData[(skip * 3) + 1] = G;
    m_PixelData[(skip * 3) + 2] = B;
}
