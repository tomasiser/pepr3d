#include "tools/Information.h"
#include <random>
#include "ui/MainApplication.h"

namespace pepr3d {
void Information::drawToSidePane(SidePane& sidePane) {
    sidePane.drawText(
        "Pepr3D is a student project made by a group of Charles University students during the years 2018 and 2019.");

    sidePane.drawText(
        "It is designed to be used in a 3D printing pipeline as a way to separate different materials into separate "
        "files.");

    sidePane.drawText("This software was made to complete the course NPRG023 - Software Project.");

    sidePane.drawText("See and contribute to the code:\nhttps://github.com/tomasiser/pepr3d");
}
}  // namespace pepr3d