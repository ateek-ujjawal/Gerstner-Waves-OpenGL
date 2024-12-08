/** @file PPM.hpp
 *  @brief Class for working with PPM images
 *  
 *  Class for working with P3 PPM images specifically.
 *
 *  @author Ateek Ujjawal
 *  @bug No known bugs.
 */
#ifndef PPM_HPP
#define PPM_HPP

#include <string>
#include <vector>
#include <cstdint>

class PPM{
public:
    // Default constructor
    PPM();
    // Constructor loads a filename with the .ppm extension
    PPM(std::string fileName);
    // Destructor clears any memory that has been allocated
    ~PPM();
    // Saves a PPM Image to a new file.
    void savePPM(std::string outputFileName) const;
    // Darken halves (integer division by 2) each of the red, green
    // and blue color components of all of the pixels
    // in the PPM. Note that no values may be less than
    // 0 in a ppm.
    void darken();
    // Lighten doubles (integer multiply by 2) each of the red, green
    // and blue color components of all of the pixels
    // in the PPM. Note that no values may be greater than
    // 255 in a ppm.
    void lighten();
    // Sets a pixel to a specific R,G,B value 
    // Note: You do not have to use this function in your implementation,
    //       but it is probably a useful helper function to have.
    void setPixel(int x, int y, uint8_t R, uint8_t G, uint8_t B);
    // Flips the PPM image
    void flipPPM();
    // Returns the raw pixel data in an array.
    // You may research what 'inline' does.
    // You may also want to research what 'const' does on a function.
    // In brief, 'const' gaureentees that we are not modifying 
    // any member variables in a class, and this is useful if we are
    // returning private member variables.
    inline std::vector<uint8_t> pixelData() const { return m_PixelData; }
    // Returns image width
    inline int getWidth() const { return m_width; }
    // Returns image height
    inline int getHeight() const { return m_height; }
// NOTE:    You may add any helper functions you like in the
//          private section.
private:    
    // Store the raw pixel data here
    // Data is R,G,B format
    // Note: Yes, you are allowed to replace 'uint8_t* m_PixelDatal' with a std::vector<uint8_t> m_PixelData.
    //       In fact, using a std::vector will likely make your life easier.    
    std::vector<uint8_t> m_PixelData;
    // Store width and height of image.
    int m_width{0};
    int m_height{0};
    int m_maxRange{0};
};


#endif
