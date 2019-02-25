#pragma once
#ifndef NDEBUG
#include <cinder/Filesystem.h>
#include <fstream>

template <typename PointType>
class GnuplotDebug {
   public:
    GnuplotDebug() = default;
    GnuplotDebug(const GnuplotDebug&) = delete;

    enum GraphType { LINES, LINESPOINTS, POINTS };

    /// Add polygon to view
    ///@param rgbStr Color in gnuplot format: #CCCCCC
    ///@param type Type of graph to use
    template <typename Polygon>
    void addPoly(const Polygon& poly, const std::string& rgbStr, GraphType type = GraphType::LINES) {
        std::vector<PointType> points(poly.vertices_begin(), poly.vertices_end());
        mPolysToDraw.push_back(std::move(points));
        mRgbStrings.push_back(rgbStr);
        mTypes.push_back(type);
    }

    void exportToFile() {
        std::ofstream oFileGnuplot("debugOut.gnuplot");
        if(oFileGnuplot.bad() || mPolysToDraw.empty())
            return;

        oFileGnuplot << "set size ratio -1 \n";

        for(size_t idx = 0; idx < mRgbStrings.size(); ++idx) {
            oFileGnuplot << "set style line " << idx + 1 << " linecolor rgb \"" << mRgbStrings[idx]
                         << "\" linetype 1 linewidth 2\n";
        }

        const cinder::fs::path filePath("debugOut.data");

        oFileGnuplot << "plot '" << absolute(filePath) << "' index 0 with " << typeToString(mTypes[0])
                     << " linestyle 1 title \"\"";

        for(size_t idx = 1; idx < mPolysToDraw.size(); ++idx) {
            oFileGnuplot << ", '" << absolute(filePath) << "' index " << idx << " with " << typeToString(mTypes[idx])
                         << " linestyle " << idx + 1;
        }

        oFileGnuplot << "\n";

        std::ofstream oFileData(absolute(filePath));
        if(oFileData.bad())
            return;

        for(size_t idx = 0; idx < mPolysToDraw.size(); ++idx) {
            const auto& poly = mPolysToDraw[idx];
            if(poly.empty()) {
                continue;
            }

            oFileData << "# X Y\n";
            for(const PointType& point : poly) {
                oFileData << CGAL::to_double(point.x()) << " " << CGAL::to_double(point.y()) << "\n";
            }

            if(mTypes[idx] != POINTS) {
                // add first point again to connect the poly
                oFileData << CGAL::to_double(poly[0].x()) << " " << CGAL::to_double(poly[0].y()) << "\n";
            }

            oFileData << "\n";
            oFileData << "\n";
        }
    }

   private:
    std::vector<std::vector<PointType>> mPolysToDraw;
    std::vector<std::string> mRgbStrings;
    std::vector<GraphType> mTypes;

    std::string typeToString(GraphType type) {
        switch(type) {
        case LINES: return "lines";
        case LINESPOINTS: return "linespoints";
        case POINTS: return "points";
        default: return "unknown_graph_type";
        }
    }
};

#endif