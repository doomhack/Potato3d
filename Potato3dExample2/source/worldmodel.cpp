#include "../include/worldmodel.h"

WorldModel::WorldModel()
{
    model->SetVisData(vis_data);
}

const P3D::BspModel* WorldModel::GetModel() const
{
    return model;
}
