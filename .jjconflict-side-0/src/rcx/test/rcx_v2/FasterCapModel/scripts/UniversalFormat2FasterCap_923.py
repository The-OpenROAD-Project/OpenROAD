# ///////////////////////////////////////////////////////////////////////////////
# // BSD 3-Clause License
# //
# // Copyright (c) 2024, Nikos Zazakos, U. of Thessaly
# // All rights reserved.
# //
# // Redistribution and use in source and binary forms, with or without
# // modification, are permitted provided that the following conditions are met:
# //
# // * Redistributions of source code must retain the above copyright notice, this
# //   list of conditions and the following disclaimer.
# //
# // * Redistributions in binary form must reproduce the above copyright notice,
# //   this list of conditions and the following disclaimer in the documentation
# //   and/or other materials provided with the distribution.
# //
# // * Neither the name of the copyright holder nor the names of its
# //   contributors may be used to endorse or promote products derived from
# //   this software without specific prior written permission.
# //
# // THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# // AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# // IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# // ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
# // LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# // CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# // SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# // INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# // CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# // ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# // POSSIBILITY OF SUCH DAMAGE.


#!/usr/bin/python
import os
import sys
import datetime
from operator import attrgetter
import operator


# Structures used while parsing processfile #
class Dielectrics:
    name = ""
    thickness = 0.0
    epsilon = 0.0

    def __init__(self, name: str, thickness: float, epsilon: float) -> None:
        self.name = name
        self.thickness = thickness
        self.epsilon = epsilon

    def print_contents(self) -> None:
        print(
            "Name: "
            + self.name
            + " Thickness: "
            + self.thickness
            + " Epsilon: "
            + self.epsilon
        )


class Metals:
    name = ""
    distance = 0.0
    thickness = 0.0
    minwidth = 0.0
    minspacing = 0.0
    resistivity = 0.0

    def __init__(
        self,
        name: str,
        distance: float,
        thickness: float,
        minwidth: float,
        minspacing: float,
        resistivity: float,
    ) -> None:
        self.name = name
        self.distance = distance
        self.thickness = thickness
        self.minwidth = minwidth
        self.minspacing = minspacing
        self.resistivity = resistivity

    def print_contents(self) -> None:
        print(
            "Name: "
            + self.name
            + " Thickness: "
            + self.thickness
            + " distance: "
            + self.distance
        )


# Structures used while parsing UniversalFormat #
class UniversalFormatDielectrics:
    # stored while parsing DIELECTRIC line #
    name = ""
    thickness = 0.0
    z = 0.0
    epsilon = 0.0

    def __init__(self, name: str, z: float, thickness: float, epsilon: float):
        self.name = name
        self.z = z
        self.thickness = thickness
        self.epsilon = epsilon

    def set_coordinates(self, x, y, width, length):
        self.x = x
        self.y = y
        self.width = widht
        self.length = length


class UniversalFormatGroundPlanes:
    # stored while parsing GROUND_PLANE line #
    name = ""
    thickness = 0.0
    z = 0.0
    metallevel = 0

    def __init__(self, name: str, z: float, thickness: float, metallevel: int):
        self.name = name
        self.z = z
        self.thickness = thickness
        self.metallevel = metallevel

    def set_coordinates(self, x, y, width, length):
        self.x = x
        self.y = y
        self.width = widht
        self.length = length


class UniversalFormatWires:
    name = ""
    num = 0
    x = 0.0
    y = 0.0
    z = 0.0
    width = 0.0
    thickness = 0.0
    length = 0.0
    voltage = 0
    groundplane = False

    def __init__(
        self,
        name: str,
        num: int,
        x: float,
        y: float,
        z: float,
        width: float,
        thickness: float,
        length: float,
        voltage: int,
    ):
        self.name = name
        self.num = num
        self.x = x
        self.y = y
        self.z = z
        self.width = width
        self.thickness = thickness
        self.length = length
        self.voltage = voltage


# Structures used after parsing UniversalFormat #
class Shapes:

    name = ""
    shapeorder = 0
    shapeheight = 0
    shapewidth = 0
    shapethickness = 0
    shapetype = 0
    shapex = 0.0
    shapey = 0.0
    shapez = 0.0

    def __init__(
        self,
        name: str,
        order: float,
        height: float,
        thickness: float,
        width: float,
        xcoordinate: float,
        ycoordinate: float,
        zcoordinate: float,
        shapetype: int,
    ) -> None:
        self.name = name
        self.shapeorder = order
        self.shapeheight = height
        self.shapewidth = width
        self.shapethickness = thickness
        self.shapex = xcoordinate
        self.shapey = ycoordinate
        self.shapez = zcoordinate
        self.shapetype = shapetype

    def print_contents(self) -> None:
        print(
            "Name: "
            + self.name
            + " order: "
            + str(self.shapeorder)
            + " height: "
            + str(self.shapeheight)
            + " width: "
            + str(self.shapewidth)
            + " thickness: "
            + str(self.shapethickness)
            + " xcoord: "
            + str(self.shapex)
            + " ycoord: "
            + str(self.shapey)
            + " type: "
            + str(self.shapetype)
        )


# TODO: Handle comment lines #
def processTechFile(filename: str):

    f = open(filename)

    conductorblock = 0
    dielectricblock = 0

    for line in f:
        line = line.split("#", 1)[0]  # split on '#' only once #
        # print(line)
        if "CONDUCTOR" in line:
            linesplits = line.split()
            conductorblock = 1
            conductorname = linesplits[1]
            conductordistance = 0.0
            conductorthickness = 0.0
            conductorminwidth = 0.0
            conductorminspacing = 0.0
            conductorresistivity = 0.0

        elif "DIELECTRIC" in line:
            linesplits = line.split()
            dielectricblock = 1
            dielectricname = linesplits[1]
            dielectricthickness = 0.0
            dielectricepsilon = 0.0

        elif conductorblock == 1:
            linesplits = line.split()

            if "distance" in line:
                conductordistance = linesplits[1]
            elif "thickness" in line:
                conductorthickness = linesplits[1]
            elif "min_width" in line:
                conductorminwidth = linesplits[1]
            elif "min_spacing" in line:
                conductorminspacing = linesplits[1]
            elif "resistivity" in line:
                conductorresistivity = linesplits[1]
            elif "}" in line:
                conductorblock = 0
                processMetals.append(
                    Metals(
                        conductorname,
                        conductordistance,
                        conductorthickness,
                        conductorminwidth,
                        conductorminspacing,
                        conductorresistivity,
                    )
                )

        elif dielectricblock == 1:
            linesplits = line.split()

            if "epsilon" in line:
                dielectricepsilon = linesplits[1]
            elif "thickness" in line:
                dielectricthickness = linesplits[1]
            elif "}" in line:
                dielectricblock = 0
                processDielectrics.append(
                    Dielectrics(dielectricname, dielectricthickness, dielectricepsilon)
                )


