#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <pcl_conversions/pcl_conversions.h>
#include <pcl/point_types.h>
#include <pcl/PCLPointCloud2.h>
#include <pcl/conversions.h>
#include <pcl/filters/voxel_grid.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/registration/gicp.h>
#include <pcl/io/pcd_io.h>
#include <functional>
#include <algorithm>
#include <memory>
#include <cmath>
#include <Eigen/Dense>

const auto pi = 3.141592;
auto roughness_count = 10; // odd
auto edge_crit = -100;
auto planar_crit = 0.001;
pcl::PointXYZI origin;
pcl::PointCloud<pcl::PointXYZI>::Ptr edge_map(new pcl::PointCloud<pcl::PointXYZI>());
pcl::PointCloud<pcl::PointXYZI>::Ptr ex_planar(new pcl::PointCloud<pcl::PointXYZI>());
pcl::PointCloud<pcl::PointXYZI>::Ptr ex_noise(new pcl::PointCloud<pcl::PointXYZI>());
pcl::KdTreeFLANN<pcl::PointXYZI> kdtree;
int K = 1;
int cycle = 0;
std::vector<int> pointIdxNKNSearch(K);
std::vector<float> pointNKNSquaredDistance(K);
float x =0;
float y = 0;
float z = 0;
float deg = 0;


float distance(pcl::PointXYZI point1, pcl::PointXYZI point2);
bool compare(const pcl::PointXYZI &p1, const pcl::PointXYZI &p2);
pcl::PointCloud<pcl::PointXYZI> cal_roughness(pcl::PointCloud<pcl::PointXYZI> layer);

