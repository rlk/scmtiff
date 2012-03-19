import xml.etree.ElementTree as ET

# Return an SVG root element with the given pixel width and height.

def svgroot(w, h):
    return ET.Element("svg", xmlns       = "http://www.w3.org/2000/svg",
                             version     = "1.2",
                             baseProfile = "tiny",
                             width       = "{}px".format(w),
                             height      = "{}px".format(h),
                             viewBox     = "0 0 {} {}".format(w, h))

# Return an empty SVG group element.

def svggroup(p):
    return ET.SubElement(p, "g")

# Return an SVG path element that draws a sequence of lines connecting the
# given points.

def svglinelist(p, l):
    if l:
        s = "M{} {}".format(l[0][0], l[0][1])

        for i in l[1:]:
            s = s + "L{} {}".format(i[0], i[1])

        return ET.SubElement(p, "path", d = s)
    else:
        return None

# Return an SVG path element that draws a sequence of lines connecting the
# given points and loops from last to first.

def svglineloop(p, l):
    if l:
        s = "M{} {}".format(int(l[0][0]), int(l[0][1]))

        for i in l[1:]:
            s = s + "L{} {}".format(int(i[0]), int(i[1]))

        s = s + "z"
        return ET.SubElement(p, "path", d = s)
    else:
        return None

# Write the given SVG element to the named file.

def svgwrite(svg, name):
    ET.ElementTree(svg).write(name)