def TranslateUniversalFile(Universalfilename: str, FasterCapfilename: str):

    global patternswindowwidth
    global patternswindowthickness
    global patternswindowheight
    global window_min_x
    global window_min_z
    global window_min_y
    global window_max_x
    global window_max_z
    global window_max_y
    global base_epsilon

    # check if specified directory paths end with '/' #
    # otherwise append '/' at the end                 #
    if not FasterCapfilename.endswith("/"):
        tempfastercapfilename = FasterCapfilename + "/"
    else:
        tempfastercapfilename = FasterCapfilename

    if not Universalfilename.endswith("/"):
        tempuniversalfilename = Universalfilename + "/"
    else:
        tempuniversalfilename = Universalfilename

    # walk each directory tree for specified Universalfilename #
    for root, dirs, files in os.walk(tempuniversalfilename):

        for directory in dirs:  # found directory #

            # create directory in Fastercap directory if it doesn't exist #
            fastercap_root = root.replace(
                tempuniversalfilename.split("/")[0], tempfastercapfilename.split("/")[0]
            )
            dirpath = os.path.join(fastercap_root, directory)
            if not os.path.exists(dirpath):
                os.makedirs(dirpath)

        for file in files:  # found file #
            if file == "wires":  # check if file is wires #
                tempwires = []
                tempdielectrics = []
                tempgroundplanes = []
                # get the fastercap pattern file root #
                fastercap_root = root.replace(
                    tempuniversalfilename.split("/")[0],
                    tempfastercapfilename.split("/")[0],
                )
                # get fastercap pattern file name #
                TempFasterCapfilename = os.path.join(fastercap_root, file) + ".lst"
                # if fastercap pattern root doesn't exist create it #
                if not os.path.exists(fastercap_root):
                    os.makedirs(fastercap_root)

                fname = os.path.join(root, file)
                print(
                    f"Translating Universal Pattern {fname} to FasterCap pattern {TempFasterCapfilename}"
                )

                # TOOD: check for errors in files #
                errorcode = 0

                # base epsilon is the value of the first in order dielectric #
                # 4.1 is the default value of p1_1 #
                # first dielectric is used to check if we are parsing the first dielectric instance #
                base_epsilon = 4.1
                length = 0
                first_dielectric = True

                f = open(fname)

                dielorder = 0
                lineindex = 0
                groundplane_num = 0
                lower_z = 0  # M0_w0
                upper_z = 14.930  # air2 + 1 upper height #
                for line in f:
                    linesplit = [
                        x for x in line.split(" ") if x != ""
                    ]  # split lines on space and filter out empty tokens #
                    # check first token #
                    if linesplit[0] == "PATTERN":
                        patternname = linesplit[1]

                    # NOTE: we have 1 or 2 Ground Planes               #
                    # Low Ground Plane Gives us lower height and       #
                    # High Ground Plane sets the upper height bound    #
                    # DIELECTIS and Wires are normalized based on that #
                    elif linesplit[0] == "GROUND_PLANE":
                        z = float(linesplit[4])
                        thickness = float(linesplit[7])
                        if thickness == float(0.0):
                            z += -0.1
                            thickness = 0.1
                        groundplane_num += 1
                        if groundplane_num == 1:  # lower ground #
                            lower_z = float(linesplit[5])
                        elif groundplane_num == 2:  # upper ground #
                            upper_z = z
                        if normalized_heights == True:
                            z -= lower_z
                        tempgroundplanes.append(
                            UniversalFormatGroundPlanes(
                                linesplit[2], z, thickness, int(linesplit[1])
                            )
                        )

                    elif linesplit[0] == "DIELECTRIC":
                        if first_dielectric == True:
                            base_epsilon = float(linesplit[6])
                            first_dielectric = False

                        # check if the heights are normalized                             #
                        # if true then check if current height is between lower and upper #
                        # if true then keep dielectric/metal                              #
                        # else continue to next dielectric/metal                          #
                        z = float(linesplit[3])
                        height_plus_z = float(linesplit[4])
                        if normalized_heights == True:
                            if (z < lower_z) or (height_plus_z > upper_z):
                                continue
                            else:
                                z -= lower_z

                        tempdielectrics.append(
                            UniversalFormatDielectrics(
                                linesplit[1],
                                z,
                                float(linesplit[4]) - float(linesplit[3]),
                                float(linesplit[6]),
                            )
                        )
                    elif linesplit[0] == "WIRE":
                        name = linesplit[2]
                        num = int(linesplit[1])
                        x = float(linesplit[4])
                        y = 0.0

                        # check if the heights are normalized                             #
                        # if true then check if current height is between lower and upper #
                        # if true then keep dielectric/metal                              #
                        # else continue to next dielectric/metal                          #
                        z = float(linesplit[5])
                        height_plus_z = float(linesplit[11])
                        if normalized_heights == True:
                            if (z < lower_z) or (height_plus_z > upper_z):
                                continue
                            else:
                                z -= lower_z

                        width = float(linesplit[7]) - float(linesplit[4])
                        thickness = float(linesplit[11]) - float(linesplit[5])
                        length = float(linesplit[16])
                        voltage = int(linesplit[18])
                        tempwires.append(
                            UniversalFormatWires(
                                name, num, x, y, z, width, thickness, length, voltage
                            )
                        )
                    elif linesplit[0] == "WINDOW_BBOX":
                        length = float(linesplit[8])
                        window_min_x = float(linesplit[2])
                        window_min_y = 0
                        window_min_z = float(linesplit[3])
                        window_max_x = float(linesplit[5])
                        window_max_z = float(linesplit[6])
                        window_max_y = length
                    elif linesplit[0] == "SIM_WIN_EXT" and user_window_ext == False:
                        # extend BBOX on x z and y axis #
                        window_min_x += float(linesplit[2])
                        window_min_z += float(linesplit[3])
                        window_max_x += float(linesplit[5])
                        window_max_z += float(linesplit[6])
                        window_min_y += float(linesplit[8])
                        window_max_y += float(linesplit[9])
                    else:  # unkown first token #
                        continue

                # add user window extension if provided #
                if user_window_ext == True:
                    # extend BBOX on x z and y axis #
                    window_min_x += window_min_x_ext
                    window_min_z += window_min_z_ext
                    window_max_x += window_max_x_ext
                    window_max_z += window_max_z_ext
                    window_min_y += window_min_y_ext
                    window_max_y += window_max_y_ext

                patternswindowwidth = window_max_x - window_min_x
                patternswindowthickness = window_max_z - window_min_z
                patternswindowheight = window_max_y - window_min_y

                # map Universal Format Data Structures to Shapes #
                # NOTE: height == length (y-axis) #
                dielorder = 0
                for dielectric in tempdielectrics:
                    shapetype = 0  # Dielectric
                    shapeorder = dielorder
                    dielorder += 1
                    shapename = dielectric.name
                    shapeheight = round(patternswindowheight, 6)
                    shapethickness = round(dielectric.thickness, 6)
                    shapewidth = round(patternswindowwidth, 6)
                    # NOTE: LL corner y <-> z #
                    LLcornerx = window_min_x
                    LLcornery = dielectric.z
                    LLcornerz = window_min_y
                    PatternShapes.append(
                        Shapes(
                            shapename,
                            shapeorder,
                            shapeheight,
                            shapethickness,
                            shapewidth,
                            LLcornerx,
                            LLcornery,
                            LLcornerz,
                            shapetype,
                        )
                    )

                for groundplane in tempgroundplanes:
                    shapetype = 1  # Metal
                    shapename = groundplane.name
                    shapeorder = shapename.split("_")[0].split("M")[1]
                    shapeheight = round(patternswindowheight, 6)
                    shapethickness = round(groundplane.thickness, 6)
                    shapewidth = round(patternswindowwidth, 6)
                    # NOTE: LL corner y <-> z #
                    LLcornerx = window_min_x
                    LLcornery = groundplane.z
                    LLcornerz = window_min_y
                    PatternShapes.append(
                        Shapes(
                            shapename,
                            shapeorder,
                            shapeheight,
                            shapethickness,
                            shapewidth,
                            LLcornerx,
                            LLcornery,
                            LLcornerz,
                            shapetype,
                        )
                    )

                # NOTE: Wires y & length should be preserved, not extented #
                for wire in tempwires:
                    shapetype = 1  # Metal
                    shapename = wire.name
                    shapeorder = shapename.split("_")[0].split("M")[1]
                    shapeheight = round(wire.length, 6)
                    shapethickness = round(wire.thickness, 6)
                    shapewidth = round(wire.width, 6)
                    # NOTE: LL corner y <-> z #
                    LLcornerx = wire.x
                    LLcornery = wire.z
                    LLcornerz = wire.y
                    PatternShapes.append(
                        Shapes(
                            shapename,
                            shapeorder,
                            shapeheight,
                            shapethickness,
                            shapewidth,
                            LLcornerx,
                            LLcornery,
                            LLcornerz,
                            shapetype,
                        )
                    )

                for i in range(len(PatternShapes)):
                    PatternShapes[i].print_contents()

                # print(base_epsilon)

                extractFasterCapfile(TempFasterCapfilename)
                # clear data global data structure #
                PatternShapes.clear()


