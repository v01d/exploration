#include <iostream>
#include <limits>
#include "topo_map.h"
#include "occupancy_grid.h"
#include "super_matrix.h"
using namespace HybNav;
using namespace std;

template<class T>
SuperMatrix<T>::SuperMatrix(void) {
  size_x = size_y = 0;
  min_x = min_y = std::numeric_limits<ssize_t>::max();
  max_x = max_y = std::numeric_limits<ssize_t>::min();
  count = 0;
}

template<class T>
double& SuperMatrix<T>::cell(ssize_t x, ssize_t y) {
  ssize_t mx = (x < 0 ? ((x + 1) / (ssize_t)T::CELLS) - 1 : x / (ssize_t)T::CELLS);
  ssize_t my = (y < 0 ? ((y + 1) / (ssize_t)T::CELLS) - 1 : y / (ssize_t)T::CELLS);
  T& m = submatrix(mx, my);

  ssize_t cx, cy;
  if (x < 0) cx = T::CELLS - ((-(x + 1)) % T::CELLS) - 1;
  else cx = x % T::CELLS;
  if (y < 0) cy = T::CELLS - ((-(y + 1)) % T::CELLS) - 1;
  else cy = y % T::CELLS;

  //cout << "x/y: " << x << "/" << y << " mx/my: " << mx << "/" << my << " cx/cy: " << cx << "/" << cy << endl;
  return m(cx, cy);
}


template<class T>
T& SuperMatrix<T>::submatrix(ssize_t x, ssize_t y) {
  if (x < min_x) min_x = x;
  if (x > max_x) max_x = x;
  if (y < min_y) min_y = y;
  if (y > max_y) max_y = y;
  
  size_x = (max_x - min_x) + 1;
  size_y = (max_y - min_y) + 1;
  
  //cout << "submatrix: " << x << "," << y << endl;
  iterator_x it = matrix_map.find(x);
  iterator_y it2;
  if (it == matrix_map.end()) {
    map<ssize_t, T> y_map;
    y_map.insert(make_pair(y, T(x,y)));
    matrix_map.insert(make_pair(x, y_map));
    it2 = matrix_map.find(x)->second.find(y);
    count++;
  }
  else {
    it2 = it->second.find(y);
    if (it2 == it->second.end()) {
      it2 = it->second.insert(make_pair(y, T(x,y))).first;
      count++;
    }
  }
  return it2->second;
}

template<class T>
gsl::vector_int SuperMatrix<T>::get_cell_coordinate(gsl::vector_int local_cell_coord, gsl::vector_int grid_position) {
  return (grid_position * T::CELLS) + local_cell_coord;
}

template<class T>
void SuperMatrix<T>::to_dot(ostream& out) {
  out << "graph G {" << std::endl;
  out << "\tnode [shape=\"box\"]" << std::endl;

  for (iterator_x itx = matrix_map.begin(); itx != matrix_map.end(); ++itx) {
    for (iterator_y ity = itx->second.begin(); ity != itx->second.end(); ++ity) {
      out << "\t" << itx->first * 100000 + ity->first << "["; // dummy index
      T& t = ity->second;
      t.to_dot(out);
      out << "]" << endl;
    }
  }
  out << "}";
}

template class SuperMatrix<OccupancyGrid>;
