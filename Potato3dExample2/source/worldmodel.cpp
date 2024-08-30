#include "../include/worldmodel.h"

WorldModel::WorldModel()
{

}

const P3D::BspModel* WorldModel::GetModel() const
{
    return model;
}
