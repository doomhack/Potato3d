#include "../include/worldmodel.h"

WorldModel::WorldModel()
{
    const unsigned int modelSize = 32 * 1024 * 1024;

    readWriteModel = (P3D::BspModel*)new unsigned char[modelSize];
    memcpy(readWriteModel, model, 4049316UL);
}

const P3D::BspModel* WorldModel::GetModel() const
{
    return readWriteModel;
}