def write_dielectric_side_panels(
    filename: str, name: str, width: float, thickness: float, height: float
):
    file = open(filename, "w+")

    # Write File Headers #
    file.write("* {}x{}x{}um box\n".format(str(width), str(thickness), str(height)))
    file.write("* Layer of Box Dielectric {}\n".format(str(name)))
    file.write("* face name	| four coordinates of one face\n\n")

    # Write dielectric fases #
    p000 = [0.0, 0.0, 0.0]
    p001 = [0.0, 0.0, height]
    p010 = [0.0, thickness, 0.0]
    p011 = [0.0, thickness, height]

    p100 = [width, 0.0, 0.0]
    p101 = [width, 0.0, height]
    p110 = [width, thickness, 0.0]
    p111 = [width, thickness, height]

    file.write(
        "Q dielectric_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p000[0],
            p000[1],
            p000[2],
            p010[0],
            p010[1],
            p010[2],
            p110[0],
            p110[1],
            p110[2],
            p100[0],
            p100[1],
            p100[2],
        )
    )  # front face #
    file.write(
        "Q dielectric_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p000[0],
            p000[1],
            p000[2],
            p001[0],
            p001[1],
            p001[2],
            p011[0],
            p011[1],
            p011[2],
            p010[0],
            p010[1],
            p010[2],
        )
    )  # left fase #
    file.write(
        "Q dielectric_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p100[0],
            p100[1],
            p100[2],
            p101[0],
            p101[1],
            p101[2],
            p111[0],
            p111[1],
            p111[2],
            p110[0],
            p110[1],
            p110[2],
        )
    )  # right fase #
    file.write(
        "Q dielectric_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p001[0],
            p001[1],
            p001[2],
            p011[0],
            p011[1],
            p011[2],
            p111[0],
            p111[1],
            p111[2],
            p101[0],
            p101[1],
            p101[2],
        )
    )  # back face #

    file.close()


def write_dielectric_bottom_panel(
    filename: str, name: str, width: float, thickness: float, height: float
):
    file = open(filename, "w+")

    # Write File Headers #
    file.write("* {}x{}x{}um box\n".format(str(width), str(thickness), str(height)))
    file.write("* Layer of Box Dielectric {}\n".format(str(name)))
    file.write("* face name	| four coordinates of one face\n\n")

    # Write dielectric fases #
    p000 = [0.0, 0.0, 0.0]
    p001 = [0.0, 0.0, height]
    p010 = [0.0, thickness, 0.0]
    p011 = [0.0, thickness, height]

    p100 = [width, 0.0, 0.0]
    p101 = [width, 0.0, height]
    p110 = [width, thickness, 0.0]
    p111 = [width, thickness, height]

    file.write(
        "Q dielectric_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p000[0],
            p000[1],
            p000[2],
            p100[0],
            p100[1],
            p100[2],
            p101[0],
            p101[1],
            p101[2],
            p001[0],
            p001[1],
            p001[2],
        )
    )  # down face #

    file.close()


def write_dielectric_top_panel(
    filename: str, name: str, width: float, thickness: float, height: float
):
    file = open(filename, "w+")

    # Write File Headers #
    file.write("* {}x{}x{}um box\n".format(str(width), str(thickness), str(height)))
    file.write("* Layer of Box Dielectric {}\n".format(str(name)))
    file.write("* face name	| four coordinates of one face\n\n")

    # Write dielectric fases #
    p000 = [0.0, 0.0, 0.0]
    p001 = [0.0, 0.0, height]
    p010 = [0.0, thickness, 0.0]
    p011 = [0.0, thickness, height]

    p100 = [width, 0.0, 0.0]
    p101 = [width, 0.0, height]
    p110 = [width, thickness, 0.0]
    p111 = [width, thickness, height]

    file.write(
        "Q dielectric_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p010[0],
            p010[1],
            p010[2],
            p110[0],
            p110[1],
            p110[2],
            p111[0],
            p111[1],
            p111[2],
            p011[0],
            p011[1],
            p011[2],
        )
    )  # upper face #

    file.close()


