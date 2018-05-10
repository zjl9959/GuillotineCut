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
    static constexpr double Scale = 1; // set to 0.2 to get 100% zoom on modern screen.
    static constexpr double CanvasWidth = 6000 * Scale;
    static constexpr double CanvasHeight = 3210 * Scale;
    static constexpr int FontSize = static_cast<int>(60 * Scale);


    void begin(std::string path, double plateWidth, double plateHeight, double plateNum, double plateGap) {
        wx = CanvasWidth / plateWidth;
        hx = CanvasHeight / plateHeight;
        int width = static_cast<int>(wx * plateWidth + 1);
        int height = static_cast<int>(hx * (plateHeight * plateNum + plateGap * (plateNum - 1)) + 1);
        ofs.open(path);
        ofs << "<!DOCTYPE html>" << std::endl
            << "<html>" << std::endl
            << "  <head>" << std::endl
            << "    <meta charset='utf-8'>" << std::endl
            << "    <title>2D Packing/Cutting Visualization</title>" << std::endl
            << "  </head>" << std::endl
            << "  <body>" << std::endl // style='text-align:center;'
            << "    <svg width='" << width << "' height='" << height << "' viewBox='0 0 " << width << " " << height << "'>" << std::endl;
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
            << "      <text x='" << x + w / 2 << "' y='" << y + h / 2 << "' text-anchor='middle' alignment-baseline='middle' style='fill:#" << fcolor << ";font-size:" << FontSize << "'>" << label << "</text>" << std::endl << std::endl;
    }
    void rect(double x, double y, double w, double h, bool d, const std::string &label) {
        rc.next();
        rect(x, y, w, h, d, label, rc.fcolor, rc.bcolor);
    }
    void rect(double x, double y, double w, double h) {
        x *= wx; y *= hx; w *= wx; h *= hx;
        ofs << "      <rect x='" << x << "' y='" << y << "' width='" << w << "' height='" << h << "' style='fill:#FFFFFF; stroke:black; stroke-width:12'/>" << std::endl << std::endl;
    }

    void line(double x1, double y1, double x2, double y2, int layer) {
        static int cutWidth[] = { 10, 8, 8, 6 };
        static std::string cutColor[] = { "cyan", "red", "blue", "orange" };
        x1 *= wx; y1 *= hx; x2 *= wx; y2 *= hx;
        ofs << "      <line x1='" << x1 << "' y1='" << y1 << "' x2='" << x2 << "' y2='" << y2 << "' stroke-dasharray='12, 4' stroke='" << cutColor[layer] << "' stroke-width='" << cutWidth[layer] << "'/>" << std::endl << std::endl;
    }

    void circle(double x, double y, double r) {
        ofs << "      <circle cx='" << x << "' cy='" << y << "' r='" << r << "' style='fill-opacity:0; stroke:#000000; stroke-width:2'/>" << std::endl;
    }


    double wx; // width expansion.
    double hx; // height expansion.
    std::ofstream ofs;
    RandColor rc;
};


}


#endif // SMART_SZX_GUILLOTINE_CUT_VISUALIZER_H
