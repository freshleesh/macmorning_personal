#include <pcl/io/pcd_io.h>
#include <pcl/point_types.h>
#include <pcl/visualization/pcl_visualizer.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <pcl/filters/voxel_grid.h>
#include <cmath>
#include <algorithm>
#include <pcl/kdtree/kdtree_flann.h>
#include <iostream>

const auto pi = 3.141592;
auto roughness_count = 10; // odd
auto edge_crit = 0.2;
auto planar_crit = 0.1;
pcl::PointXYZI origin;

float distance(pcl::PointXYZI point1, pcl::PointXYZI point2);
bool compare(const pcl::PointXYZI &p1, const pcl::PointXYZI &p2);
pcl::PointCloud<pcl::PointXYZI> cal_roughness(pcl::PointCloud<pcl::PointXYZI>);

int main(int argc, char** argv) {
    origin.x = 0;
    origin.y = 0;
    origin.z = 0;
    origin.intensity = 0;
    // PointCloud 객체 생성
    pcl::PointCloud<pcl::PointXYZI>::Ptr cloud(new pcl::PointCloud<pcl::PointXYZI>);

    // PCD 파일을 로드
    if (pcl::io::loadPCDFile<pcl::PointXYZI>("/home/leesh/Downloads/service_LOAM/GlobalMap.pcd", *cloud) == -1) {
        PCL_ERROR("Couldn't read file GlobalMap.pcd \n");
        return (-1);
    }

    // 불러온 PointCloud 정보 출력
    std::cout << "Loaded " << cloud->width * cloud->height << " data points from GlobalMap.pcd with the following fields: " << std::endl;

    // 포인트 클라우드의 Z 값 범위 계산
    float min_z = std::numeric_limits<float>::max();
    float max_z = -std::numeric_limits<float>::max();
    for (const auto& point : cloud->points) {
        if (point.z < min_z) min_z = point.z;
        if (point.z > max_z) max_z = point.z;
    }

    // Z 값을 기준으로 0.1m 간격으로 나누기 위한 레이어 계산
    float interval = 0.1;
    int num_layers = static_cast<int>((max_z - min_z) / interval) + 1;

    // 각 레이어를 저장할 벡터 생성
    std::unordered_map<int, pcl::PointCloud<pcl::PointXYZI>::Ptr> layers;
    for (int i = 0; i < num_layers; ++i) {
        layers[i] = pcl::PointCloud<pcl::PointXYZI>::Ptr(new pcl::PointCloud<pcl::PointXYZI>);
    }

    // 포인트를 각 레이어에 할당
    for (const auto& point : cloud->points) {
        int layer_index = static_cast<int>((point.z - min_z) / interval);
        layers[layer_index]->points.push_back(point);
    }

    pcl::VoxelGrid<pcl::PointXYZI> sor;
    sor.setLeafSize(0.1, 0.1, 0.1);
    pcl::PointCloud<pcl::PointXYZI> cal_pc;

    // 각 레이어의 포인트 클라우드 출력
    for (int i = 0; i < num_layers; ++i) {
        auto selected_layer = layers[i];
        sor.setInputCloud(selected_layer);
        pcl::PointCloud<pcl::PointXYZI> voxel_layer;
        sor.filter(voxel_layer);
        std::sort(voxel_layer.begin(), voxel_layer.end(), compare);
        auto cal_layer = cal_roughness(voxel_layer);
        cal_pc.insert(cal_pc.end(), cal_layer.begin(), cal_layer.end());

        // for( int i=0; i<cal_pc.size(); ++i){
        //     cal_pc.at(i).intensity = i;
        // }


        std::cout << "Layer " << i << " has " << cal_layer.points.size() << " points." << std::endl;
    }

    //cal_pc의 각 점을 intensity 값에 따라 분류
    for (auto& point : cal_pc)
    {
        if (point.intensity >= edge_crit)
        {
            point.intensity = 100;
        }
        else if( (point.intensity < planar_crit) && (point.intensity > 0))
        {
            point.intensity = 50;
        }
        else
        {
            point.intensity = 0;
        }
    }

    // PointCloud 시각화
    pcl::visualization::PCLVisualizer::Ptr viewer(new pcl::visualization::PCLVisualizer("Layered Cloud Viewer"));

    // 레이어마다 다른 색상으로 포인트 클라우드를 추가
    pcl::visualization::PointCloudColorHandlerGenericField<pcl::PointXYZI> intensity_distribution(cal_pc.makeShared(), "intensity");
    viewer->addPointCloud<pcl::PointXYZI>(cal_pc.makeShared(), intensity_distribution, "cal_pc");


    // Viewer가 닫힐 때까지 기다림
    viewer->setBackgroundColor(0, 0, 0); // 배경 색을 검은색으로 설정
    viewer->addCoordinateSystem(1.0); // 좌표계를 추가
    viewer->initCameraParameters(); // 카메라 파라미터 초기화

    while (!viewer->wasStopped()) {
        viewer->spinOnce(100);
    }

    return 0;
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

float distance(pcl::PointXYZI point1, pcl::PointXYZI point2)
{
    return std::pow(std::pow(point1.x - point2.x, 2) + std::pow(point1.y - point2.y, 2) + std::pow(point1.z - point2.z, 2), 0.5);
}

pcl::PointCloud<pcl::PointXYZI> cal_roughness(pcl::PointCloud<pcl::PointXYZI> layer)
{   
    auto width = layer.size();
    if (width > roughness_count * 2)
    {   
        // KdTree 객체 생성
        pcl::PointCloud<pcl::PointXYZI>::Ptr cloudPtr(new pcl::PointCloud<pcl::PointXYZI>(layer));
        pcl::KdTreeFLANN<pcl::PointXYZI> kdtree;
        kdtree.setInputCloud(cloudPtr);

        // compute roughness
        for(unsigned int idx = 0; idx < width ; ++idx)
        {   
            float sum = 0.0;
            int K = 10;
            std::vector<int> pointIdxNKNSearch(K);
            std::vector<float> pointNKNSquaredDistance(K);

            int numNeighborsFound = kdtree.nearestKSearch(layer.at(idx), K, pointIdxNKNSearch, pointNKNSquaredDistance);

            if (numNeighborsFound > 0) {
                for (int i = 0; i < numNeighborsFound; ++i) {
                    sum += pointNKNSquaredDistance[i];
                }
            }

            auto roughness = sum / (distance(layer.at(idx), origin) * roughness_count);

            if (std::isnan(roughness) || std::isinf(roughness)) {
                layer.at(idx).intensity = -1;
            } else {
                layer.at(idx).intensity = roughness;
            }
        }

        // exception
        auto resolution = 2 * pi / 1024;
        for(unsigned int idx = 1; idx < width; ++idx)
        {   
            if (layer.at(idx).intensity > edge_crit)
            {   
                int K = 2;
                std::vector<int> pointIdxNKNSearch(K);
                std::vector<float> pointNKNSquaredDistance(K);
                
                int numNeighborsFound = kdtree.nearestKSearch(layer.at(idx), K, pointIdxNKNSearch, pointNKNSquaredDistance);

                if (numNeighborsFound > 0) {
                    for (int i = 0; i < numNeighborsFound; ++i) {
                        if (layer.points[pointIdxNKNSearch[i]].intensity > edge_crit) {   
                            auto dis = distance(layer.at(idx), origin);
                            auto gap = dis - distance(layer.points[pointIdxNKNSearch[i]], origin);
                            if (gap > dis * tan(resolution) * 2) {
                                layer.at(idx).intensity = -1;
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        for(int i = 0; i < width; ++i)
        {
            layer.at(i).intensity = -1;
        }
    }

    return layer;
}