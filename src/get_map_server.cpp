#include <ros/ros.h>
#include <map>
#include <std_msgs/Float64.h>
#include <nav_msgs/GetMap.h>
#include <nav_msgs/OccupancyGrid.h>
#include <message_filters/subscriber.h>
#include <boost/thread.hpp>

using namespace ros;

nav_msgs::GetMap::Response map_;
nav_msgs::GetMap::Response binary_map_;
boost::mutex map_mutex;
bool got_map;
int g_obstacle_threshold;
// todo could make changes on map(ROI, replace value...)here
void mapUpdate(const nav_msgs::OccupancyGrid::ConstPtr &map)
{
	ROS_DEBUG("Updating a map");
	boost::mutex::scoped_lock map_lock (map_mutex);

	binary_map_.map.info.width = map_.map.info.width = map->info.width;
	binary_map_.map.info.height =map_.map.info.height = map->info.height;
	binary_map_.map.info.resolution = map_.map.info.resolution = map->info.resolution;

	binary_map_.map.info.map_load_time = map_.map.info.map_load_time = map->info.map_load_time;

	binary_map_.map.info.origin.position.x = map_.map.info.origin.position.x = map->info.origin.position.x;
	binary_map_.map.info.origin.position.y = map_.map.info.origin.position.y = map->info.origin.position.y;
	binary_map_.map.info.origin.position.z = map_.map.info.origin.position.z = map->info.origin.position.z;

	binary_map_.map.info.origin.orientation.x = map_.map.info.origin.orientation.x = map->info.origin.orientation.x;
	binary_map_.map.info.origin.orientation.y = map_.map.info.origin.orientation.y = map->info.origin.orientation.y;
	binary_map_.map.info.origin.orientation.z = map_.map.info.origin.orientation.z = map->info.origin.orientation.z;
	binary_map_.map.info.origin.orientation.w = map_.map.info.origin.orientation.w = map->info.origin.orientation.w;

	map_.map.data.resize(map->info.width*map->info.height);
	binary_map_.map.data.resize(map->info.width*map->info.height);

	std::map<int, int> mp;
    /**
     * binary_map_ ： 100 --> OBSTACLE
     *               0 --> FREE/UNKNOWN
     * triple_map_ : 0 --> FREE
     *               100 --> OBSTACLE
     *               -1 --> UNKNOWN
     */
	for(int i=0; i<map->data.size(); i++) {
        mp[map->data[i]]++;
        if(map->data[i] == -1) {
            map_.map.data[i] = map->data[i];
			binary_map_.map.data[i] = 0;
		} else if(map->data[i] <= g_obstacle_threshold) {
            map_.map.data[i] = 0;
			binary_map_.map.data[i] = 0;
		} else {
            map_.map.data[i] = 100;
			binary_map_.map.data[i] = 100;

		}
    }

	binary_map_.map.header.stamp = map_.map.header.stamp = map->header.stamp;
	binary_map_.map.header.frame_id = map_.map.header.frame_id = map->header.frame_id;
/*
	ROS_INFO("Got map!");
	for(std::map<int, int>::iterator it=mp.begin(); it!=mp.end(); it++)
		ROS_INFO("%d, %d", it->first, it->second);
*/
	got_map = true;
}


bool mapCallback(nav_msgs::GetMap::Request  &req, 
		 nav_msgs::GetMap::Response &res)
{
	boost::mutex::scoped_lock map_lock (map_mutex);
	if(got_map && map_.map.info.width && map_.map.info.height)
	{
		res = map_;
		return true;
	}
	else
		return false;
}

bool binary_mapCallback(nav_msgs::GetMap::Request  &req,
				 nav_msgs::GetMap::Response &res)
{
	boost::mutex::scoped_lock map_lock (map_mutex);
	if(got_map && binary_map_.map.info.width && binary_map_.map.info.height)
	{
		res = binary_map_;
		return true;
	}
	else
		return false;
}


int main(int argc, char **argv)
{
	init(argc, argv, "map_server");
	NodeHandle n;
    ros::NodeHandle private_nh_("~");
    int obstacle_value = 0;
	std::string map_topic_name;
    private_nh_.param<int>("LETHAL_OBSTACLE", obstacle_value, 80);
	private_nh_.param<std::string>("map_topic_name", map_topic_name, "/cover_map");
    g_obstacle_threshold = obstacle_value;

	Subscriber sub = n.subscribe(map_topic_name, 10, mapUpdate);

	ServiceServer ss = n.advertiseService("current_map", mapCallback);

	ServiceServer sss = n.advertiseService("binary_map", binary_mapCallback);


	spin();

	return 0;
}
