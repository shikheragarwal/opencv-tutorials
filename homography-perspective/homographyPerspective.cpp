#include <iomanip>
#include <iostream>
#include <opencv2/calib3d/calib3d.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/nonfree/features2d.hpp>

static void showUsage(const char *av0)
{
    std::string av0colon(av0);
    const int width = av0colon.append(": ").size();
    std::cerr << av0colon << "Use homography and a perspective transform "
              << std::endl << std::setw(width) << ""
              << "to locate and outline an object in a scene." << std::endl
              << std::endl
              << "Usage: " << av0 << " <goal> <scene>" << std::endl
              << std::endl
              << "Where: <goal> and <scene> are image files." << std::endl
              << "       <goal> has features present in <scene>." << std::endl
              << "       <scene> is where to search for features" << std::endl
              << "               from the <goal> image." << std::endl
              << std::endl
              << "Example: " << av0 << " ../resources/box.png"
              << " ../resources/box_in_scene.png" << std::endl
              << std::endl;
}

// Features in a goal image matched to a scene image.
//
typedef std::vector<cv::DMatch> Matches;

// The keypoints and descriptors for features in goal or scene image i.
//
struct Features {
    const cv::Mat image;
    std::vector<cv::KeyPoint> keyPoints;
    cv::Mat descriptors;
    std::vector<cv::Point2f> locations;
    Features(const cv::Mat &i): image(i) {}
};

// Return matches of goal in scene.
//
static Matches matchFeatures(Features &goal, Features &scene)
{
    static const int minHessian = 400;
    cv::SurfFeatureDetector detector(minHessian);
    detector.detect(goal.image, goal.keyPoints);
    detector.detect(scene.image, scene.keyPoints);
    cv::SurfDescriptorExtractor extractor;
    extractor.compute(goal.image, goal.keyPoints, goal.descriptors);
    extractor.compute(scene.image, scene.keyPoints, scene.descriptors);
    cv::FlannBasedMatcher matcher;
    Matches result;
    matcher.match(goal.descriptors, scene.descriptors, result);
    return result;
}

// Return only good matches in matches.  A good match has distance less
// than thrice the minimum distance.
//
static Matches goodMatches(const Matches &matches)
{
    double minDist = 100.0, maxDist = 0.0;
    for (int i = 0; i < matches.size(); ++i) {
        const double dist = matches[i].distance;
        if (dist < minDist) minDist = dist;
        if (dist > maxDist) maxDist = dist;
    }
    std::cout << "Minimum distance: " << minDist << std::endl
              << "Maximum distance: " << maxDist << std::endl;
    const double threshold = 3 * minDist;
    Matches result;
    for (int i = 0; i < matches.size(); ++i) {
        if (matches[i].distance < threshold) result.push_back(matches[i]);
    }
    return result;
}

// Return image with matches drawn from goal to scene in random colors.
//
static cv::Mat drawMatches(Features &goal, Features &scene,
                           const Matches &matches)
{
    static const cv::Scalar color = cv::Scalar::all(-1);
    static const std::vector<char> noMask;
    static const int flags
        = cv::DrawMatchesFlags::NOT_DRAW_SINGLE_POINTS
        | cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS;
    cv::Mat result;
    cv::drawMatches(goal.image, goal.keyPoints, scene.image, scene.keyPoints,
                    matches, result, color, color, noMask, flags);
    return result;
}

// Find the best homography between the goal image and the scene image
// based on the features in matches.
//
static cv::Mat findHomography(Features &goal, Features &scene,
                              const Matches &matches)
{
    for (int i = 0; i < matches.size(); ++i) {
        const cv::DMatch &m = matches[i];
        goal.locations.push_back(goal.keyPoints[m.queryIdx].pt);
        scene.locations.push_back(scene.keyPoints[m.trainIdx].pt);
    }
    return cv::findHomography(goal.locations, scene.locations, cv::RANSAC);
}

// Use homography to map corners of the goal object to corners in the scene
// based on the features in matches.
//
static std::vector<cv::Point2f> findCorners(Features &goal, Features &scene,
                                            const Matches &matches)
{
    const cv::Mat homography = findHomography(goal, scene, matches);
    const int x = goal.image.size().width;
    const int y = goal.image.size().height;
    std::vector<cv::Point2f> corners;
    corners.push_back(cv::Point2f(0, 0));
    corners.push_back(cv::Point2f(x, 0));
    corners.push_back(cv::Point2f(x, y));
    corners.push_back(cv::Point2f(0, y));
    std::vector<cv::Point2f> result(corners.size());
    cv::perspectiveTransform(corners, result, homography);
    const cv::Point2f offset(x, 0);
    for (int i = 0; i < result.size(); ++i) result[i] += offset;
    return result;
}

int main(int ac, char *av[])
{
    if (ac == 3) {
        Features  goal(cv::imread(av[1], cv::IMREAD_GRAYSCALE));
        Features scene(cv::imread(av[2], cv::IMREAD_GRAYSCALE));
        if (goal.image.data && scene.image.data) {
            std::cout << std::endl << av[0] << ": Press any key to quit."
                      << std::endl << std::endl;
            const Matches matches = matchFeatures(goal, scene);
            const Matches good = goodMatches(matches);
            cv::Mat image = drawMatches(goal, scene, good);
            const std::vector<cv::Point2f> corner
                = findCorners(goal, scene, good);
            static const cv::Scalar green(0, 255, 0);
            static const int thickness = 4;
            cv::line(image, corner[0], corner[1], green, thickness);
            cv::line(image, corner[1], corner[2], green, thickness);
            cv::line(image, corner[2], corner[3], green, thickness);
            cv::line(image, corner[3], corner[0], green, thickness);
            cv::imshow("Good Matches & Object detection", image);
            cv::waitKey(0);
            return 0;
        }
    }
    showUsage(av[0]);
    return 1;
}
