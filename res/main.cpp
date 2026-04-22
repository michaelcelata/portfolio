#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <sstream>
#include <vector>
#include <iterator>
#include "lib/stb_image.h"
#include "lib/stb_image_write.h"
#include "lib/qrcodegen.hpp"
#include <regex>

std::string tostr(float f){
	std::string str = std::to_string(f);
	str.erase ( str.find_last_not_of('0') + 1, std::string::npos );
	return str;
}

std::string tostr(int f) {
	return std::to_string(f);
}

void printQr(const qrcodegen::QrCode& qr) {
	int border = 4;
	for (int y = -border; y < qr.getSize() + border; y++) {
		for (int x = -border; x < qr.getSize() + border; x++) {
			std::cout << (qr.getModule(x, y) ? "▉▉" : "  ");
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

std::pair<std::string, std::string> sep_dash(const std::string& input) {
    size_t char_pos = input.find('-');

    if (char_pos != std::string::npos) {
        std::string part1 = input.substr(0, char_pos);

        std::string part2 = input.substr(char_pos + 1);

        return {part1, part2};
    } else {

        return {input, "Error"};
    }
}

std::string genQR_NC(std::string name, std::string text) {
	
	qrcodegen::QrCode q = qrcodegen::QrCode::encodeText(text.c_str(), qrcodegen::QrCode::Ecc::HIGH);
	//printQr(q);
	int length = q.getSize();

	// gcode
	std::string gcode = "";

	// parameters
	int spindle_speed = 5000;
	float retract = 0.1f;
	float padding = 0.000f; // The spacing between each spot (negative for bigger than fit)

	float total_size = 2.0f; // total size in inches

	// Offset the QR Code from the center of QR code
	float offsetX = 0.0f;
	float offsetY = 0.0f;
	
	float plungeFeedHeight = 0.015f;
	float plungeFeedrate = 20.0f;

    gcode += "(" + name + ")" + "\n";

	// Safety line
    gcode += "G90 G80 G17 G40\n";
    gcode += "G00 G53 Z0\n";
    gcode += "G00 X0Y0\n";
    gcode += "G00 G54\n";
    gcode += "T1 M6\n";
    gcode += "G43 H1\n";
    gcode += "G00 X0Y0\n";
    gcode += "S" + tostr(spindle_speed) + " M03\n";
    gcode += "G00 Z" + tostr(retract) + "\n";
    gcode += "M08\n";
	gcode += "\n(Loop starts here)\n";

	// go through qr
	float dist = total_size/(float)length;

	float referenceDiameter = dist;

	float depth = -(referenceDiameter/2.0f);

	std::cout << "Ref Dia: " << referenceDiameter << " Length: " << total_size << " Lanes: " << length << "\n";

	for (int y = 0; y < length; y++) {
    	for (int x = 0; x < length; x++) {
			bool is = q.getModule(x,y);
			if(is) {
				// place
				float placeX = (dist*(float)x+(dist/2.0f))-(total_size/2.0f) +offsetX;
				float placeY = -((dist*(float)y+(dist/2.0f))-(total_size/2.0f) + offsetY);


				// position XY
				gcode += "G00 X" + tostr(placeX) + " Y" + tostr(placeY) + "\n";
				// plunge
				gcode += "G00 Z" + tostr(plungeFeedHeight) + "\n";
				gcode += "G01 Z" + tostr(depth) + " F" + tostr(plungeFeedrate) + "\n";

				gcode += "G00 Z" + tostr(retract) + "\n";
			}
    	}
	}
	
    // Finish program
    gcode = gcode + "G00 Z" + tostr(retract) + "\n";
    gcode = gcode + "M09\n";
    gcode = gcode + "G00 G53 Z0.\n";
    gcode = gcode + "M05\n";
    gcode = gcode + "M30\n";

	return gcode;
}

int main(){


	std::cout << "Processing text file\n";

    std::ifstream file("res/qr.txt");
	std::string str;

	while (std::getline(file, str))
    {
        // Process string
		std::pair<std::string, std::string> v = sep_dash(str);

		std::string name = v.first;
		std::string url = v.second;

		// remove the trailing spaces to prevent issues
		name = std::regex_replace(name, std::regex("^ +| +$|( ) +"), "$1");
		url = std::regex_replace(url, std::regex("^ +| +$|( ) +"), "$1");

		std::cout << "Decoding " << "[" << name << "]" << " with url of " << "[" << url << "]" << "\n"; 

		// write gcode
		
		std::string filename = "code/" +name + ".nc";
		std::ofstream out;
		out.open(filename.c_str());
		out << genQR_NC(name, url);
		out.close();

    }


	return 0;
}