#include <QCoreApplication>
#include "objloader.h"
#include "bspbuilder.h"
#include "bspmodelexport.h"

void SaveBytesAsCFile(QByteArray bytes, QString file);

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QImageReader::setAllocationLimit(4096);

    Obj2Bsp::ObjLoader loader;

    //QString objPath = "C:\\Users\\Zak\\Downloads\\DDHQ\\ddhq.obj";
    //QString objPath = "C:\\Users\\Zak\\Downloads\\Villa\\villa.obj";
    QString objPath = "C:\\Users\\Zak\\Downloads\\Dam\\dam.obj";
    //QString objPath = "C:\\Users\\Zak\\Downloads\\Temple\\temple.obj";
    //QString objPath = "C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\Streets\\Streets.obj";
    //QString objPath = "C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\temple.obj";
    //QString objPath = "C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\hf\\hf2.obj";
    //QString objPath = "C:\\Users\\Zak\\Documents\\GitProjects\\Potato3d\\Potato3dExample\\models\\d2\\driver2_small.obj";

    bool result = loader.LoadObjFile(objPath);

    if(!result)
        return 0;

    Obj2Bsp::BspBuilder bspBuilder;

    Obj2Bsp::BspNode* root = bspBuilder.BuildBSPTree(loader.GetModel());

    Obj2Bsp::BspModelExport bspExport;

    QByteArray bspData = bspExport.ExportBSPModel(root, loader.GetModel());

    QDir workDir = QDir(QFileInfo(objPath).absolutePath());
    QString baseName = QFileInfo(objPath).fileName().chopped(3);

    QFile bspFile(workDir.filePath(baseName + "bsp"));
    bspFile.open(QFile::Truncate | QFile::ReadWrite);
    bspFile.write(bspData);
    bspFile.close();

    SaveBytesAsCFile(bspData, workDir.filePath(baseName + "cpp"));
}

void SaveBytesAsCFile(QByteArray bytes, QString file)
{
    QFile f(file);

    if(!f.open(QIODevice::Truncate | QIODevice::ReadWrite))
        return;

    QString decl = QString("const extern unsigned char modeldata[%1UL] = {\n").arg(bytes.size());

    f.write(decl.toLatin1());

    for(int i = 0; i < bytes.size(); i++)
    {
        QString element = QString("0x%1,").arg((quint8)bytes.at(i),2, 16, QChar('0'));

        if(( (i+1) % 40) == 0)
            element += "\n";

        f.write(element.toLatin1());
    }

    QString close = QString("\n};");
    f.write(close.toLatin1());

    f.close();
}
