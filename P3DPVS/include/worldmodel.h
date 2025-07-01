#ifndef WORLDMODEL_H
#define WORLDMODEL_H

#include "../include/common.h"
#include "../../Config.h"
#include "../../3dmaths/f3dmath.h"
#include "../../bspmodel.h"
#include "model.h"


class WorldModel
{
public:
    explicit WorldModel();

    const P3D::BspModel* GetModel() const;
private:
    const P3D::BspModel* model = (P3D::BspModel*)&modeldata;
    P3D::BspModel* readWriteModel = nullptr;
};

#endif // WORLDMODEL_H
