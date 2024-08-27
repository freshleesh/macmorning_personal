#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/msg/point_cloud.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <geometry_msgs/msg/point32.hpp>
#include <std_msgs/msg/color_rgba.hpp>
#include <vector>
#include <cmath>
#include <thread>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_types.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/conversions.h>
#include <pcl/filters/voxel_grid.h>
#include "my_package/ThreadPool.hpp"


float distance(const std::vector<float>& point1, const std::vector<float>& point2);
pcl::PointCloud<pcl::PointXYZ> cloudmsg2cloud(sensor_msgs::msg::PointCloud2 cloudmsg);
sensor_msgs::msg::PointCloud2 cloud2cloudmsg(pcl::PointCloud<pcl::PointXYZ> cloud_src);
std::vector<std::vector<std::vector<float>>> cal_roughness(std::vector<std::vector<std::vector<float>>> scan_points, unsigned int start, unsigned int end);
const auto pi = 3.141592;
unsigned int width = 0;
unsigned int height = 0;
unsigned int roughness_count = 10; // odd
auto edge_crit = 5*std::pow(10, -1);
auto planar_crit = 3*std::pow(10, -2);

ThreadPool pool(4);

class Pointcloud2Handler : public rclcpp::Node {
public:
    Pointcloud2Handler() : Node("pointcloud2_handler") {
        subscription_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
            "/velodyne_points", 10, std::bind(&Pointcloud2Handler::pointCloudCallback, this, std::placeholders::_1));
        publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud>("processed/points", 10);
    }

private:
    void pointCloudCallback(const sensor_msgs::msg::PointCloud2::SharedPtr msg) const {
        std::cout << "3" << std::endl ;
        // 포인트 클라우드 데이터를 읽기 위한 이터레이터 생성
        sensor_msgs::PointCloud2ConstIterator<float> iter_x(*msg, "x");
        sensor_msgs::PointCloud2ConstIterator<float> iter_y(*msg, "y");
        sensor_msgs::PointCloud2ConstIterator<float> iter_z(*msg, "z");
        std::cout << "11" << std::endl ;
        width = msg->width;
        height = msg->height;
        unsigned int i = 0;
        int room = 0;
        int floor = 0;
        std::vector<std::vector<std::vector<float>>> scan_points(height, std::vector<std::vector<float>>(width, std::vector<float>(4, -1.0))); 
        std::cout << "12" << std::endl ;
        for (; iter_x != iter_x.end(); ++iter_x, ++iter_y, ++iter_z) 
        {
            room = i % width;
            floor = i / width;

            scan_points[floor][room][0] = *iter_x;
            scan_points[floor][room][1] = *iter_y;
            scan_points[floor][room][2] = *iter_z;

            i += 1;
        }
        std::vector<std::vector<std::vector<float>>> result; 
        std::vector<std::vector<std::vector<float>>> vec0, vec1, vec2, vec3; 

        std::cout << "13" << std::endl ;
        // 쓰레드를 저장할 벡터
        auto future0 = pool.enqueue(cal_roughness, scan_points, 0, 16);
        auto future1 = pool.enqueue(cal_roughness, scan_points, 16, 32);
        auto future2 = pool.enqueue(cal_roughness, scan_points, 32, 48);
        auto future3 = pool.enqueue(cal_roughness, scan_points, 48, 64);

        vec0 = future0.get();
        vec1 = future1.get();
        vec2 = future2.get();
        vec3 = future3.get();

        result.insert(result.end(), vec0.begin(), vec0.end());
        result.insert(result.end(), vec1.begin(), vec1.end());
        result.insert(result.end(), vec2.begin(), vec2.end());
        result.insert(result.end(), vec3.begin(), vec3.end());
        std::cout << "15" << std::endl ;
        // scan_points를 PointCloud 메시지로 변환하여 퍼블리시
        auto output_msg = std::make_shared<sensor_msgs::msg::PointCloud>();
        output_msg->header = msg->header;
        output_msg->points.resize(width * height);
        output_msg->channels.resize(1);
        output_msg->channels[0].name = "roughness";
        output_msg->channels[0].values.resize(width * height);
        std::cout << "16" << std::endl ;
        // unsigned int index = 0;
        // for (unsigned int f = 0; f < height; ++f) 
        // {
        //     for (unsigned int r = 0; r < width; ++r) 
        //     {
        //         geometry_msgs::msg::Point32 point;
        //         point.x = result[f][r][0];
        //         point.y = result[f][r][1];
        //         point.z = result[f][r][2];

        //         output_msg->points[index] = point;
                
        //         if (result[f][r][3] > edge_crit)
        //         {
        //             // output_msg->channels[0].values[index] = scan_points[f][r][3];
        //             output_msg->channels[0].values[index] = 100;
        //         }
        //         else if( (result[f][r][3] < planar_crit) && (result[f][r][3] > 0))
        //         {
        //             // output_msg->channels[0].values[index] = scan_points[f][r][3];
        //             output_msg->channels[0].values[index] = 50;
        //         }
        //         else
        //         {
        //             output_msg->channels[0].values[index] = 0;
        //         }
        //         ++index;
        //     }
        // }
        // std::cout << "17" << std::endl ;
        // publisher_->publish(*output_msg);
        // std::cout << "18" << std::endl ;
    }
    

    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr subscription_;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud>::SharedPtr publisher_;
};

