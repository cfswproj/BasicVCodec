#ifndef TGA_H
#define TGA_H

#include <cstdint>

#include <istream>
#include <vector>

/*static*/extern int /*const*/ tga_source_width    /*= 1296*/;
/*static*/ extern int /*const*/ tga_source_height   /*= 1024*/;
static int const tga_source_depth    = 8;
static int const tga_source_channels = 3;

bool readTGA(std::istream &, std::vector<uint8_t> &);
bool readTGA(std::string const &, std::vector<uint8_t> &);

bool writeTGA(std::ostream &, std::vector<uint8_t> const &);
bool writeTGA(std::string const &, std::vector<uint8_t> const &);

#endif