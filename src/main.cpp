#include "tga.h"

#include <cassert>

#include <iomanip>
#include <iostream>
#include <sstream>

#include "encoder.hpp"
#include "decoder.hpp"

static int const frame_offset = 11;
static int const frame_count  = 3;
static int bytes_per_frame;// = tga_source_width * tga_source_height * tga_source_channels;

void encode(std::vector<uint8_t> const *inputs, std::vector<uint8_t> &output) {
    EncoderDesc encDesc;

    encDesc.FrameDesc_t.FrameFmt        = IMAGE_COLOR_FORMAT_BGR;
    encDesc.FrameDesc_t.FrameWidth      = tga_source_width;
    encDesc.FrameDesc_t.FrameHeight     = tga_source_height;
    ENCFRAME_TYPE encPattern[]          = { ENCFRAME_TYPE_I, 
                                            ENCFRAME_TYPE_P,
                                            ENCFRAME_TYPE_P,
                                            ENCFRAME_TYPE_P,
                                            ENCFRAME_TYPE_P,
                                          };

    encDesc.RDDesc.QP                   = 6;

    const int encFramePatternSize       = sizeof(encPattern) / sizeof(ENCFRAME_TYPE);
    for (int i = 0; i < encFramePatternSize; i++)
        encDesc.EncFramePattern.push_back(encPattern[i]);

    encDesc.MESearchPattern             = ENC_ME_SEARCH_PATTERN_NULL;
    vector<uint8_t> bitstream;
    Encoder encoder(encDesc);
    encoder.init();

    vector<EncFrameInfo> encFrameInfoVec;
    encFrameInfoVec.resize(frame_count);

    // Begin the encode process.
	size_t encBSBytePos                 = 0;
	for (int i = 0; i < frame_count; ++i) 
    {
        Frame frameIn(inputs[i].data(), encDesc.FrameDesc_t);

        encFrameInfoVec[i]              = encoder.encodeFrame(frameIn, bitstream);

        encoder.testDecode(bitstream); 
        encBSBytePos                    = 0;
		for (int j = 0; j < bitstream.size(); ++j)
			output.push_back(bitstream[encBSBytePos+j]);

        encBSBytePos                    += bitstream.size();
	}
}

void decode(std::vector<uint8_t> const &input, std::vector<uint8_t> *outputs) {
    DecoderDesc decDesc;

    decDesc.FrameDesc_t.FrameFmt        = IMAGE_COLOR_FORMAT_BGR;
    decDesc.FrameDesc_t.FrameWidth      = tga_source_width;
    decDesc.FrameDesc_t.FrameHeight     = tga_source_height;

    Decoder decoder(decDesc);
    decoder.init();

    
    Frame frameOut(NULL, decDesc.FrameDesc_t);
    
    int decBSPos                        = 0;
	for (int i = 0; i < frame_count; ++i) 
    {
		std::vector<uint8_t> &output = outputs[i];

        decBSPos                        = decoder.decodeFrame(input, decBSPos, frameOut);

		output.resize(bytes_per_frame);
        int outPos  = 0;
        for (int i = 0; i < decDesc.FrameDesc_t.FrameHeight; i++)
		    for (int j = 0; j < decDesc.FrameDesc_t.FrameWidth; ++j)
                for (int ci = 0; ci < COLOR_PLANE_BGR_NUM_PLANES; ci++)
			        output[outPos++] = frameOut.getPel(i, j, ci);
	}
}

int main(int, char const**) {
	#ifdef WIN32
    static std::string const base_path   = "..//data//";
#else
	static std::string const base_path   = "./data/";
#endif
	static std::string const input_name  = "Cam1_animation_Color_01";
	static std::string const output_name = "result";

	std::vector<uint8_t> inputs [frame_count];
	std::vector<uint8_t> outputs[frame_count];
	std::vector<uint8_t> encoded_data;

	for (int i = 0; i < frame_count; ++i) {
		int j = i + frame_offset;
		std::ostringstream path;
		path << base_path << "/" << input_name << std::setfill('0') << std::setw(4) << j << ".tga";
		if (!readTGA(path.str(), inputs[i]))
			return -1;
	}

    bytes_per_frame = tga_source_width * tga_source_height * tga_source_channels;

	encode(inputs, encoded_data);

	std::cout << "encoded size: " << encoded_data.size()/1024 << "kB" << " ("
		<< std::setprecision(1) << std::fixed
		<< (100.0 * encoded_data.size() / (frame_count * bytes_per_frame))
		<< "% of original size)" << std::endl;

	decode(encoded_data, outputs);

	double max_error = 0;
	double squared_error = 0;
	for (int i = 0; i < frame_count; ++i) {
		for (int j = 0; j < bytes_per_frame; ++j) {
			double error = abs(double(outputs[i][j]) - inputs[i][j]);
			if (error > max_error)
				max_error = error;
			squared_error += error * error;
		}
	}

	std::cout << "RMS error: " << std::setprecision(4) << sqrt(squared_error / (frame_count * bytes_per_frame)) << std::endl;
	std::cout << "max error: " << max_error << std::endl;
	
	for (int i = 0; i < frame_count; ++i) {
		int j = i + frame_offset;
		std::ostringstream path;
		path << base_path << "/" << output_name << std::setfill('0') << std::setw(4) << j << ".tga";
		if (!writeTGA(path.str(), outputs[i]))
			return -1;
	}

	return 0;
}