class Sub : public rclcpp::Node
{
public:
    Sub() : Node("sub")
    {
        auto qos_profile_ = rclcpp::QoS(rclcpp::KeepLast(10));
        sub_ = this->create_subscription<sensor_msgs::msg::PointCloud2>(
                "/velodyne_points",
                qos_profile_,
                std::bind(&Sub::callback, this, std::placeholders::_1));

        pub_e = this->create_publisher<sensor_msgs::msg::PointCloud2>("edge", qos_profile_);
        pub_p = this->create_publisher<sensor_msgs::msg::PointCloud2>("planar", qos_profile_);
        pub_n = this->create_publisher<sensor_msgs::msg::PointCloud2>("noise", qos_profile_);
        pub_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("rough", qos_profile_);
        pub_pos = this->create_publisher<sensor_msgs::msg::PointCloud2>("pos", qos_profile_);
        pub_map = this->create_publisher<sensor_msgs::msg::PointCloud2>("map", qos_profile_);
    }

private:
    rclcpp::Subscription<sensor_msgs::msg::PointCloud2>::SharedPtr sub_;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_e;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_p;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_n;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_pos;
    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr pub_map;
    void callback(const sensor_msgs::msg::PointCloud2::SharedPtr msg) const
    {
        pcl::PointCloud<pcl::PointXYZI>::Ptr cloud_dst(new pcl::PointCloud<pcl::PointXYZI>);
        pcl::fromROSMsg(*msg, *cloud_dst);

        auto width = cloud_dst->height;
        auto height = cloud_dst->width;
        auto size = cloud_dst->size();
        pcl::VoxelGrid<pcl::PointXYZI> sor;
        sor.setLeafSize(0.1, 0.1, 0.1);
        pcl::PointCloud<pcl::PointXYZI> cal_pc;
        cal_pc.reserve(width * height);

        std::map<int, pcl::PointCloud<pcl::PointXYZI>::Ptr> ring_pointcloud_map;

        sensor_msgs::PointCloud2Iterator<float> iter_x(*msg, "x");
        sensor_msgs::PointCloud2Iterator<float> iter_y(*msg, "y");
        sensor_msgs::PointCloud2Iterator<float> iter_z(*msg, "z");
        sensor_msgs::PointCloud2Iterator<float> iter_intensity(*msg, "intensity");
        sensor_msgs::PointCloud2Iterator<uint16_t> iter_ring(*msg, "ring");

        for (; iter_x != iter_x.end(); ++iter_x, ++iter_y, ++iter_z, ++iter_intensity, ++iter_ring)
        {
            int ring_value = *iter_ring;
            if (ring_pointcloud_map.find(ring_value) == ring_pointcloud_map.end())
            {
                ring_pointcloud_map[ring_value] = pcl::PointCloud<pcl::PointXYZI>::Ptr(new pcl::PointCloud<pcl::PointXYZI>());
            }

            pcl::PointXYZI point;
            point.x = *iter_x;
            point.y = *iter_y;
            point.z = *iter_z;
            point.intensity = *iter_intensity;
            ring_pointcloud_map[ring_value]->points.push_back(point);
        }

        for (auto& pair : ring_pointcloud_map)
        {
            int ring = pair.first;
            pcl::PointCloud<pcl::PointXYZI>::Ptr cloud = pair.second;
            cloud->width = cloud->points.size();
            cloud->height = 1;
            cloud->is_dense = false;
            // RCLCPP_INFO(this->get_logger(), "Ring %d has %zu points", ring, cloud->points.size());

            sor.setInputCloud(cloud);
            // RCLCPP_INFO(this->get_logger(), "%d", cloud->size());
            pcl::PointCloud<pcl::PointXYZI> voxel_layer;
            sor.filter(voxel_layer);
            std::sort(voxel_layer.begin(), voxel_layer.end(), compare);
            
            auto cal_layer = cal_roughness(voxel_layer);
            cal_pc.insert(cal_pc.end(), cal_layer.begin(), cal_layer.end());
            // 여기서 각 cloud를 원하는 대로 처리하거나 사용합니다.
        }

        // intensity 값에 따라 분류할 PointCloud들
        pcl::PointCloud<pcl::PointXYZI>::Ptr edge(new pcl::PointCloud<pcl::PointXYZI>());
        pcl::PointCloud<pcl::PointXYZI>::Ptr planar(new pcl::PointCloud<pcl::PointXYZI>());
        pcl::PointCloud<pcl::PointXYZI>::Ptr noise(new pcl::PointCloud<pcl::PointXYZI>());

        // cal_pc의 각 점을 intensity 값에 따라 분류
        for (const auto& point : cal_pc)
        {
            if (point.intensity >= edge_crit)
            {
                edge->points.push_back(point);
            }
            else if( (point.intensity < planar_crit) && (point.intensity > 0))
            {
                planar->points.push_back(point);
            }
            else
            {
                noise->points.push_back(point);
            }
        }

        RCLCPP_INFO(this->get_logger(), "edge : %d, planar : %d, noise : %d", edge->size(),planar->size(),noise->size());

        sensor_msgs::msg::PointCloud2 edge_msg;
        sensor_msgs::msg::PointCloud2 planar_msg;
        sensor_msgs::msg::PointCloud2 noise_msg;
        sensor_msgs::msg::PointCloud2 rough_msg;
        sensor_msgs::msg::PointCloud2 map_msg;
        sensor_msgs::msg::PointCloud2 pos_msg;

        pcl::toROSMsg(*edge, edge_msg);
        edge_msg.header.frame_id = "map";
        pub_e->publish(edge_msg);

        pcl::toROSMsg(*planar, planar_msg);
        planar_msg.header.frame_id = "map";
        pub_p->publish(planar_msg);

        pcl::toROSMsg(*noise, noise_msg);
        noise_msg.header.frame_id = "map";
        pub_n->publish(noise_msg);

        pcl::toROSMsg(cal_pc, rough_msg);
        rough_msg.header.frame_id = "map";
        pub_->publish(rough_msg);

        cycle += 1;

        if(cycle > 1)
        {
            pcl::GeneralizedIterativeClosestPoint<pcl::PointXYZI, pcl::PointXYZI> gicp;
            gicp.setMaximumIterations(5); // 최대 반복 횟수 설정
            gicp.setTransformationEpsilon(1e-5); // 변환 행렬 오차
            gicp.setMaxCorrespondenceDistance(0.05); // 최대 대응 거리
            gicp.setInputSource(edge);
            gicp.setInputTarget(edge_map);

            pcl::PointCloud<pcl::PointXYZI> final_cloud;
            gicp.align(final_cloud);

            if (gicp.hasConverged()) {
                std::cout << "GICP has converged with score: " << gicp.getFitnessScore() << std::endl;
                
                auto transform = gicp.getFinalTransformation();
                Eigen::Vector4f local_origin(0, 0, 0, 1);

                

                Eigen::Vector4f global_origin = transform * local_origin;

                pcl::PointXYZI global_origin_point;
                global_origin_point.x = global_origin(0);
                global_origin_point.y = global_origin(1);
                global_origin_point.z = global_origin(2);
                global_origin_point.intensity = 1.0; // 필요에 따라 적절히 설정

                pcl::PointCloud<pcl::PointXYZI>::Ptr origin_cloud(new pcl::PointCloud<pcl::PointXYZI>);
                origin_cloud->width = 1;
                origin_cloud->height = 1;
                origin_cloud->is_dense = false;
                origin_cloud->points.push_back(global_origin_point);

                std::cout << "Transformation matrix: \n" << global_origin << std::endl;


                pcl::toROSMsg(*origin_cloud, pos_msg);
                pos_msg.header.frame_id = "global";
                pub_pos->publish(pos_msg);

            } else {
                std::cout << "GICP did not converge." << std::endl;
            }
            pcl::toROSMsg(*edge_map, map_msg);
            map_msg.header.frame_id = "global";
            map_msg.header.stamp = this->now(); // 현재 시간으로 설정
            pub_map->publish(map_msg);
        }




    }
};

