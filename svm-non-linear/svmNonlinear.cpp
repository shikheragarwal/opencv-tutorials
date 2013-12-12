#include <opencv2/highgui/highgui.hpp>
#include <opencv2/ml/ml.hpp>


// Train with LINEAR kernel Support Vector Classifier (C_SVC) with up to
// 1e7 iterations to achieve epsilon.
//
static cv::SVMParams makeSvmParams(void)
{
    static const int criteria = CV_TERMCRIT_ITER;
    static const int iterationCount = 10 * 1000 * 1000;
    static const double epsilon = std::numeric_limits<double>::epsilon();
    cv::SVMParams result;
    result.svm_type    = cv::SVM::C_SVC;
    result.kernel_type = cv::SVM::LINEAR;
    result.C           = 0.1;
    result.term_crit   = cv::TermCriteria(criteria, iterationCount, epsilon);
    return result;
}

// Train svm on data and labels.
//
static void trainSvm(cv::SVM &svm, const cv::Mat &data, const cv::Mat labels)
{
    static const cv::Mat zeroIdx;
    static const cv::Mat varIdx = zeroIdx;
    static const cv::Mat sampleIdx = zeroIdx;
    static const cv::SVMParams params = makeSvmParams();
    svm.train(data, labels, varIdx, sampleIdx, params);
}

// Return COUNT points of mostly (80%) SEParable training data randomly
// scattered in a float matrix of size.
//
// The first SEParable (40%) points belong to the first region (x1), which
// labelData() will later give the value 1.0.  The last SEParable (40%)
// points belong to another region (x2), which labelData() will later give
// the value 2.0.  In between are 20% mixed between the two regions (xM),
// such that labelData() will give the first half of the mixed 20% the
// value 1.0 and the second half the value 2.0.
//
// The draw*() routines will later color the regions by coloring the first
// half of the COUNT points green and the second half blue.
//
// The regions divide along the X (column or width) axis and span the
// entire Y (row or height) axis.
//                                   rows Y     columns X
// The first separable region is {[  0%,  40%), [0, 100%)}
// The mixed region between is   {[ 40%,  60%), [0, 100%)}
// The other separable region is {[ 60%, 100%), [0, 100%)}
//
// The result is a 2-D float matrix whose row is the COUNT data points, and
// where column 0 has the X coordinates and column 1 has the Y coordinates.
//
static cv::Mat_<float> makeData(int COUNT, const cv::Size &size)
{
    static cv::RNG rng(666);
    static const int uniform = cv::RNG::UNIFORM;
    const int SEP = 0.4 * COUNT;
    const int cols = size.width;
    const int rows = size.height;
    cv::Mat_<float> result(COUNT, 2, CV_32FC1);
    const cv::Mat x1 = result.rowRange(                0,   SEP);
    const cv::Mat xM = result.rowRange(        SEP, COUNT - SEP);
    const cv::Mat x2 = result.rowRange(COUNT - SEP, COUNT      );
    rng.fill(x1.colRange(0, 1), uniform, 0.0 * cols, 0.4 * cols); //  40%
    rng.fill(xM.colRange(0, 1), uniform, 0.4 * cols, 0.6 * cols); //  20%
    rng.fill(x2.colRange(0, 1), uniform, 0.6 * cols, 1.0 * cols); //  40%
    rng.fill(result.colRange(1, 2), uniform, 0.0 * rows, 1.0 * rows);
    return result;
}

// Return half the COUNT data labeled 1.0 and half labeled 2.0.
//
static cv::Mat_<float> labelData(int COUNT)
{
    cv::Mat_<float> result(COUNT, 1, CV_32FC1);
    result.rowRange(        0, COUNT / 2).setTo(1.0);
    result.rowRange(COUNT / 2, COUNT    ).setTo(2.0);
    return result;
}

// Draw on image the 2 classification regions predicted by svm.
// Draw class labeled 1.0 in green, and class 2.0 in blue.
//
static void drawRegions(cv::Mat_<cv::Vec3b> &image, const cv::SVM &svm)
{
    static const cv::Vec3b green(  0, 100,  0);
    static const cv::Vec3b  blue(100,   0,  0);
    for (int i = 0; i < image.rows; ++i) {
        for (int j = 0; j < image.cols; ++j) {
            const cv::Mat sample = (cv::Mat_<float>(1,2) << i, j);
            const float response = svm.predict(sample);
            if (response == 1.0) {
                image(j, i) = green;
            } else if (response == 2.0) {
                image(j, i) = blue;
            } else {
                std::cerr << "Unexpected response from SVM::predict(): "
                          << response << std::endl;
                assert(!"expected response from SVM");
            }
        }
    }
}

// Draw training data as COUNT circles of radius 3 on image.
// Again, draw class 1.0 in green and class 2.0 in blue.
//
static void drawData(cv::Mat &image, int COUNT, const cv::Mat_<float> &data)
{
    static const cv::Scalar green(  0, 255,   0);
    static const cv::Scalar  blue(255,   0,   0);
    static const int radius = 3;
    static const int thickness = -1;
    static const int lineKind = 8;
    for (int i = 0; i < COUNT / 2; ++i) {
        const cv::Point center(data(i, 0), data(i, 1));
        cv::circle(image, center, radius, green, thickness, lineKind);
    }
    for (int i = COUNT / 2; i < COUNT; ++i) {
        const cv::Point center(data(i, 0), data(i, 1));
        cv::circle(image, center, radius, blue, thickness, lineKind);
    }

}

// Draw the support vectors in svm as circles of radius 6 in red.
//
static void drawSupportVectors(cv::Mat &image, const cv::SVM &svm)
{
    static const cv::Scalar red(0, 0, 255);
    static const int radius = 6;
    static const int thickness = 2;
    static const int lineKind = 8;
    const int count = svm.get_support_vector_count();
    std::cout << "support vector count == " << count << std::endl;
    for (int i = 0; i < count; ++i) {
        const float *const v = svm.get_support_vector(i);
        const cv::Point center(v[0], v[1]);
        std::cout << i << ": center == " << center << std::endl;
        cv::circle(image, center, radius, red, thickness, lineKind);
    }
}

int main(int, const char *[])
{
    static const int COUNT = 200;
    cv::Mat_<cv::Vec3b> image = cv::Mat::zeros(512, 512, CV_8UC3);
    const cv::Mat_<float> data = makeData(COUNT, image.size());
    const cv::Mat_<float> labels = labelData(COUNT);
    std::cout << "Training SVM ... " << std::flush;
    cv::SVM svm;
    trainSvm(svm, data, labels);
    std::cout << "done." << std::endl;
    drawRegions(image, svm);
    drawData(image, COUNT, data);
    drawSupportVectors(image, svm);
    cv::imwrite("result.png", image);
    cv::imshow("SVM for Non-Linear Training Data", image);
    cv::waitKey(0);
}
