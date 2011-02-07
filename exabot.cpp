#include <gsl/gsl_sf_trig.h>
#include <iostream>
#include "metric_map.h"
#include "plotter.h"
#include "exabot.h"
#include "explorer.h"
#include "util.h"
using namespace HybNav;
using namespace std;

/**************************
 * Constructor/Destructor *
 **************************/

ExaBot::ExaBot(void) : player_client(), laser_proxy(&player_client),
  position_proxy(&player_client), target_position_proxy(&player_client, 1), planner_proxy(&player_client), simulator_proxy(&player_client)
{
  laser_proxy.RequestGeom();
  position_proxy.RequestGeom();
  target_position_proxy.RequestGeom();

  // initialize plotter
  new Plotter;
  *Plotter::instance() << "set term x11 0 noraise persist";
  *Plotter::instance() << "set term x11 1 noraise persist";
  *Plotter::instance() << "set term x11 2 noraise persist";

  player_client.Read();

  // initialize variables
  double dummy;
  last_position = gsl::vector(2, true);
  get_pose(last_position, dummy);
  
  initial_position = last_position;
  trajectory_length = 0;

  graph_timer = std::time(NULL);

  // enable planner
}

ExaBot::~ExaBot(void) {
  cout << "destroying plotter" << endl;
  delete Plotter::instance();
}

/**************************
 *     Public Methods     *
 **************************/

void ExaBot::update(void) {
  try {
    player_client.Read();
    if (position_proxy.GetStall()) throw std::runtime_error("collision detected!");

    update_position();

    MetricMap::instance()->process_distances(position_proxy, laser_proxy);
    Explorer::instance()->update();
    Explorer::instance()->compute_motion(position_proxy, planner_proxy);

    // Graphics
    cout << "timer " << (std::time(NULL) - graph_timer) << endl;
    if ((std::time(NULL) - graph_timer) >= 1) {
      graph_timer = std::time(NULL);
      Plotter& p = *Plotter::instance();
      p << "set term x11 0";
      p << "set xrange [0:" + to_s(Place::CELLS) + "]";
      p << "set yrange [0:" + to_s(Place::CELLS) + "]";
      //p << "set xrange [0:" + to_s(MetricMap::WINDOW_CELLS) + "]";
      //p << "set yrange [0:" + to_s(MetricMap::WINDOW_CELLS) + "]";
      p << "set xtics 1; set ytics 1; set grid front; unset key";
      p << "set palette gray negative";
      p << "set cbrange [" + to_s(Place::Lfree) + ":" + to_s(Place::Locc) + "]";

      list<Plot> plots;

      Plot locc_plot(MetricMap::instance()->current_node->place.occupancy_matrix, "image", "flipy origin=(0.5,0.5)");
      //Plot locc_plot(MetricMap::instance()->window, "image", "flipy origin=(0.5,0.5)");
      plots.push_back(locc_plot);

      gsl::vector_int robot_position = MetricMap::instance()->grid_position();
      gsl::matrix robot_position_mat(1, 2);
      robot_position_mat(0, 0) = robot_position(0) + 0.5; robot_position_mat(0, 1) = robot_position(1) + 0.5;
      Plot pos_plot(robot_position_mat, "points");
      plots.push_back(pos_plot);

      gsl::matrix path_data;
      if (!LocalExplorer::instance()->follow_path.empty()) {
        list<gsl::vector_int>& path = LocalExplorer::instance()->follow_path;
        path_data = gsl::matrix(path.size(), 2);
        size_t i = 0;
        for (list<gsl::vector_int>::iterator it = path.begin(); it != path.end(); ++it, i++) {
          path_data(i, 0) = (*it)(0); path_data(i, 1) = (*it)(1);
        }
        plots.push_back(Plot(path_data, "linespoints"));
      }

      MetricMap::Node::FrontierList& frontiers = MetricMap::instance()->current_node->frontiers;
      gsl::matrix frontiers_data;
      if (!frontiers.empty()) {
        frontiers_data = gsl::matrix(frontiers.size(), 2);
        size_t i = 0;
        for (MetricMap::Node::FrontierList::iterator it = frontiers.begin(); it != frontiers.end(); ++it, i++) {
          frontiers_data(i, 0) = (*it)(0) + 0.5; frontiers_data(i, 1) = (*it)(1) + 0.5;
        }
        plots.push_back(Plot(frontiers_data, "points"));
      }

      p.plot(plots);
      /*if (@delta_position != Vector.zero(2))
        @positions_log << absolute_position.to_a
      end*/
    }
  }
  catch(const PlayerCc::PlayerError& err) {
    cout << "player error!" << endl;
    throw;
  }
}

void ExaBot::deinitialize(void) {
  //puts "Trajectory Length: %.2f" % @trajectory_length
  //@position_proxy.SetSpeed(0, 0)
  cout << "Saving Map..." << endl;
  MetricMap::instance()->save();
  TopoMap::instance()->save();
}


/**************************
 *    Private Methods     *
 **************************/

void ExaBot::get_pose(gsl::vector& absolute_position, double& absolute_rotation) {
  absolute_position(0) = position_proxy.GetXPos();
  absolute_position(1) = position_proxy.GetYPos();

  absolute_rotation = gsl_sf_angle_restrict_pos(position_proxy.GetYaw());
  cout << "Position: " << absolute_position(0) << "," << absolute_position(1) << " Yaw: " << absolute_rotation << endl;
}

void ExaBot::update_position(void) {
  gsl::vector absolute_position(2);
  double absolute_rotation;
  
  get_pose(absolute_position, absolute_rotation);
  
  gsl::vector delta_position = absolute_position;
  delta_position -= last_position;
  trajectory_length += delta_position.norm2();

  double delta_rotation = gsl_sf_angle_restrict_symm(absolute_rotation - last_rotation);

  MetricMap::instance()->update_position(delta_position, delta_rotation);
  last_position = absolute_position;
  last_rotation = absolute_rotation;

  /*guessed_position = (MetricMap.instance.position - Place::SIZE * 0.5 + MetricMap.instance.current_node.position * Place::SIZE)
  error = absolute_position - @initial_position - guessed_position
  puts "ERROR IN POSITION: #{error.norm} : #{absolute_position - @initial_position} #{guessed_position}"
  error = absolute_rotation - @initial_rotation - MetricMap.instance.rotation * (Math::PI / 180.0)
  puts "ERROR IN ROTATION: #{Math.angle_restrict_symm(error)}"*/
}