float distance(pcl::PointXYZI point1, pcl::PointXYZI point2)
{
    return std::pow(std::pow(point1.x - point2.x, 2) + std::pow(point1.y - point2.y, 2) + std::pow(point1.z - point2.z, 2), 0.5);
}

bool compare(const pcl::PointXYZI &p1, const pcl::PointXYZI &p2)
{
    auto angle1 = std::atan2(p1.x, p1.y);
    auto angle2 = std::atan2(p2.x, p2.y);
    if(angle1 < angle2)
    {
        return true;
    }
    else{
        return false;
    }
}

pcl::PointCloud<pcl::PointXYZI> cal_roughness(pcl::PointCloud<pcl::PointXYZI> layer)
{   
    auto width = layer.size();
    // pcl::PointCloud<pcl::PointXYZI> roughed_layer;
    // roughed_layer.resize(width);

    if (width > roughness_count)
    {   
        // easily compute
        for(unsigned int count = 0; count < roughness_count; ++count)
        {
            layer.push_back(layer.at(count));
        }
        //compute roughness
        for(unsigned int idx = roughness_count/2; idx < width + roughness_count/2; ++idx)
        {   
            float sum = 0.0;
            for(unsigned int i = 1; i <= roughness_count/2; ++i)
            {   
                sum += (distance(layer.at(idx), layer.at(idx + i)) + distance(layer.at(idx), layer.at(idx - i)));
            }
            auto roughness = sum / (distance(layer.at(idx), origin) * roughness_count);
            // auto roughness = sum / roughness_count;

            if (std::isnan(roughness) || std::isinf(roughness))
            {
                layer.at(idx).intensity = -1;
            }
            else
            {
                layer.at(idx).intensity = roughness;
            }
        }
        for(unsigned int i = 0 ; i < roughness_count/2 ; ++i)
        {
            layer.at(i).intensity = layer.at(width + i).intensity;
        }
        // exception
        auto resolution = 2 * pi/1024;
        for(unsigned int idx = 1; idx <= width ; ++idx)
        {   
            if (layer.at(idx).intensity > edge_crit)
            {   
                if(layer.at(idx + 1).intensity > edge_crit)
                {   
                    auto dis = distance(layer.at(idx), origin);
                    auto gap = dis - distance(layer.at(idx + 1), origin);
                    if(gap > dis * tan(resolution) * 2)
                    {
                        layer.at(idx).intensity = -1;
                    }
                }
                if(layer.at(idx -1 ).intensity > edge_crit)
                {   
                    auto dis = distance(layer.at(idx), origin);
                    auto gap = dis - distance(layer.at(idx - 1), origin);
                    if(gap > dis * tan(resolution) * 2)
                    {
                        layer.at(idx).intensity = -1;
                    }
                }
            }
        }
    }

    else
    {
        for(int i=0; i<width; ++i)
        {
            layer.at(i).intensity = -1;
        }
    }
    layer.resize(width);

    // for(int i=0; i<width; ++i)
    // {
    //     layer.at(i).intensity = i;
    // }

    return layer;

}


int main(int argc, char *argv[])
{   
    origin.x = 0;
    origin.y = 0;
    origin.z = 0;
    origin.intensity = 0;

    Eigen::Matrix4f initial;
    initial << 1, 0, 0, 0,
               0, 1, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1;
    pcl::io::loadPCDFile<pcl::PointXYZI>("/home/leesh/Downloads/service_LOAM/GlobalMap.pcd", *edge_map);
    rclcpp::init(argc, argv);
    auto node = std::make_shared<Sub>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
