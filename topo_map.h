#ifndef __TOPOMAP_H__
#define __TOPOMAP_H__

#include <algorithm>
#include "metric_map.h"
#include "singleton.h"
#include "graph.h"

namespace HybNav {
  class TopoMap : public Singleton<TopoMap> {
    public:
      class Node;
      class AreaNode;
      class GatewayNode;

      TopoMap(void);

      AreaNode* add_area(OccupancyGrid* grid);
      GatewayNode* add_gateway(OccupancyGrid* grid, Direction edge, uint x0, uint xf);
      void del_node(TopoMap::Node* node);
      void connect(Node* node1, Node* node2);
      void disconnect(Node* node1, Node* node2);

      void merge(AreaNode* node1, AreaNode* node2);

      void save(void);
      void plot(void);

      Node* current_node;

      class Node {
        public:
          virtual ~Node(void) { }

          virtual bool is_gateway(void) const { return false; }
          virtual bool is_area(void) const { return false; }
          virtual void to_dot(std::ostream& out)=0;
          virtual void to_graphml(std::ostream& out)=0;

          std::list<Node*> neighbors(void) {
            std::list<Node*> out;
            Graph<Node>::EdgeArray& edges = TopoMap::instance()->graph.edges;
            for (Graph<Node>::EdgeIterator it = edges.begin(); it != edges.end(); ++it) {
              if (it->first == this) out.push_back(it->second);
              else if (it->second == this) out.push_back(it->first);
            }
            return out;
          }
      };

      class GatewayNode : public Node {
        public:
          GatewayNode(OccupancyGrid* _grid, Direction _edge, uint _x0, uint _xf) : grid(_grid),
            edge(_edge), reach_attempts(0)
          {
            set_dimensions(_x0, _xf);
          }

          bool is_gateway(void) const { return true; }

          void add_reach_attempt(void) { if (reach_attempts < MAX_REACH_ATTEMPTS) reach_attempts++; }
          bool is_inaccessible(void) { return reach_attempts >= MAX_REACH_ATTEMPTS; }
          void set_accessible(void) { reach_attempts = 0; }

          void set_dimensions(uint new_x0, uint new_xf);

          // if this gateway is not connected to another gateway
          bool unexplored_gateway(void);

          gsl::vector_int position(void) const;

          void get_ranges(gsl::vector_int& x_range, gsl::vector_int& y_range);
          
          AreaNode* area_node(void) {
            Graph<TopoMap::Node>::EdgeArray& edges = TopoMap::instance()->graph.edges;
            for(Graph<TopoMap::Node>::EdgeIterator it = edges.begin(); it != edges.end(); ++it) {
              if (it->first == this && it->second->is_area()) return (AreaNode*)it->second;
              else if (it->second == this && it->first->is_area()) return (AreaNode*)it->first;
            }
            return NULL;
          }

          void to_dot(std::ostream& out);
          void to_graphml(std::ostream& out);

          OccupancyGrid* grid;
          Direction edge;
          uint x0, xf;
          uint reach_attempts;
          uint MAX_REACH_ATTEMPTS;
      };

      class AreaNode : public Node {
        public:
          AreaNode(OccupancyGrid* _grid) : Node(), grid(_grid), completely_explored(false) { }
          bool is_area(void) const { return true; }

          void to_dot(std::ostream& out);
          void to_graphml(std::ostream& out);

          OccupancyGrid* grid;
          bool completely_explored;
      };

      Graph<Node> graph;
  };
}

std::ostream& operator<<(std::ostream& out, const std::list<HybNav::TopoMap::Node*>& node);
std::ostream& operator<<(std::ostream& out, const HybNav::TopoMap::Node* node);
std::ostream& operator<<(std::ostream& out, const HybNav::TopoMap::AreaNode* node);
std::ostream& operator<<(std::ostream& out, const HybNav::TopoMap::GatewayNode* node);

#endif