float distance(const std::vector<float>& point1, const std::vector<float>& point2)
{
    return std::pow(std::pow(point1[0] - point2[0], 2) + std::pow(point1[1] - point2[1], 2) + std::pow(point1[2] - point2[2], 2), 0.5);
}

std::vector<std::vector<std::vector<float>>> cal_roughness(const std::vector<std::vector<std::vector<float>>> scan_points, unsigned int start, unsigned int end)
{   
    std::vector<std::vector<std::vector<float>>> return_vector; 
    for(unsigned int floor = start; floor < end; ++floor)
    {   
        auto layer = scan_points[floor];
        if (layer.size() > roughness_count)
        {   
            // easily compute
            for(unsigned int count = 0; count < roughness_count; ++count)
            {
                layer.push_back(layer[count]);
            }
            //compute roughness
            for(unsigned int idx = roughness_count/2; idx < width + roughness_count/2; ++idx)
            {   
                float sum = 0.0;
                for(unsigned int i = 1; i <= roughness_count/2; ++i)
                {   
                    sum += (distance(layer[idx], layer[idx + i]) + distance(layer[idx], layer[idx - i]));
                }
                auto roughness = sum / (distance(layer[idx], std::vector<float>(3, 0.0)) * roughness_count);

                if (std::isnan(roughness) || std::isinf(roughness))
                {
                    layer[idx][3] = -1;
                }
                else
                {
                    layer[idx][3] = roughness;
                }
            }
            for(unsigned int i = 0 ; i < roughness_count/2 ; ++i)
            {
                layer[i][3] = layer[width + i][3];
            }
            //exception
            auto resolution = 2 * pi/1024;
            for(unsigned int idx = 1; idx <= width ; ++idx)
            {   
                if (layer[idx][3] > edge_crit)
                {   
                    if(layer[idx+1][3] > edge_crit)
                    {   
                        auto dis = distance(layer[idx], std::vector<float>(3, 0.0));
                        auto gap = dis - distance(layer[idx+1], std::vector<float>(3, 0.0));
                        if(gap > dis * tan(resolution) * 2)
                        {
                            layer[idx][3] = -1;
                        }
                    }
                    if(layer[idx-1][3] > edge_crit)
                    {   
                        auto dis = distance(layer[idx], std::vector<float>(3, 0.0));
                        auto gap = dis - distance(layer[idx+1], std::vector<float>(3, 0.0));
                        if(gap > dis * tan(resolution) * 1)
                        {
                            layer[idx][3] = -1;
                        }
                    }
                }
            }
            layer[0][3] = layer[width][3];
        }
        layer.resize(width);
        return_vector.push_back(layer);
    }
    return return_vector;
}

pcl::PointCloud<pcl::PointXYZ> cloudmsg2cloud(sensor_msgs::msg::PointCloud2 cloudmsg)
  {
    pcl::PointCloud<pcl::PointXYZ> cloud_dst;
    pcl::fromROSMsg(cloudmsg, cloud_dst);
    return cloud_dst;
  }

sensor_msgs::msg::PointCloud2 cloud2cloudmsg(pcl::PointCloud<pcl::PointXYZ> cloud_src)
  {
    sensor_msgs::msg::PointCloud2 cloudmsg;
    pcl::toROSMsg(cloud_src, cloudmsg);
    cloudmsg.header.frame_id = "map";
    return cloudmsg;
  }

int main(int argc, char * argv[]) {
    std::cout << "1" << std::endl ;
    rclcpp::init(argc, argv);
    std::cout << "2" << std::endl ;
    rclcpp::spin(std::make_shared<Pointcloud2Handler>());
    std::cout << "4" << std::endl ;
    rclcpp::shutdown();
    std::cout << "5" << std::endl ;
    return 0;
}


