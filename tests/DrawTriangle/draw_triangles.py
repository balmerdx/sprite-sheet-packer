from PyQt6.QtWidgets import QLabel, QMainWindow, QApplication
from PyQt6.QtGui import QPixmap, QImage, QColor, QPainter, QPolygon, QBrush, QMouseEvent
from PyQt6.QtCore import QSize, Qt, QPoint
import sys
import os
import plistlib

filename = r'Z:\projects\tmp\polygon_atlas\эхо\out\hdr\a1.png'
#filename = r'Z:\projects\tmp\polygon_atlas\эхо\out-tps\out.png'
#filename = r'Z:\projects\tmp\polygon_atlas\эхо_sm\out\hdr\a1.png'

def triangle_area(p : list[QPoint]):
    assert(len(p)==3)
    a = p[1] - p[0]
    b = p[2] - p[0]
    return abs(a.x()*b.y() - a.y()*b.x())*0.5

def triangles_area(triangles : list[list[QPoint]]):
    area = 0.
    for t in triangles:
        area += triangle_area(t)
    return area

def intersect_tri(triangle : list[QPoint], point : QPoint):
    def cross(a : QPoint, b : QPoint):
        return a.x()*b.y() - a.y()*b.x()
    def line_sign(pt, p0, p1):
        return cross(pt-p0, p1-p0)
    assert(len(triangle)==3)
    s0 = line_sign(point, triangle[0], triangle[1])
    s1 = line_sign(point, triangle[1], triangle[2])
    s2 = line_sign(point, triangle[2], triangle[0])
    return (s0 > 0 and s1 > 0 and s2 > 0) or (s0 < 0 and s1 < 0 and s2 < 0)


class TextureData:
    def __init__(self, filename):
        with open(filename, "rb") as file:
            data = plistlib.load(file, fmt=plistlib.FMT_XML)
        frames = data['frames']

        self.frames = {}
        sum_triangles = 0
        sum_area = 0
        for tex_name in frames.keys():
            #print(tex_name)
            tex_dict = frames[tex_name]
            #print(len(tex_dict['triangles'].split(" ")))
            #print(len(tex_dict['verticesUV'].split(" ")))
            #triangles_uv - тройки вида [QPoint(0,0), QPoint(100, 0), QPoint(0, 100)]
            triangles_uv = []
            if 'triangles' in tex_dict:
                self._add_triangles_uv(triangles_uv, tex_dict['triangles'], tex_dict['verticesUV'])
            else:
                print(f"Sprite without triangles! `{tex_name}`")
            self.frames[tex_name] = triangles_uv
            sum_triangles += len(triangles_uv)
            sum_area += triangles_area(triangles_uv)

        #print(self.triangles_uv)
        print("triangles_count =", sum_triangles)
        print("triangles_area =", sum_area)
        #exit()

    def _add_triangles_uv(self, triangles_list, str_tiangles, str_vertices):
        tri_indices = [int(s) for s in str_tiangles.split(" ")]
        tri_vertices = [int(s) for s in str_vertices.split(" ")]
        for ti in range(0, len(tri_indices), 3):
            tri_idx = tri_indices[ti:ti+3]
            p = []
            for i in tri_idx:
                p.append(QPoint(tri_vertices[i*2], tri_vertices[i*2+1]))
            triangles_list.append(p)

    def find_triangle(self, point : QPoint):
        for name, triangles_uv in self.frames.items():
            for tri in triangles_uv:
                if intersect_tri(tri, point):
                    return name
        return None


class MainWindow(QMainWindow):

    def __init__(self):
        super().__init__()
        self.setWindowTitle("Title")
        self.label = QLabel(self)
        self.label.setAlignment(Qt.AlignmentFlag.AlignTop)
        td = TextureData(os.path.splitext(filename)[0]+".plist")
        self.td = td

        self.statusBar().showMessage("Status message!")

        checkmate = self.loadPixmapWithCheckmate(filename)
        self.drawTriangles(checkmate, td)
        QImage(checkmate).save("filled.png")

        max_width = 1200
        self.scale_image = 1.
        if checkmate.width() > max_width:
            self.scale_image = checkmate.width() / max_width
            checkmate = checkmate.scaledToWidth(max_width, Qt.TransformationMode.SmoothTransformation)

        self.label.setPixmap(checkmate)
        self.setCentralWidget(self.label)
        self.show()
        self.move(300,0)

    def createCheckmate(self, size : QSize) -> QImage:
        print(f"{size.width()}, {size.height()}")
        image = QImage(size, QImage.Format.Format_RGB32)
        c0 = QColor(103, 103, 103, 255).rgba()
        c1 = QColor(153, 153, 153, 255).rgba()
        check_size = 10
        for y in range(size.height()):
            for x in range(size.width()):
                image.setPixel(x, y, c0 if (x//check_size+y//check_size)%2==0 else c1)
        return image
    
    def loadPixmapWithCheckmate(self, filename):
        img = QImage(filename)
        checkmate = QPixmap(self.createCheckmate(img.size()))
        p = QPainter(checkmate)
        p.drawImage(0,0, img)
        p.end()
        return checkmate
    
    def drawTriangles(self, pixmap : QPixmap, td : TextureData):
        brush = QBrush(QColor(0, 255, 0, 50))
        p = QPainter(pixmap)
        p.setBrush(brush)
        for triangles_uv in td.frames.values():
            for tri in triangles_uv:
                points = QPolygon(tri)
                p.drawPolygon(points, Qt.FillRule.OddEvenFill)
        p.end()

    def findElem(self, event : QMouseEvent):
        pos = self.label.mapFromGlobal(event.globalPosition())
        pos = QPoint(round(pos.x() * self.scale_image), round(pos.y() * self.scale_image))
        name = self.td.find_triangle(pos)
        self.statusBar().showMessage(f"{pos.x()},{pos.y()} {name}")

    def mousePressEvent(self, event : QMouseEvent):
        if event.button() == Qt.MouseButton.LeftButton:
            self.findElem(event)
    def mouseMoveEvent(self, event : QMouseEvent):
        self.findElem(event)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    app = QApplication(sys.argv)
    ex = MainWindow()
    sys.exit(app.exec())