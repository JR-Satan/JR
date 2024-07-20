#include <opencv2/opencv.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <stdexcept>

// 读取CSV文件的函数
std::vector<std::vector<std::string>> readCSV(const std::string& filename) {
    std::vector<std::vector<std::string>> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Error opening file");
    }
    std::string line, cell;
    while (std::getline(file, line)) {
        std::vector<std::string> row;
        std::stringstream lineStream(line);
        while (std::getline(lineStream, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row);
    }
    return data;
}


// 缩放图像的函数
cv::Mat scaleImage(const cv::Mat& img, double scaleX, double scaleY, const cv::Point2f& center, const std::string& interpolation) 
{
    cv::Mat scaled;
    cv::Size outputSize(static_cast<int>(scaleX), static_cast<int>(scaleY));
    int interp_code = (interpolation == "NEAREST") ? cv::INTER_NEAREST : cv::INTER_LINEAR;
    cv::resize(img, scaled, outputSize, 0, 0, interp_code);
    return scaled;
}

// 平移图像的函数
cv::Mat translateImage(const cv::Mat& img, int tx, int ty) 
{
    cv::Mat translated;
    cv::Mat translationMatrix = (cv::Mat_<double>(2, 3) << 1, 0, tx, 0, 1, ty);
    cv::warpAffine(img, translated, translationMatrix, img.size(), cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    return translated;
}

// 平移旋转图像的函数
cv::Mat rotateAndTranslateImage(const cv::Mat& img, double angle, const std::string& interpolation, const cv::Point2f& center) 
{
    cv::Mat rotated;
    cv::Mat rotationMatrix = cv::getRotationMatrix2D(center, angle, 1);
    int interp_code = (interpolation == "NEAREST") ? cv::INTER_NEAREST : cv::INTER_LINEAR;
    cv::warpAffine(img, rotated, rotationMatrix, img.size(), interp_code, cv::BORDER_CONSTANT, cv::Scalar(255, 255, 255));
    return rotated;
}

int main(int argc, char** argv)
{
    // 读取CSV文件
    std::vector<std::vector<std::string>> csvData = readCSV("experiment1.csv");

    // 遍历CSV文件中的每一行
    for (size_t i = 1; i < csvData.size(); ++i) 
    { // 从第2行开始，跳过表头

        std::string filename = csvData[i][0];
        double img_scaleX = std::stod(csvData[i][1]);
        double img_scaleY = std::stod(csvData[i][2]);
        std::string interpolation = csvData[i][3];
        int img_Horizontal = std::stoi(csvData[i][4]);
        int img_Vertical = std::stoi(csvData[i][5]);
        std::string Rotation_center=csvData[i][6];
        double rotation_angle = std::stod(csvData[i][7]);
    
        // 读取图像
        cv::Mat img = cv::imread(filename);
        if (img.empty()) 
        {
            std::cerr << "Error loading image: " << filename << std::endl;
            continue;
        }

        //原始图像的宽度和高度
        int Width = img.cols;
        int Height = img.rows;

        //计算原始图像的中心点
        cv::Point2f Center(Width / 2.0f, Height / 2.0f);

        // 进行缩放变换
         cv::Mat scaledImg = scaleImage(img, img_scaleX, img_scaleY, Center, interpolation);

        // 进行平移变换
        cv::Mat translatedImg = translateImage(scaledImg, img_Horizontal, img_Vertical);

        // 获取缩放和平移后图像的宽度和高度
        int scaledWidth = translatedImg.cols;
        int scaledHeight = translatedImg.rows;

       // 计算变换后的图像尺寸
        double cosTheta = std::cos(rotation_angle * CV_PI / 180.0);
        double sinTheta = std::sin(rotation_angle * CV_PI / 180.0);
        int newWidth = static_cast<int>(std::abs(cosTheta * scaledWidth) + std::abs(sinTheta * scaledHeight));
        int newHeight = static_cast<int>(std::abs(sinTheta * scaledWidth) + std::abs(cosTheta * scaledHeight));


        // 计算平移后图像的新中心点
        cv::Point2f newCenter;
        if(Rotation_center == "center")
        {    
            newCenter = cv::Point2f(img.cols / 2.0, img.rows / 2.0);
        }
        else if(Rotation_center == "origin")
        {    
            newCenter = cv::Point2f(0, 0);
        }
        
        //进行旋转变换
        cv::Mat processedImg = rotateAndTranslateImage(translatedImg, rotation_angle, interpolation, newCenter);
    
        // 显示结果
        cv::imshow("Original Image", img);
        cv::imshow("scaled Image", scaledImg);
        cv::imshow("translated Image", translatedImg);
        cv::imshow("Processed Image", processedImg); 

        // 保存结果
        std::string outputFilename = "processed_" + filename;
        cv::imwrite(outputFilename, processedImg);

        cv::waitKey(0); // 等待按键以查看每个图像
    }
    return 0;
}
