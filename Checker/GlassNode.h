#ifndef GLASSNODE_H
#define GLASSNODE_H



/* This class is created
 * to model a solution node instance with its all attributes
**/
class GlassNode
{
    public:
      GlassNode ();
      GlassNode(unsigned int plate, unsigned int node, unsigned int x,
                unsigned int y, unsigned int w, unsigned int h,
                int t, unsigned int c, unsigned int p, unsigned int *c_array, unsigned int n);
      virtual ~GlassNode();
      unsigned int Getplate_id() { return plate_id; }
      unsigned int Getnode_id() { return node_id; }
      unsigned int Getpos_x() { return pos_x; }
      unsigned int Getpos_y() { return pos_y; }
      unsigned int Getwidth() { return width; }
      unsigned int Getheight() { return height; }
      int Gettype() { return type; }
      unsigned int Getcut() { return cut; }
      unsigned int Getparent() { return parent; }
      unsigned int Getsuccessor_nbr() { return successor_nbr; }
      GlassNode Getsuccessor (unsigned int idx) { return successor[idx]; }

      void Setplate_id (unsigned int id) { plate_id = id; }
      void Setnode_id (unsigned int id) { node_id = id; }
      void Setpos_x (unsigned int x) { pos_x = x; }
      void Setpos_y (unsigned int y) { pos_y = y; }
      void Setwidth (unsigned int w) { width = w; }
      void Setheight (unsigned int h) { height = h; }
      void Settype (int t) { type = t; }
      void Setcut (unsigned int c) { cut = c; }
      void Setparent (unsigned int p) { parent = p; }
      void Setchild_nbr (unsigned int nbr) { successor_nbr = nbr; }
      unsigned int Setsuccessor (GlassNode c) { if (successor_nbr >= 2 && cut > 2) { return 1; } successor[successor_nbr] = c; successor_nbr++; return 0; }

    public:
      GlassNode * successor; // Successor nodes list.
    protected:
    private:
      unsigned int plate_id; // Plate Id.
      unsigned int node_id; // Node Id.
      unsigned int pos_x; // x position.
      unsigned int pos_y; // y position.
      unsigned int width; // Plate's width.
      unsigned int height; // Plate's height.
      int type;   /* node type
                   * > 0 glass piece index in batch file.
                   * = -1 wasted glass.
                   * = -2 branch.
                   * = -3 residual.
                   */
      unsigned int cut; // cut level (there are 1, 2, 3 and 4-cut level, could not be 0-cut because GlassNode class instance can't be an entire plate).
      unsigned int parent; // parent node Id.
      unsigned int successor_nbr; // number of children for this node instance.
};

#endif // GLASSNODE_H