def write_wire_side_panels(
    filename: str, name: str, width: float, thickness: float, height: float
):
    file = open(filename, "w+")

    # Write File Headers #
    file.write("* {}x{}x{}um box\n".format(str(width), str(thickness), str(height)))
    file.write("* Layer of Box wire {}\n".format(str(name)))
    file.write("* face name	| four coordinates of one face\n\n")

    # Write wire fases #
    p000 = [0.0, 0.0, 0.0]
    p001 = [0.0, 0.0, height]
    p010 = [0.0, thickness, 0.0]
    p011 = [0.0, thickness, height]

    p100 = [width, 0.0, 0.0]
    p101 = [width, 0.0, height]
    p110 = [width, thickness, 0.0]
    p111 = [width, thickness, height]

    file.write(
        "Q wire_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p000[0],
            p000[1],
            p000[2],
            p010[0],
            p010[1],
            p010[2],
            p110[0],
            p110[1],
            p110[2],
            p100[0],
            p100[1],
            p100[2],
        )
    )  # front face #
    file.write(
        "Q wire_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p000[0],
            p000[1],
            p000[2],
            p001[0],
            p001[1],
            p001[2],
            p011[0],
            p011[1],
            p011[2],
            p010[0],
            p010[1],
            p010[2],
        )
    )  # left fase #
    file.write(
        "Q wire_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p100[0],
            p100[1],
            p100[2],
            p101[0],
            p101[1],
            p101[2],
            p111[0],
            p111[1],
            p111[2],
            p110[0],
            p110[1],
            p110[2],
        )
    )  # right fase #
    file.write(
        "Q wire_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p001[0],
            p001[1],
            p001[2],
            p011[0],
            p011[1],
            p011[2],
            p111[0],
            p111[1],
            p111[2],
            p101[0],
            p101[1],
            p101[2],
        )
    )  # back face #

    file.close()


def write_wire_bottom_panel(
    filename: str, name: str, width: float, thickness: float, height: float
):
    file = open(filename, "w+")

    # Write File Headers #
    file.write("* {}x{}x{}um box\n".format(str(width), str(thickness), str(height)))
    file.write("* Layer of Box wire {}\n".format(str(name)))
    file.write("* face name	| four coordinates of one face\n\n")

    # Write wire fases #
    p000 = [0.0, 0.0, 0.0]
    p001 = [0.0, 0.0, height]
    p010 = [0.0, thickness, 0.0]
    p011 = [0.0, thickness, height]

    p100 = [width, 0.0, 0.0]
    p101 = [width, 0.0, height]
    p110 = [width, thickness, 0.0]
    p111 = [width, thickness, height]

    file.write(
        "Q wire_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p000[0],
            p000[1],
            p000[2],
            p100[0],
            p100[1],
            p100[2],
            p101[0],
            p101[1],
            p101[2],
            p001[0],
            p001[1],
            p001[2],
        )
    )  # down face #

    file.close()


def write_wire_top_panel(
    filename: str, name: str, width: float, thickness: float, height: float
):
    file = open(filename, "w+")

    # Write File Headers #
    file.write("* {}x{}x{}um box\n".format(str(width), str(thickness), str(height)))
    file.write("* Layer of Box wire {}\n".format(str(name)))
    file.write("* face name	| four coordinates of one face\n\n")

    # Write wire fases #
    p000 = [0.0, 0.0, 0.0]
    p001 = [0.0, 0.0, height]
    p010 = [0.0, thickness, 0.0]
    p011 = [0.0, thickness, height]

    p100 = [width, 0.0, 0.0]
    p101 = [width, 0.0, height]
    p110 = [width, thickness, 0.0]
    p111 = [width, thickness, height]

    file.write(
        "Q wire_{}  {} {} {}     {} {} {}     {} {} {}     {} {} {}\n".format(
            name,
            p010[0],
            p010[1],
            p010[2],
            p110[0],
            p110[1],
            p110[2],
            p111[0],
            p111[1],
            p111[2],
            p011[0],
            p011[1],
            p011[2],
        )
    )  # upper face #

    file.close()


def insert_dielectric_into_pattern_file(
    fastercapfile,
    dielectricfilename,
    outperdiel,
    inperdiel,
    offx,
    offy,
    offz,
    refx,
    refy,
    refz,
):

    fastercapfile.write(
        "D ../../../../../../{}   {} {}   {} {} {}    {} {} {} -  \n".format(
            dielectricfilename,
            outperdiel,
            inperdiel,
            offx,
            offy,
            offz,
            refx,
            refy,
            refz,
        )
    )


def insert_conductor_into_pattern_file(
    fastercapfile, conductorfilename, diel, refx, refy, refz
):
    fastercapfile.write(
        "C ../../../../../../{}   {}    {} {} {} ".format(
            conductorfilename, diel, refx, refy, refz
        )
    )


def process_intersection():
    pass


