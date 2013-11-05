#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>


// Display dst in the named window.  Lay windows out 3 across.
//
static void makeWindow(const char *window, const cv::Mat &image)
{
    static const int across = 3;
    static int moveCount = 0;
    cv::imshow(window, image);
    const int moveX = (moveCount % across) * image.cols;
    const int moveY = (moveCount / across) * (50 + image.rows);
    cv::moveWindow(window, moveX, moveY);
    ++moveCount;
}

// Return src after applying a default Gaussian blur with a kernel of size
// (kernelSize x kernelSize) and converting to grayscale and showing the
// results.
//
static cv::Mat showOriginalBlurGray(const cv::Mat &src, int kernelSize)
{
    static const double sigmaX = 0.0;
    static const double sigmaY = 0.0;
    static const int borderKind = cv::BORDER_DEFAULT;
    const cv::Size kernel(kernelSize, kernelSize);
    makeWindow("Original", src);
    cv::Mat blur;
    cv::GaussianBlur(src, blur, kernel, sigmaX, sigmaY, borderKind);
    makeWindow("Original Blur", blur);
    cv::Mat result;
    cv::cvtColor(blur, result, cv::COLOR_RGB2GRAY);
    makeWindow("Original Blurred Grayscale", result);
    return result;
}

// Show application of Sobel() to src.
//
static void showSobel(const cv::Mat &src, int kernelSize)
{
    static const int borderKind = cv::BORDER_DEFAULT;
    static const int depth = CV_16S;
    static const double scale = 1.0;
    static const double delta = 0.0;
    static const double alpha = 0.5;
    static const double beta  = 0.5;
    static const double gamma = 0.0;
    cv::Mat grad, gradX, gradY, absGradX, absGradY;
    int dx = 1, dy = 0;
    cv::Sobel(src, gradX, depth, dx, dy, kernelSize, scale, delta, borderKind);
    cv::convertScaleAbs(gradX, absGradX);
    dx = 0, dy = 1;
    cv::Sobel(src, gradY, depth, dx, dy, kernelSize, scale, delta, borderKind);
    cv::convertScaleAbs(gradY, absGradY);
    cv::addWeighted(absGradX, alpha, absGradY, beta, gamma, grad);
    makeWindow("Sobel Derivative", grad);
}

// Show application of Scharr() to src.
//
static void showScharr(const cv::Mat &src, int kernelSizeIgnored)
{
    static const int borderKind = cv::BORDER_DEFAULT;
    static const int depth = CV_16S;
    static const double scale = 1.0;
    static const double delta = 0.0;
    static const double alpha = 0.5;
    static const double beta  = 0.5;
    static const double gamma = 0.0;
    cv::Mat grad, gradX, gradY, absGradX, absGradY;
    int dx = 1, dy = 0;
    cv::Scharr(src, gradX, depth, dx, dy, scale, delta, borderKind);
    cv::convertScaleAbs(gradX, absGradX);
    dx = 0, dy = 1;
    cv::Scharr(src, gradY, depth, dx, dy, scale, delta, borderKind);
    cv::convertScaleAbs(gradY, absGradY);
    cv::addWeighted(absGradX, alpha, absGradY, beta, gamma, grad);
    makeWindow("Scharr Derivative", grad);
}

// Show application of Sobel(CV_SCHARR) to src.
//
static void showSobelScharr(const cv::Mat &src, int kernelSizeIgnored)
{
    static const int borderKind = cv::BORDER_DEFAULT;
    static const int depth = CV_16S;
    static const double scale = 1.0;
    static const double delta = 0.0;
    static const double alpha = 0.5;
    static const double beta  = 0.5;
    static const double gamma = 0.0;
    cv::Mat grad, gradX, gradY, absGradX, absGradY;
    int dx = 1, dy = 0;
    cv::Sobel(src, gradX, depth, dx, dy, CV_SCHARR, scale, delta, borderKind);
    cv::convertScaleAbs(gradX, absGradX);
    dx = 0, dy = 1;
    cv::Sobel(src, gradY, depth, dx, dy, CV_SCHARR, scale, delta, borderKind);
    cv::convertScaleAbs(gradY, absGradY);
    cv::addWeighted(absGradX, alpha, absGradY, beta, gamma, grad);
    makeWindow("Sobel(CV_SCHARR) Derivative", grad);
}

int main(int ac, const char *av[])
{
    if (ac == 2) {
        const cv::Mat image = cv::imread(av[1]);
        if (image.data) {
            static const int kernelSize = 3;
            const cv::Mat blurGray = showOriginalBlurGray(image, kernelSize);
            showSobel(blurGray, kernelSize);
            showScharr(blurGray, kernelSize);
            showSobelScharr(blurGray, kernelSize);
            cv::waitKey(0);
            return 0;
        }
    }
    std::cerr << av[0] << ": Edge detection with Sobel and Scharr derivatives."
              << std::endl << std::endl
              << "Usage: " << av[0] << " <image-file>" << std::endl
              << std::endl
              << "Where: <image-file> is the name of an image file."
              << std::endl << std::endl
              << "Example: " << av[0] << " ../resources/lena.jpg"
              << std::endl << std::endl;
    return 1;
}
