#include <opencv2/opencv.hpp>
#include <iostream>

// 针对图像的上半部分删除天空部分的函数
cv::Mat removeSky(const cv::Mat& image, int threshold_value) {
    // 获取图像尺寸
    int height = image.rows;
    int width = image.cols;

    // 只处理图像的上半部分
    cv::Rect upper_half_rect(0, 0, width, height / 2.5);
    cv::Mat upper_half = image(upper_half_rect);

    cv::Mat gray, binary, labels, stats, centroids;

    // 将上半部分转换为灰度图
    cv::cvtColor(upper_half, gray, cv::COLOR_BGR2GRAY);

    // 应用二值化处理
    cv::threshold(gray, binary, threshold_value, 255, cv::THRESH_BINARY);

    // 找到所有连通域
    int num_labels = cv::connectedComponentsWithStats(binary, labels, stats, centroids, 4, CV_32S);

    // 获取最大的连通域（忽略背景连通域，背景为标签0）
    int largest_component = 1;
    int max_area = 0;
    for (int i = 1; i < num_labels; ++i) {
        int area = stats.at<int>(i, cv::CC_STAT_AREA);
        if (area > max_area) {
            max_area = area;
            largest_component = i;
        }
    }

    // 创建掩膜并将最大的连通域设置为0（黑色）
    cv::Mat mask = cv::Mat::ones(binary.size(), CV_8U) * 255;
    mask.setTo(0, labels == largest_component);

    // 应用掩膜删除最大的连通域
    cv::Mat result_upper_half;
    upper_half.copyTo(result_upper_half, mask);

    // 将处理后的上半部分与未处理的下半部分合并
    cv::Mat result = image.clone();
    result_upper_half.copyTo(result(upper_half_rect));

    return result;
}

// 删除车前身部分的函数
void removeCarHood(cv::Mat& img, const cv::Mat& mask) {
    // 检查图像和掩码尺寸是否匹配
    if (img.size() != mask.size()) {
        std::cerr << "Error: Image and mask sizes do not match" << std::endl;
        return;
    }

    // 使用掩码将车前身部分置为黑色
    for (int i = 0; i < img.rows; ++i) {
        for (int j = 0; j < img.cols; ++j) {
            if (mask.at<uchar>(i, j) == 255) {
                img.at<cv::Vec3b>(i, j) = cv::Vec3b(0, 0, 0);
            }
        }
    }
}


// 提取白色和黄色车道线的函数
cv::Mat extractLaneLines(const cv::Mat& img) {
    cv::Mat hsvImg, whiteMask, yellowMask, combinedMask;

    // 将图像从BGR格式转换为HSV格式
    cv::cvtColor(img, hsvImg, cv::COLOR_BGR2HSV);

    // 定义白色的HSV范围
    cv::Scalar lowerWhite = cv::Scalar(0, 0, 150);
    cv::Scalar upperWhite = cv::Scalar(30, 200, 255);

    // 定义黄色的HSV范围
    cv::Scalar lowerYellow = cv::Scalar(15, 100, 100);
    cv::Scalar upperYellow = cv::Scalar(35, 255, 255);

    // 创建白色掩码
    cv::inRange(hsvImg, lowerWhite, upperWhite, whiteMask);

    // 创建黄色掩码
    cv::inRange(hsvImg, lowerYellow, upperYellow, yellowMask);

    // 合并掩码
    cv::bitwise_or(whiteMask, yellowMask, combinedMask);

    return combinedMask;
}

    //亮度统一
cv::Mat equalizeChannels(const cv::Mat &src) {
    std::vector<cv::Mat> channels;
    cv::split(src, channels); // Split into B, G, R channels
    for (auto &channel : channels) {
        cv::equalizeHist(channel, channel);
    }
    cv::merge(channels, src); // Merge back to BGR

    cv::imshow("equalizeChannels", src);

    return src;
}

int main() {
    //循环读取五张图片
    for(int i=0;i<5;i++) {
        std::string filename1 = "/home/jr/C++/opencv2/";
        std::string filename0 = ".jpg";
        std::string filename = filename1 + std::to_string(i+1) + filename0;
        std::cout << filename << std::endl;

        // 读取输入图像
        cv::Mat img = cv::imread(filename);
        if (img.empty()) {
            std::cerr << "Error loading image" << std::endl;
            return -1;
        }


        // 读取车前身掩码
        cv::Mat mask = cv::imread("/home/jr/C++/opencv2/car_mask.png", cv::IMREAD_GRAYSCALE);
        if (mask.empty()) {
            std::cerr << "Error loading mask" << std::endl;
            return -1;
        }

        // 删除车前身部分
        removeCarHood(img, mask);

        // // 提取天空掩码
        // cv::Mat skyMask = extractSkyMask(img);

        // 亮度统一
       // cv::Mat img_brightness = equalizeChannels(img);

        // 删除天空部分
        cv::Mat removeskyimg =removeSky(img,90);

        // 提取车道线
        cv::Mat laneLines = extractLaneLines(removeskyimg);

        /*
        // 创建一个结构元素（卷积核）
        cv::Mat element = cv::getStructuringElement(cv::MORPH_CROSS, cv::Size(3, 3));
        */

        // 形态学操作（开运算）
        cv::Mat kernel = getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
        morphologyEx(laneLines, laneLines, cv::MORPH_OPEN, kernel);

        /*
        cv::dilate(laneLines, laneLines, element);
        cv::erode(laneLines, laneLines, element);
        */

        // 显示结果
         cv::imshow("Original Image", img);
        // cv::imshow("Removed sky", removeskyimg);
        // cv::imshow("Lane Lines", laneLines);
        // cv::imshow("Eroded Image", imgEroded2);
        cv::imshow("lane_line", laneLines);

        // 保存结果
        std::string outputFilename = "output_" + std::to_string(i+1) + ".jpg";
        cv::imwrite(outputFilename, laneLines);

        cv::waitKey(0);
    }
    return 0;
}