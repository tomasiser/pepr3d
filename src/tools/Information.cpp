#include "tools/Information.h"
#include <random>
#include "ui/MainApplication.h"

namespace pepr3d {
void Information::drawToSidePane(SidePane& sidePane) {
    sidePane.drawText(
        "Pepr3D is a student project made by a group of Charles University students during 2018 and 2019. It is "
        "designed to be used in the 3D printing pipeline as a way to separate different materials into separate "
        "files.");

    sidePane.drawText("This software was made to complete the course NPRG023 - Software project.");

    sidePane.drawText("You can see and contribute to the code at https://github.com/tomasiser/pepr3d");
}
}  // namespace pepr3d