/**
   J5 Example ROS Node
   @file j5_example_node.cpp

	This program is an example for commanding velocities and reading data from 
	the J5 ROS network.  The J5 ROS example node publishes geometry_msgs::Twist messages
	as velocity commands, and receives a custom j5StatusMsg message for status health
	feedback.
	
	Velocity commands should be published on topic "/j5_cmd", with a frequency of at least 10Hz.
	The parameters should be set as follows:
	
		-	linear.x = forward commanded velocity in meters/second
		-	angular.z = angular rotational velocity in rads/second
		- 	all other parameters are ignored
		
	The J5 will attempt to perform the commanded motion, but does not guarantee that the motion
	will be exactly achieved
	
	Vehicle status (j5StatusMsg) can be read on the topic "/j5_status", with following parameters 
	
		-	hdr: standard ROS header
		-	externalControl: indicates if the J5 is being controlled via the direct connection or the Futaba handheld device
		-	contactors: True if the line contactors are closed, false otherwise
		-	fault: True if a fault is detected on the J5, false otherwise
		-	voltage: the supply voltage in volts

	The master ROS node runs on the J5 RCU computer at --> ROS_MASTER_URI=http://192.168.0.20:11311
	You may also have to set the environment variable ROS_IP=*YOUR IP HERE* to connect

	This program expects C++11 support in the compiler.  If it isn't available, you can change the
	std::stof commands in the 'getCommandMsg' function to use the atof C functions

   @author Luc Brunet (lbrunet@provectus-robotics.com)
   @par Creation Date: 2015-01-17

   @par Copyrights:
   Copyright (c) 2015, Provectus Robotics Solutions Inc.
   All rights reserved.
 */

/* =========================================================================
 * INCLUDES
 * ========================================================================= */

#include <ros/ros.h>
#include <j5_msgs/j5StatusMsg.h>
#include <geometry_msgs/Twist.h>

#include <string>
#include <vector>

/* =========================================================================
 * CONSTS
 * ========================================================================= */

/// ROS node name
const std::string ComponentName = "j5_interface";

/// ROS topics --> do not change these
const std::string J5CommandTopic	= "/j5_cmd";
const std::string J5StatusTopic		= "/j5_status";

/// default forward linear velocity command in m/s
const float DefaultVelocityCmd = 0.0;

/// default angular velocity command in rad/s
const float DefaultTurnRateCmd = 0.0;

/// the rate at which the component will publish commands
const int LoopRate = 10; // Hz

/// input value limits
/// these are just here for safety and do not reflect the actual limits of the platform
const double MaxVelocityCommand = 3.0;
const double MaxTurnRateCommand = 1.0;

/* =========================================================================
 * FUNCTION IMPLEMENTATION
 * ========================================================================= */
 
/**
 * Status Message Callback
 * This function is triggered each time a new status message is received from the J5
 */
void statusMsgHandler(const j5_msgs::j5StatusMsg::ConstPtr& msg){

	ROS_INFO("RCV Status: EXT_CONTROL: %i FAULT: %i CONTACTORS: %i VOLTAGE: %.1f", msg->externalControl, msg->fault, msg->contactors, msg->voltage);
}

/**
 * Get Command Message
 * Returns the command message given the input arguments.  If the function
 * fails to parse the input, default parameters are used
 */
geometry_msgs::Twist getCommandMsg(const std::vector<std::string> &args){

   geometry_msgs::Twist msg;

   // body coordinates
   // x is forward linear motion
   msg.linear.x = DefaultVelocityCmd;

   // rotation around the Z axis
   msg.angular.z = DefaultTurnRateCmd;

   // all other parameters are ignored in the Twist message
   // NOTE:: args[0] is the name of the
   if(args.size() > 1)
   {
   	try
   	{
   		msg.linear.x = std::stof(args[1]);
   		msg.linear.x = std::max(msg.linear.x, -MaxVelocityCommand);
   		msg.linear.x = std::min(msg.linear.x, MaxVelocityCommand);
   	}
   	catch(...)
   	{
   	   // could not parse the input, use default value
   		msg.linear.x = DefaultVelocityCmd;
   	}
   }
   
   if(args.size() > 2)
   {
      try
   	{
   		msg.angular.z = std::stof(args[2]);
   		msg.angular.z = std::max(msg.angular.z, -MaxTurnRateCommand);
   		msg.angular.z = std::min(msg.angular.z, MaxTurnRateCommand);
   	}
   	catch(...)
   	{
   	   // could not parse the input, use default value
   		msg.angular.z = DefaultTurnRateCmd;
   	}
   }
   
   return msg;
}

/**
 * Main execution thread
 * Periodically publishes velocity commands to the J5 and prints out any
 * received data
 * 
 * Program should be run with two command line arguments:
 * arg1: velocity in meters/second	-	default = 0.0
 * arg2: turn rate in rads/second	-	default = 0.0
 *
 * ./j5_example_node linearVelocity angularVelocity
 */
int main(int argc, char **argv) {

   // get all command line arguments that aren't ROS specific 
   // these should be the velocity command parameters
   std::vector<std::string> args;
   ros::removeROSArgs(argc, argv, args);
	
   // initialize ROS
   ros::init(argc, argv, ComponentName);
   
   /// node handle
   ros::NodeHandle nh;
   
   // velocity command publisher
   ros::Publisher velCmdPub = nh.advertise<geometry_msgs::Twist>(J5CommandTopic, 1);
   
   // J5 status subscriber
   ros::Subscriber statusSub = nh.subscribe<j5_msgs::j5StatusMsg>(J5StatusTopic, 1, statusMsgHandler);
   
   // asynchronous spinner to receive ROS messages
   ros::AsyncSpinner spinner(0);
   
   // loop rate used to control the frequency of message publication
   ros::Rate loop_rate(LoopRate);
   
   // set up message command
   geometry_msgs::Twist cmdMsg = getCommandMsg(args);

   // run loop
   spinner.start();
   while (ros::ok())
   {
   	// publish velocity command
   	ROS_INFO("Sending Velocty Command: {%f, %f}", cmdMsg.linear.x, cmdMsg.angular.z);
   	velCmdPub.publish(cmdMsg);
		loop_rate.sleep();
   }

	// shutdown component
   spinner.stop();
   ros::shutdown();

   return 0;
}