def extractFasterCapfile(filename: str):

    # for i in range(len(PatternShapes)):
    #   PatternShapes[i].print_contents()

    if not os.path.exists("Dielectrics/"):
        os.mkdir("Dielectrics/")

    if not os.path.exists("Wires/"):
        os.mkdir("Wires/")

    # create the Dielectrics Stack as no other conductor exist //

    # PatternShapes.sort(key = operator.attrgetter('shapey'))

    index = 0
    FasterCapFile = open(filename, "w+")

    conductorindexlist = []
    conductorsintersections = []

    dielindexlist = []
    dielintersections = []

    for shape in PatternShapes:

        if shape.shapetype == 0:
            condintersect = []

            for j in range(len(PatternShapes)):

                if PatternShapes[j].shapetype == 1:
                    if (
                        shape.shapey
                        >= PatternShapes[j].shapey + PatternShapes[j].shapethickness
                    ):
                        continue
                    elif shape.shapey == PatternShapes[j].shapey:
                        condintersect.append(j)
                    elif shape.shapey < PatternShapes[j].shapey:
                        continue
                    elif (
                        shape.shapey
                        < PatternShapes[j].shapey + PatternShapes[j].shapethickness
                    ):
                        condintersect.append(j)
                    else:
                        break
                else:
                    # print(shape.name)
                    continue

            dielindexlist.append(index)
            dielintersections.append(condintersect)

        else:

            dielintersect = []
            for j in range(len(PatternShapes)):

                if PatternShapes[j].shapetype == 0:
                    if shape.shapey > PatternShapes[j].shapey:
                        continue
                    elif shape.shapey == PatternShapes[j].shapey:
                        dielintersect.append(j)
                    elif shape.shapey + shape.shapethickness > PatternShapes[j].shapey:
                        dielintersect.append(j)
                    else:
                        break
                else:
                    # print(shape.name)
                    continue

            conductorindexlist.append(index)
            conductorsintersections.append(dielintersect)

        index += 1

    conductorindex = 0
    groundconductorexists = 0
    minimumconductorlayer = 10
    maximumconductorlayer = 0
    for intersectionslist in conductorsintersections:

        conductororder = int(
            PatternShapes[conductorindexlist[conductorindex]]
            .name.split("_")[0]
            .split("M")[1]
        )

        if conductororder < minimumconductorlayer:
            minimumconductorlayer = conductororder

        if conductororder > maximumconductorlayer:
            maximumconductorlayer = conductororder

        if len(intersectionslist) == 0:
            groundconductorexists = 1
            conductorfile = (
                "Wires/wire_"
                + PatternShapes[conductorindexlist[conductorindex]].name
                + "_W"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapewidth)
                + "_T"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapethickness)
                + "_H"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapeheight)
                + "-bottom.txt"
            )
            write_wire_bottom_panel(
                conductorfile,
                PatternShapes[conductorindexlist[conductorindex]].name,
                PatternShapes[conductorindexlist[conductorindex]].shapewidth,
                PatternShapes[conductorindexlist[conductorindex]].shapethickness,
                PatternShapes[conductorindexlist[conductorindex]].shapeheight,
            )
            insert_conductor_into_pattern_file(
                FasterCapFile,
                conductorfile,
                float(1.0e-6),
                PatternShapes[conductorindexlist[conductorindex]].shapex,
                PatternShapes[conductorindexlist[conductorindex]].shapey,
                PatternShapes[conductorindexlist[conductorindex]].shapez,
            )
            FasterCapFile.write(" + \n")

            conductorfile = (
                "Wires/wire_"
                + PatternShapes[conductorindexlist[conductorindex]].name
                + "_W"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapewidth)
                + "_T"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapethickness)
                + "_H"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapeheight)
                + "-sides.txt"
            )
            write_wire_side_panels(
                conductorfile,
                PatternShapes[conductorindexlist[conductorindex]].name,
                PatternShapes[conductorindexlist[conductorindex]].shapewidth,
                PatternShapes[conductorindexlist[conductorindex]].shapethickness,
                PatternShapes[conductorindexlist[conductorindex]].shapeheight,
            )
            insert_conductor_into_pattern_file(
                FasterCapFile,
                conductorfile,
                float(1.0e-6),
                PatternShapes[conductorindexlist[conductorindex]].shapex,
                PatternShapes[conductorindexlist[conductorindex]].shapey,
                PatternShapes[conductorindexlist[conductorindex]].shapez,
            )
            FasterCapFile.write(" + \n")

            conductorfile = (
                "Wires/wire_"
                + PatternShapes[conductorindexlist[conductorindex]].name
                + "_W"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapewidth)
                + "_T"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapethickness)
                + "_H"
                + str(PatternShapes[conductorindexlist[conductorindex]].shapeheight)
                + "-top.txt"
            )
            write_wire_top_panel(
                conductorfile,
                PatternShapes[conductorindexlist[conductorindex]].name,
                PatternShapes[conductorindexlist[conductorindex]].shapewidth,
                PatternShapes[conductorindexlist[conductorindex]].shapethickness,
                PatternShapes[conductorindexlist[conductorindex]].shapeheight,
            )
            insert_conductor_into_pattern_file(
                FasterCapFile,
                conductorfile,
                round(base_epsilon * float(1e-6), 8),
                PatternShapes[conductorindexlist[conductorindex]].shapex,
                PatternShapes[conductorindexlist[conductorindex]].shapey,
                PatternShapes[conductorindexlist[conductorindex]].shapez,
            )
            FasterCapFile.write("\n\n")

        else:
            intersectiondiel = 0

            for intersection in intersectionslist:

                if intersectiondiel == 0:
                    conductorfile = (
                        "Wires/wire_"
                        + PatternShapes[conductorindexlist[conductorindex]].name
                        + "_W"
                        + str(
                            PatternShapes[conductorindexlist[conductorindex]].shapewidth
                        )
                        + "_T"
                        + str(PatternShapes[intersection].shapethickness)
                        + "_H"
                        + str(
                            PatternShapes[
                                conductorindexlist[conductorindex]
                            ].shapeheight
                        )
                        + "-bottom.txt"
                    )
                    write_wire_bottom_panel(
                        conductorfile,
                        PatternShapes[conductorindexlist[conductorindex]].name,
                        PatternShapes[conductorindexlist[conductorindex]].shapewidth,
                        PatternShapes[intersection].shapethickness,
                        PatternShapes[conductorindexlist[conductorindex]].shapeheight,
                    )
                    insert_conductor_into_pattern_file(
                        FasterCapFile,
                        conductorfile,
                        round(
                            float(
                                processDielectrics[
                                    PatternShapes[
                                        dielindexlist[intersection]
                                    ].shapeorder
                                ].epsilon
                            )
                            * float(1.0e-6),
                            8,
                        ),
                        PatternShapes[conductorindexlist[conductorindex]].shapex,
                        PatternShapes[intersection].shapey,
                        PatternShapes[conductorindexlist[conductorindex]].shapez,
                    )
                    FasterCapFile.write(" + \n")

                conductorfile = (
                    "Wires/wire_"
                    + PatternShapes[conductorindexlist[conductorindex]].name
                    + "_W"
                    + str(PatternShapes[conductorindexlist[conductorindex]].shapewidth)
                    + "_T"
                    + str(PatternShapes[intersection].shapethickness)
                    + "_H"
                    + str(PatternShapes[conductorindexlist[conductorindex]].shapeheight)
                    + "-sides.txt"
                )
                write_wire_side_panels(
                    conductorfile,
                    PatternShapes[conductorindexlist[conductorindex]].name,
                    PatternShapes[conductorindexlist[conductorindex]].shapewidth,
                    PatternShapes[intersection].shapethickness,
                    PatternShapes[conductorindexlist[conductorindex]].shapeheight,
                )
                insert_conductor_into_pattern_file(
                    FasterCapFile,
                    conductorfile,
                    round(
                        float(
                            processDielectrics[
                                PatternShapes[dielindexlist[intersection]].shapeorder
                            ].epsilon
                        )
                        * float(1.0e-6),
                        8,
                    ),
                    PatternShapes[conductorindexlist[conductorindex]].shapex,
                    PatternShapes[intersection].shapey,
                    PatternShapes[conductorindexlist[conductorindex]].shapez,
                )
                FasterCapFile.write(" + \n")

                if intersectiondiel == len(intersectionslist) - 1:
                    conductorfile = (
                        "Wires/wire_"
                        + PatternShapes[conductorindexlist[conductorindex]].name
                        + "_W"
                        + str(
                            PatternShapes[conductorindexlist[conductorindex]].shapewidth
                        )
                        + "_T"
                        + str(PatternShapes[intersection].shapethickness)
                        + "_H"
                        + str(
                            PatternShapes[
                                conductorindexlist[conductorindex]
                            ].shapeheight
                        )
                        + "-top.txt"
                    )
                    write_wire_top_panel(
                        conductorfile,
                        PatternShapes[conductorindexlist[conductorindex]].name,
                        PatternShapes[conductorindexlist[conductorindex]].shapewidth,
                        PatternShapes[intersection].shapethickness,
                        PatternShapes[conductorindexlist[conductorindex]].shapeheight,
                    )
                    insert_conductor_into_pattern_file(
                        FasterCapFile,
                        conductorfile,
                        round(
                            float(
                                processDielectrics[
                                    PatternShapes[
                                        dielindexlist[intersection]
                                    ].shapeorder
                                    + 1
                                ].epsilon
                            )
                            * float(1.0e-6),
                            8,
                        ),
                        PatternShapes[conductorindexlist[conductorindex]].shapex,
                        PatternShapes[intersection].shapey,
                        PatternShapes[conductorindexlist[conductorindex]].shapez,
                    )
                    FasterCapFile.write("\n\n")

                intersectiondiel += 1
        conductorindex += 1

    dielindex = 0
    for intersections in dielintersections:

        if "m" in PatternShapes[dielindexlist[dielindex]].name:
            dielectricororder = int(
                PatternShapes[dielindexlist[dielindex]].name.split("_")[0].split("m")[1]
            )
        else:
            dielectricororder = 0

        if (dielectricororder >= minimumconductorlayer) and (
            dielectricororder <= maximumconductorlayer
        ):
            if len(intersections) == 0:
                # create deielectric files covering the entire space #
                # create the plane for the dielectric if they are not exist covering the entire window for the
                # specific thickness and insert the information into FasterCap pattern file

                if groundconductorexists == 0:
                    if PatternShapes[dielindexlist[dielindex]].shapeorder == 0:
                        dielectricfile = (
                            "Dielectrics/dielectric_"
                            + PatternShapes[dielindexlist[dielindex]].name
                            + "_W"
                            + str(PatternShapes[dielindexlist[dielindex]].shapewidth)
                            + "_T"
                            + str(
                                PatternShapes[dielindexlist[dielindex]].shapethickness
                            )
                            + "_H"
                            + str(PatternShapes[dielindexlist[dielindex]].shapeheight)
                            + "-bottom.txt"
                        )
                        write_dielectric_bottom_panel(
                            dielectricfile,
                            PatternShapes[dielindexlist[dielindex]].name,
                            PatternShapes[dielindexlist[dielindex]].shapewidth,
                            PatternShapes[dielindexlist[dielindex]].shapethickness,
                            PatternShapes[dielindexlist[dielindex]].shapeheight,
                        )
                        insert_dielectric_into_pattern_file(
                            FasterCapFile,
                            dielectricfile,
                            float(1.0e-6),
                            round(
                                float(
                                    processDielectrics[
                                        PatternShapes[
                                            dielindexlist[dielindex]
                                        ].shapeorder
                                    ].epsilon
                                )
                                * float(1.0e-6),
                                8,
                            ),
                            PatternShapes[dielindexlist[dielindex]].shapex,
                            PatternShapes[dielindexlist[dielindex]].shapey,
                            0.0,
                            round(
                                PatternShapes[dielindexlist[dielindex]].shapex + 0.01, 4
                            ),
                            round(
                                PatternShapes[dielindexlist[dielindex]].shapey + 0.01, 4
                            ),
                            0.0 + 0.01,
                        )
                    else:
                        dielectricfile = (
                            "Dielectrics/dielectric_"
                            + PatternShapes[dielindexlist[dielindex]].name
                            + "_W"
                            + str(PatternShapes[dielindexlist[dielindex]].shapewidth)
                            + "_T"
                            + str(
                                PatternShapes[dielindexlist[dielindex]].shapethickness
                            )
                            + "_H"
                            + str(PatternShapes[dielindexlist[dielindex]].shapeheight)
                            + "-bottom.txt"
                        )
                        write_dielectric_bottom_panel(
                            dielectricfile,
                            PatternShapes[dielindexlist[dielindex]].name,
                            PatternShapes[dielindexlist[dielindex]].shapewidth,
                            PatternShapes[dielindexlist[dielindex]].shapethickness,
                            PatternShapes[dielindexlist[dielindex]].shapeheight,
                        )
                        insert_dielectric_into_pattern_file(
                            FasterCapFile,
                            dielectricfile,
                            round(
                                float(
                                    processDielectrics[
                                        PatternShapes[
                                            dielindexlist[dielindex]
                                        ].shapeorder
                                        - 1
                                    ].epsilon
                                )
                                * float(1.0e-6),
                                8,
                            ),
                            round(
                                float(
                                    processDielectrics[
                                        PatternShapes[
                                            dielindexlist[dielindex]
                                        ].shapeorder
                                    ].epsilon
                                )
                                * float(1.0e-6),
                                8,
                            ),
                            PatternShapes[dielindexlist[dielindex]].shapex,
                            PatternShapes[dielindexlist[dielindex]].shapey,
                            0.0,
                            round(
                                PatternShapes[dielindexlist[dielindex]].shapex + 0.01, 4
                            ),
                            round(
                                PatternShapes[dielindexlist[dielindex]].shapey + 0.01, 4
                            ),
                            0.0 + 0.01,
                        )

                dielectricfile = (
                    "Dielectrics/dielectric_"
                    + PatternShapes[dielindexlist[dielindex]].name
                    + "_W"
                    + str(PatternShapes[dielindexlist[dielindex]].shapewidth)
                    + "_T"
                    + str(PatternShapes[dielindexlist[dielindex]].shapethickness)
                    + "_H"
                    + str(PatternShapes[dielindexlist[dielindex]].shapeheight)
                    + "-sides.txt"
                )
                write_dielectric_side_panels(
                    dielectricfile,
                    PatternShapes[dielindexlist[dielindex]].name,
                    PatternShapes[dielindexlist[dielindex]].shapewidth,
                    PatternShapes[dielindexlist[dielindex]].shapethickness,
                    PatternShapes[dielindexlist[dielindex]].shapeheight,
                )
                insert_dielectric_into_pattern_file(
                    FasterCapFile,
                    dielectricfile,
                    1.0e-6,
                    round(
                        float(
                            processDielectrics[
                                PatternShapes[dielindexlist[dielindex]].shapeorder
                            ].epsilon
                        )
                        * float(1.0e-6),
                        8,
                    ),
                    PatternShapes[dielindexlist[dielindex]].shapex,
                    PatternShapes[dielindexlist[dielindex]].shapey,
                    0.0,
                    round(PatternShapes[dielindexlist[dielindex]].shapex + 0.01, 4),
                    round(PatternShapes[dielindexlist[dielindex]].shapey + 0.01, 4),
                    0.0 + 0.01,
                )

                if dielindex < len(dielintersections) - 1:
                    dielectricfile = (
                        "Dielectrics/dielectric_"
                        + PatternShapes[dielindexlist[dielindex]].name
                        + "_W"
                        + str(PatternShapes[dielindexlist[dielindex]].shapewidth)
                        + "_T"
                        + str(PatternShapes[dielindexlist[dielindex]].shapethickness)
                        + "_H"
                        + str(PatternShapes[dielindexlist[dielindex]].shapeheight)
                        + "-top.txt"
                    )
                    write_dielectric_top_panel(
                        dielectricfile,
                        PatternShapes[dielindexlist[dielindex]].name,
                        PatternShapes[dielindexlist[dielindex]].shapewidth,
                        PatternShapes[dielindexlist[dielindex]].shapethickness,
                        PatternShapes[dielindexlist[dielindex]].shapeheight,
                    )
                    insert_dielectric_into_pattern_file(
                        FasterCapFile,
                        dielectricfile,
                        round(
                            float(
                                processDielectrics[
                                    PatternShapes[dielindexlist[dielindex]].shapeorder
                                    + 1
                                ].epsilon
                            )
                            * float(1.0e-6),
                            8,
                        ),
                        round(
                            float(
                                processDielectrics[
                                    PatternShapes[dielindexlist[dielindex]].shapeorder
                                ].epsilon
                            )
                            * float(1.0e-6),
                            8,
                        ),
                        PatternShapes[dielindexlist[dielindex]].shapex,
                        PatternShapes[dielindexlist[dielindex]].shapey,
                        0.0,
                        round(PatternShapes[dielindexlist[dielindex]].shapex + 0.01, 4),
                        round(PatternShapes[dielindexlist[dielindex]].shapey + 0.01, 4),
                        0.0 + 0.01,
                    )

                FasterCapFile.write("\n\n")

            else:
                # create dielectric files respecting the coordinates of conductors #

                rowsstartingpoints = []
                rowsnumber = patternswindowheight / 0.01

                for i in range(int(rowsnumber)):
                    rowsstartingpoints.append(window_min_x)

                dielectricsgroups = []
                groupindex = 0

                while 1:
                    previousgroup_x = window_min_x

                    # for each row have to find the intersection with conductor #
                    for i in range(int(rowsnumber)):

                        row_x = rowsstartingpoints[i]
                        row_z = 0.01 * i

                        starting_row_x = row_x
                        ending_row_x = window_max_x
                        min_row_x_intersection = window_max_x

                        # iterate through conductors #
                        for conductor in dielintersections[dielindex]:
                            conductor_x = PatternShapes[conductor].shapex
                            conductor_z = PatternShapes[conductor].shapez

                            conductor_h = PatternShapes[conductor].shapeheight
                            conductor_w = PatternShapes[conductor].shapewidth

                            if (conductor_z <= row_z) and (
                                conductor_z + conductor_h > row_z
                            ):
                                if conductor_x > row_x:
                                    if min_row_x_intersection > conductor_x:
                                        min_row_x_intersection = (
                                            conductor_x + conductor_w
                                        )
                                        ending_row_x = conductor_x

                        rowsstartingpoints[i] = min_row_x_intersection

                        if starting_row_x != window_max_x:
                            if i != 0:
                                if previousgroup_x == min_row_x_intersection:
                                    dielectricsgroups[
                                        len(dielectricsgroups) - 1
                                    ].append(i)
                                else:
                                    dielectricsgroups.append([])
                                    dielectricsgroups[
                                        len(dielectricsgroups) - 1
                                    ].append(starting_row_x)
                                    dielectricsgroups[
                                        len(dielectricsgroups) - 1
                                    ].append(ending_row_x)
                                    dielectricsgroups[
                                        len(dielectricsgroups) - 1
                                    ].append(i)
                                    previousgroup_x = min_row_x_intersection
                            else:
                                previousgroup_x = min_row_x_intersection
                                dielectricsgroups.append([])
                                dielectricsgroups[len(dielectricsgroups) - 1].append(
                                    starting_row_x
                                )
                                dielectricsgroups[len(dielectricsgroups) - 1].append(
                                    ending_row_x
                                )
                                dielectricsgroups[len(dielectricsgroups) - 1].append(i)
                        else:
                            if len(dielectricsgroups[len(dielectricsgroups) - 1]) != 0:
                                previousgroup_x = window_min_x

                    finished = 1
                    for i in range(int(rowsnumber)):
                        row_x = rowsstartingpoints[i]
                        if row_x != window_max_x:
                            finished = 0
                            break

                    if finished == 1:
                        break

                for block in dielectricsgroups:

                    block_width = round(block[1] - block[0], 4)
                    block_height = round((block[-1] * 0.01) - (block[3] * 0.01), 4)
                    block_thickness = PatternShapes[
                        dielindexlist[dielindex]
                    ].shapethickness

                    block_x = block[0]
                    block_y = PatternShapes[dielindexlist[dielindex]].shapey
                    block_z = round(block[3] * 0.01, 4)

                    if dielindex == 0:
                        dielectricfile = (
                            "Dielectrics/dielectric_"
                            + PatternShapes[dielindexlist[dielindex]].name
                            + "_W"
                            + str(block_width)
                            + "_T"
                            + str(block_thickness)
                            + "_H"
                            + str(block_height)
                            + "-bottom.txt"
                        )
                        write_dielectric_bottom_panel(
                            dielectricfile,
                            PatternShapes[dielindexlist[dielindex]].name,
                            block_width,
                            block_thickness,
                            block_height,
                        )
                        insert_dielectric_into_pattern_file(
                            FasterCapFile,
                            dielectricfile,
                            round(
                                float(
                                    processDielectrics[
                                        PatternShapes[
                                            dielindexlist[dielindex]
                                        ].shapeorder
                                        - 1
                                    ].epsilon
                                )
                                * float(1.0e-6),
                                8,
                            ),
                            round(
                                float(
                                    processDielectrics[
                                        PatternShapes[
                                            dielindexlist[dielindex]
                                        ].shapeorder
                                    ].epsilon
                                )
                                * float(1.0e-6),
                                8,
                            ),
                            round(block_x, 4),
                            round(block_y, 4),
                            round(block_z, 4),
                            round(block_x + 0.01, 4),
                            round(block_y + 0.01, 4),
                            round(block_z + 0.01, 4),
                        )

                    dielectricfile = (
                        "Dielectrics/dielectric_"
                        + PatternShapes[dielindexlist[dielindex]].name
                        + "_W"
                        + str(block_width)
                        + "_T"
                        + str(block_thickness)
                        + "_H"
                        + str(block_height)
                        + "-sides.txt"
                    )
                    write_dielectric_side_panels(
                        dielectricfile,
                        PatternShapes[dielindexlist[dielindex]].name,
                        block_width,
                        block_thickness,
                        block_height,
                    )
                    insert_dielectric_into_pattern_file(
                        FasterCapFile,
                        dielectricfile,
                        1.0e-6,
                        round(
                            float(
                                processDielectrics[
                                    PatternShapes[dielindexlist[dielindex]].shapeorder
                                ].epsilon
                            )
                            * float(1.0e-6),
                            8,
                        ),
                        round(block_x, 4),
                        round(block_y, 4),
                        round(block_z, 4),
                        round(block_x + 0.01, 4),
                        round(block_y + 0.01, 4),
                        round(block_z + 0.01, 4),
                    )

                    if dielindex < len(dielintersections) - 1:
                        dielectricfile = (
                            "Dielectrics/dielectric_"
                            + PatternShapes[dielindexlist[dielindex]].name
                            + "_W"
                            + str(block_width)
                            + "_T"
                            + str(block_thickness)
                            + "_H"
                            + str(block_height)
                            + "-top.txt"
                        )
                        write_dielectric_top_panel(
                            dielectricfile,
                            PatternShapes[dielindexlist[dielindex]].name,
                            block_width,
                            block_thickness,
                            block_height,
                        )
                        insert_dielectric_into_pattern_file(
                            FasterCapFile,
                            dielectricfile,
                            round(
                                float(
                                    processDielectrics[
                                        PatternShapes[
                                            dielindexlist[dielindex]
                                        ].shapeorder
                                        + 1
                                    ].epsilon
                                )
                                * float(1.0e-6),
                                8,
                            ),
                            round(
                                float(
                                    processDielectrics[
                                        PatternShapes[
                                            dielindexlist[dielindex]
                                        ].shapeorder
                                    ].epsilon
                                )
                                * float(1.0e-6),
                                8,
                            ),
                            round(block_x, 4),
                            round(block_y, 4),
                            round(block_z, 4),
                            round(block_x + 0.01, 4),
                            round(block_y + 0.01, 4),
                            round(block_z + 0.01, 4),
                        )

                FasterCapFile.write("\n\n")
        dielindex += 1

    FasterCapFile.close()


