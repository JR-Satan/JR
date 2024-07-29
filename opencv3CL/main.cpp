#include <opencv2/opencv.hpp>
#include <opencv2/features2d.hpp>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

// 函数声明
void processImages(const std::string& templateDir, const std::string& archiveDir, const std::string& outputDir);
cv::Mat findHomographyAndWarp(const cv::Mat& src, const cv::Mat& dst);

int main() {
    std::string templateDir = "../template";
    std::string archiveDir = "../archive";
    std::string outputDir = "../output";

    try {
        // 检查目录是否存在
        if (!fs::exists(templateDir) || !fs::is_directory(templateDir)) {
            throw std::runtime_error("Template directory does not exist or is not a directory: " + templateDir);
        }

        if (!fs::exists(archiveDir) || !fs::is_directory(archiveDir)) {
            throw std::runtime_error("Archive directory does not exist or is not a directory: " + archiveDir);
        }

        // 创建输出目录（如果不存在）
        if (!fs::exists(outputDir)) {
            fs::create_directory(outputDir);
        }

        // 处理图像
        processImages(templateDir, archiveDir, outputDir);
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

// 函数定义
void processImages(const std::string& templateDir, const std::string& archiveDir, const std::string& outputDir) {
    // 获取模板文件夹中的所有图像文件
    std::vector<cv::Mat> templates;
    for (const auto& entry : fs::directory_iterator(templateDir)) {
        if (entry.is_regular_file()) {
            cv::Mat tmpl = cv::imread(entry.path().string(), cv::IMREAD_COLOR);
            if (!tmpl.empty()) {
                templates.push_back(tmpl);
            } else {
                std::cerr << "Failed to load template image: " << entry.path() << std::endl;
            }
        }
    }

    // 检查是否有模板图像
    if (templates.empty()) {
        std::cout << "No template images found in " << templateDir << std::endl;
        return;
    }

    for (const auto& tmpl : templates) {

        // 获取归档文件夹中的所有图像文件
        for (const auto& entry : fs::directory_iterator(archiveDir)) {
            if (entry.is_regular_file()) {
                cv::Mat archiveImg = cv::imread(entry.path().string(), cv::IMREAD_COLOR);
                if (archiveImg.empty()) {
                    std::cerr << "Failed to load archive image: " << entry.path() << std::endl;
                    continue;
                }
                    //灰度图转换
                    cv::Mat image1, image2;
                    cvtColor(tmpl, image1, cv::COLOR_RGB2GRAY);
                    cvtColor(archiveImg, image2, cv::COLOR_RGB2GRAY);

                    //对归档图像做两次开运算
                    cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
                    cv::morphologyEx(image2, image2, cv::MORPH_OPEN, element);
                    cv::morphologyEx(image2, image2, cv::MORPH_OPEN, element);

                    // 使用ORB特征检测器和描述符
                    cv::Ptr<cv::ORB> orbDetector = cv::ORB::create(16000);
                    std::vector<cv::KeyPoint> keyPoint1, keyPoint2;
                    cv::Mat imageDesc1, imageDesc2;

                    orbDetector->detectAndCompute(image1, cv::Mat(), keyPoint1, imageDesc1);
                    orbDetector->detectAndCompute(image2, cv::Mat(), keyPoint2, imageDesc2);

                    // 使用BFMatcher进行特征点匹配
                    cv::BFMatcher matcher(cv::NORM_HAMMING, true);
                    std::vector<cv::DMatch> matches;
                    matcher.match(imageDesc1, imageDesc2, matches);

                    // 使用RANSAC算法剔除离群点
                    std::vector<cv::DMatch> good_matches;
                    std::vector<cv::Point2f> points1, points2;
                    for (const auto& match : matches) {
                        points1.push_back(keyPoint1[match.queryIdx].pt);
                        points2.push_back(keyPoint2[match.trainIdx].pt);
                    }

                    cv::Mat mask;
                    cv::Mat homography = cv::findHomography(points1, points2, cv::RANSAC, 3.0, mask);

                    for (size_t i = 0; i < matches.size(); i++) {
                        if (mask.at<uchar>(i)) {
                            good_matches.push_back(matches[i]);
                        }
                    }

                    std::cout << "total match points: " << matches.size() << std::endl;
                    std::cout << "good matches after RANSAC: " << good_matches.size() << std::endl;

                    /*
                    // 绘制特征点
                    cv::Mat result1;
                    cv::drawKeypoints(image1, keyPoint1, result1, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                    cv::imshow("result1", result1);
                    cv::Mat result2;
                    cv::drawKeypoints(image2, keyPoint2, result2, cv::Scalar::all(-1), cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS);
                    cv::imshow("result2", result2);
                    cv::waitKey(0);


                    cv::Mat img_matches1;
                    cv::drawMatches(image1, keyPoint1, image2, keyPoint2, good_matches, img_matches1,
                                    cv::Scalar::all(-1), cv::Scalar::all(-1),
                                    std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);
                    cv::imshow("Matches", img_matches1);
                    cv::waitKey(0);
                    */

                    // 如果找到足够的好匹配，则认为包含模板
                    if (good_matches.size() > 20) {
                        // 绘制匹配点
                        cv::Mat img_matches;
                        cv::drawMatches(image1, keyPoint1, image2, keyPoint2, good_matches, img_matches,
                                        cv::Scalar::all(-1), cv::Scalar::all(-1),
                                        std::vector<char>(), cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS);

                        // 显示匹配点图像
                        //cv::imshow("Matches", img_matches);
                        //保存在output文件夹中
                        std::string outputPath = outputDir + "/" + entry.path().filename().string();
                        cv::imwrite(outputPath, img_matches);
                        //cv::waitKey(0);
                }
            }
        }
    }
}