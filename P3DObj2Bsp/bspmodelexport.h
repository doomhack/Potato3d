#ifndef BSPMODELEXPORT_H
#define BSPMODELEXPORT_H

#include <QtCore>
#include <QVector3D>

#include "bspbuilder.h"
#include "config.h"
#include "../BspModelDefs.h"


namespace Obj2Bsp
{
    class BspModelExport
    {
    public:
        BspModelExport();

        QByteArray ExportBSPModel(Obj2Bsp::BspNode* root, Model3d* model);

    private:
        void TraverseNodesRecursive(BspNode* n, QList<BspNode*>& nodeList);

    };

}

#endif // BSPMODELEXPORT_H
