#include "GlassNode.h"

GlassNode::GlassNode()
{
    plate_id = 0;
    node_id = 0;
    pos_x = 0;
    pos_y = 0;
    width = 0;
    height = 0;
    type = 0;
    cut = 0;
    parent = 0;
    successor_nbr = 0;
}

GlassNode::~GlassNode()
{
    //dtor
}
