////////////////////////////////
/// usage : 1.	
/// 
/// note  : 1.	
////////////////////////////////

#ifndef SMART_SZX_GUILLOTINE_CUT_VISUALIZER_H
#define SMART_SZX_GUILLOTINE_CUT_VISUALIZER_H


#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>

#include "Utility.h"


namespace szx {

struct RandColor {
    static constexpr auto ColorCodeChar = "0123456789ABCDEF";
    static constexpr int ColorCodeBase = 16;
    static constexpr int ColorCodeLen = 6;

    void next() {
        for (int i = 0; i < ColorCodeLen; ++i) {
            int c = r.pick(ColorCodeBase);
            bcolor[i] = ColorCodeChar[c];
            fcolor[i] = ColorCodeChar[(c >(ColorCodeBase / 2)) ? 0 : (ColorCodeBase - 1)]; // (c + ColorCodeBase / 2) % ColorCodeBase
        }
    }

    char fcolor[ColorCodeLen + 1] = { 0 }; // front color.
    char bcolor[ColorCodeLen + 1] = { 0 }; // background color.
    Random r;
};

struct Drawer {
    static constexpr double W = 1200;
    static constexpr double H = 642;


    Drawer(std::string path, double width, double height) : ofs(path), wx(W / width), hx(H / height) {}


    void begin() {
        ofs << "<!DOCTYPE html>" << std::endl
            << "<html>" << std::endl
            << "  <head>" << std::endl
            << "    <meta charset='utf-8'>" << std::endl
            << "    <title>2D Packing/Cutting Visualization</title>" << std::endl
            << "  </head>" << std::endl
            << "  <body>" << std::endl // style='text-align:center;'
            << "    <svg width='" << W << "' height='" << H << "' viewBox='0 0 " << W << " " << H << "'>" << std::endl;
    }
    void end() {
        ofs << "    </svg>" << std::endl
            << "  </body>" << std::endl
            << "</html>" << std::endl;
        ofs.close();
    }

    void rect(double x, double y, double w, double h, bool d, const std::string &label, const std::string &fcolor, const std::string &bcolor) {
        if (d) { std::swap(w, h); }
        x *= wx; y *= hx; w *= wx; h *= hx;
        ofs << "      <rect x='" << x << "' y='" << y << "' width='" << w << "' height='" << h << "' style='fill:#" << bcolor << "; stroke:black; stroke-width:2'/>" << std::endl
            << "      <text x='" << x + w / 2 << "' y='" << y + h / 2 << "' text-anchor='middle' alignment-baseline='middle' style='fill:#" << fcolor << "'>" << label << "</text>" << std::endl << std::endl;
    }
    void rect(double x, double y, double w, double h, bool d, const std::string &label) {
        rc.next();
        rect(x, y, w, h, d, label, rc.fcolor, rc.bcolor);
    }

    void line(double x1, double y1, double x2, double y2, int layer) {
        static int cutWidth[] = { 16, 8, 8, 6 };
        static std::string cutColor[] = { "cyan", "red", "blue", "orange" };
        x1 *= wx; y1 *= hx; x2 *= wx; y2 *= hx;
        ofs << "      <line x1='" << x1 << "' y1='" << y1 << "' x2='" << x2 << "' y2='" << y2 << "' stroke-dasharray='12, 4' stroke='" << cutColor[layer] << "' stroke-width='" << cutWidth[layer] << "'/>" << std::endl << std::endl;
    }


    double wx;
    double hx;
    std::ofstream ofs;
    RandColor rc;
};


}


#endif // SMART_SZX_GUILLOTINE_CUT_VISUALIZER_H