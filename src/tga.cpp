#include "tga.h"

#include <fstream>

static int const no_color_map = 0;
static int const uncompressed_truecolor = 2;
static int const color_map_size = 5;
/*static*/int /*const*/ tga_source_width    /*= 1296*/;
/*static*/ int /*const*/ tga_source_height   /*= 1024*/;
template <class T>
static bool readValue(std::istream &istream, T &value) {
	istream.read(reinterpret_cast<char*>(&value), sizeof(T));
	return istream.good();
}

template <class T>
static bool writeValue(std::ostream &ostream, T const value) {
	ostream.write(reinterpret_cast<char const *>(&value), sizeof(T));
	return ostream.good();
}

bool readTGA(std::istream &istream, std::vector<uint8_t> &data) {
	uint8_t id_length;
	if (!readValue(istream, id_length) || id_length != 0)
		return false;

	uint8_t color_map_type;
	if (!readValue(istream, color_map_type) || color_map_type != no_color_map)
		return false;

	uint8_t image_type;
	if (!readValue(istream, image_type) || image_type != uncompressed_truecolor)
		return false;

	istream.seekg(color_map_size, std::ios_base::cur);
	if (!istream.good())
		return false;

	uint16_t x, y;
	if (!readValue(istream, x) || x != 0)
		return false;

	if (!readValue(istream, y) || y != 0)
		return false;

	uint16_t w, h;/*
	if (!readValue(istream, w) || w != tga_source_width)
		return false;*/

    
	if (!readValue(istream, w))
		return false;

    tga_source_width    = w;

	if (!readValue(istream, h))
		return false;
    tga_source_height   = h;

	uint8_t bits_per_pixel;
	if (!readValue(istream, bits_per_pixel) || bits_per_pixel != tga_source_depth * tga_source_channels)
		return false;

	uint8_t descriptor;
	if (!readValue(istream, descriptor) || descriptor != 0)
		return false;

	int bytes = tga_source_width * tga_source_height * tga_source_channels;
	data.resize(bytes);
	istream.read(reinterpret_cast<char*>(data.data()), bytes);
	return istream.good();
}

bool readTGA(std::string const &path, std::vector<uint8_t> &data) {
	std::ifstream istream(path, std::ios_base::binary);
	if (!istream.good())
		return false;

	return readTGA(istream, data);
}

bool writeTGA(std::ostream &ostream, std::vector<uint8_t> const &data) {
	uint8_t id_length = 0;
	if (!writeValue<uint8_t>(ostream, id_length))
		return false;

	if (!writeValue<uint8_t>(ostream, no_color_map))
		return false;

	if (!writeValue<uint8_t>(ostream, uncompressed_truecolor))
		return false;

	for (int i = 0; i < color_map_size; ++i) {
		if (!writeValue<uint8_t>(ostream, 0))
			return false;
	}

	uint16_t x = 0;
	uint16_t y = 0;
	if (!writeValue(ostream, x) || !writeValue(ostream, y))
		return false;

	if (!writeValue<uint16_t>(ostream, tga_source_width))
		return false;

	if (!writeValue<uint16_t>(ostream, tga_source_height))
		return false;

	if (!writeValue<uint8_t>(ostream, tga_source_depth * tga_source_channels))
		return false;

	uint8_t descriptor = 0;
	if (!writeValue(ostream, descriptor))
		return false;

	int bytes = tga_source_width * tga_source_height * tga_source_channels;
	if (data.size() != bytes)
		return false;

	ostream.write(reinterpret_cast<char const *>(data.data()), data.size());
	return ostream.good();
}

bool writeTGA(std::string const &path, std::vector<uint8_t> const &data) {
	std::ofstream ostream(path, std::ios_base::binary);
	if (!ostream.good())
		return false;

	return writeTGA(ostream, data);
}