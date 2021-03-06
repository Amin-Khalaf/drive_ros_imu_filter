#include "ros/ros.h"
#include <mutex>

#include "geometry_msgs/TransformStamped.h"
#include "geometry_msgs/Vector3.h"
#include "sensor_msgs/Imu.h"

#include "tf/tf.h"
#include "tf2/buffer_core.h"
#include "tf2_ros/transform_listener.h"
#include "drive_ros_msgs/tf2_IMU.h"

static std::string vehicle_frame;

ros::Publisher imu_pub;
tf2_ros::Buffer* tf2_buffer;
tf2_ros::TransformListener* tf2_listener;

static geometry_msgs::Vector3 current_gyro_off;

bool received_first_msg = false;


void imuCallback(const sensor_msgs::Imu::ConstPtr &msg)
{

  if(!received_first_msg)
  {
    ROS_INFO("Received first IMU message.");
    ROS_INFO("Wait 1s to let the broadcaster some time to publish.");
    ros::Duration d(1);
    d.sleep();
    received_first_msg = true;
  }

  // remove gyro offset
  sensor_msgs::Imu msg_gyro(*msg); // pretty expensive :(
  msg_gyro.angular_velocity.x -= current_gyro_off.x;
  msg_gyro.angular_velocity.y -= current_gyro_off.y;
  msg_gyro.angular_velocity.z -= current_gyro_off.z;

  // rotate
  try
  {
    sensor_msgs::Imu imu_out;
    tf2_buffer->transform(msg_gyro, imu_out, vehicle_frame);
    imu_pub.publish(imu_out);
  }
  catch (tf2::TransformException ex)
  {
    ROS_ERROR_STREAM("IMU Transform failure: " << ex.what());
    return;
  }
}

// save gyro offset
void gyroOffCallback(const geometry_msgs::Vector3::ConstPtr &msg)
{
  current_gyro_off.x = msg->x;
  current_gyro_off.y = msg->y;
  current_gyro_off.z = msg->z;
}


int main(int argc, char **argv)
{
  ros::init(argc, argv, "imu_apply_transform");
  ros::NodeHandle nh;
  ros::NodeHandle pnh("~");

  // setup tf 2 stuff
  tf2_buffer = new tf2_ros::Buffer();
  tf2_listener = new tf2_ros::TransformListener(*tf2_buffer);


  // get target frame
  vehicle_frame = pnh.param<std::string>("vehicle_frame", "");
  ROS_INFO_STREAM("Loaded vehicle frame: " << vehicle_frame);

  // setup subscriber and publisher
  ros::Subscriber imu_sub = pnh.subscribe("imu_in", 10, imuCallback);
  ros::Subscriber gyr_off_sub = pnh.subscribe("gyro_off", 10, gyroOffCallback);
  imu_pub = pnh.advertise<sensor_msgs::Imu>("imu_out", 10);

  // forever loop
  ros::spin();

  return 0;
}
