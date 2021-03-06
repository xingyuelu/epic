/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2015 Kyle Hollins Wray, University of Massachusetts
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *  the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef EPIC_NAVIGATION_NODE_HARMONIC_RVIZ_H
#define EPIC_NAVIGATION_NODE_HARMONIC_RVIZ_H


#include <ros/ros.h>

#include <nav_msgs/OccupancyGrid.h>
#include <geometry_msgs/PoseStamped.h>
#include <geometry_msgs/PoseWithCovarianceStamped.h>

#include <epic/harmonic/harmonic.h>

#include <epic/epic_navigation_node_harmonic.h>

// Generated by having the .srv files defined.
#include <epic/ModifyGoals.h>
#include <epic/SetCells.h>
#include <epic/ComputePath.h>

namespace epic {

class EpicNavigationNodeHarmonicRviz : public EpicNavigationNodeHarmonic {
public:
    /**
     *  The default constructor for the EpicNavigationNodeHarmonicRviz.
     *  @param  nh      The node handle from main.
     */
    EpicNavigationNodeHarmonicRviz(ros::NodeHandle &nh);

    /**
     *  The deconstructor for the EpicNavigationNodeHarmonicRviz.
     */
    virtual ~EpicNavigationNodeHarmonicRviz();

    /**
     *  Initialize the services, messages, and algorithm variables. Overridden from EpicNavigationNodeHarmonic.
     *  @return True if successful in registering, subscribing, etc.; false otherwise.
     */
    bool initialize();

protected:
    /**
     *  Handler for receiving PoseWithConvarianceStamped messages for the "/initialpose" topic,
     *  published by rviz by clicking the "2D Nav Goal" button.
     *  @param  msg     The PoseWithConvarianceStamped message.
     */
    void subMapPoseEstimate(const geometry_msgs::PoseWithCovarianceStamped::ConstPtr &msg);

    /**
     *  Handler for receiving PostStamped messages for the "/move_base_simple/goal" topic, published
     *  by rviz by clicking the "2D Nav Goal" button.
     *  @param  msg     The PoseStamped message.
     */
    void subMapNavGoal(const geometry_msgs::PoseStamped::ConstPtr &msg);

    // The subscriber for the "/initialpose" topic, published by rviz
    // by clicking the "2D Nav Goal" button.
    ros::Subscriber sub_map_pose_estimate;

    // The subscriber for the "/move_base_simple/goal" topic, published by rviz
    // by clicking the "2D Nav Goal" button.
    ros::Subscriber sub_map_nav_goal;

    // The publisher for the "path" topic, published to rviz.
    ros::Publisher pub_map_path;

    // If a goal has ever been added, however, only with the "subMapGoal" function.
    bool goal_added;

    // The last goal location of the robot. Used by "subMapGoal" function.
    // Assigned in rviz via "2D Nav Goal" button.
    geometry_msgs::PoseStamped last_goal;

    // The current pose of the robot. Used by "subMapGoal" function.
    // Assigned in rviz via "2D Pose Estimate" button.
    geometry_msgs::PoseStamped current_pose;

};

};


#endif // EPIC_NAVIGATION_NODE_HARMONIC_RVIZ_H