def main(argv):

    syntax = "python3 UniversalFormat2FasterCap.py <Process File> <Universal_format_pattern_folder> <FasterCap_format_output_folder> <heights_reference>(normalized|standard) ?-sim_window_ext <lower_dx> <lower_dz> <lower_dy> <upper_dx> <upper_dz> <upper_dy>"

    argvlength = len(argv)

    ## check if a file path has been specified ##
    if argvlength != 5 and argvlength != 12:
        print("ERROR! Wrong number of arguments!")
        print("Re-run with: " + syntax)
        sys.exit(-2)

    ## check specified Raphael Format file path exists ##
    if not (os.path.exists(argv[1])):
        print("ERROR! Specified Process File path does not exist!")
        print("Re-run with: " + syntax)
        sys.exit(-2)

    ## check specified Raphael Format file path exists ##
    if not (os.path.exists(argv[2])):
        print("ERROR! Specified Raphael Pattern File path does not exist!")
        print("Re-run with: " + syntax)
        sys.exit(-2)

    ## check specified FasterCap output folder path exists ##
    if not (os.path.exists(argv[3])):
        print("ERROR! Specified FasterCap Folder path does not exist!")
        print("Re-run with: " + syntax)
        sys.exit(-2)

    global normalized_heights
    ## check if heights are normalized or standard in reference to ground planes ##
    if argv[4] == "normalized":
        print("Heights are assumed normalized based on the lowest Ground Plane!")
        normalized_heights = True
    elif argv[4] == "standard":
        print("Heights have their standard values")
        normalized_heights = False
    else:
        print("ERROR! Uknown specified heights_reference value!")
        print("Re-run with: " + syntax)
        sys.exit(-2)

    global window_min_x_ext
    global window_min_z_ext
    global window_min_y_ext
    global window_max_x_ext
    global window_max_z_ext
    global window_max_y_ext
    global user_window_ext

    ## check if simulation window extension is provided ##
    if argvlength != 5:
        if not argv[5] == "-sim_window_ext":
            print(f"ERROR! Invalid argument {argv[5]}!")
            print("Re-run with: " + syntax)
            sys.exit(-2)
        else:
            try:
                window_min_x_ext = float(argv[6])
                window_min_z_ext = float(argv[7])
                window_min_y_ext = float(argv[8])
                window_max_x_ext = float(argv[9])
                window_max_z_ext = float(argv[10])
                window_max_y_ext = float(argv[11])
                user_window_ext = True
            except ValueError:
                print("ERROR! Simulation Window Extension parameters must be numbers!")
                print("Re-run with: " + syntax)
                sys.exit(-2)
    else:
        window_min_x_ext = 0.0
        window_min_z_ext = 0.0
        window_min_y_ext = 0.0
        window_max_x_ext = 0.0
        window_max_z_ext = 0.0
        window_max_y_ext = 0.0
        user_window_ext = False

    global processMetals
    global processDielectrics
    global PatternShapes

    global patternswindowwidth
    global patternswindowthickness
    global patternswindowheight
    global currentpath

    currentpath = os.getcwd()

    patternswindowwidth = 0.0
    patternswindowthickness = 0.0
    patternswindowheight = 0.0

    processMetals = []
    processDielectrics = []
    PatternShapes = []

    processTechFile(argv[1])

    # for i in range(len(processDielectrics)):
    #   processDielectrics[i].print_contents()

    # for i in range(len(processMetals)):
    #   processMetals[i].print_contents()

    TranslateUniversalFile(argv[2], argv[3])


if __name__ == "__main__":
    main(sys.argv[0:])
