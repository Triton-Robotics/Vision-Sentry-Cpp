//#include <opencv2/core/core.hpp>
//#include <opencv2/highgui/highgui.hpp>
//#include <opencv2/imgproc/imgproc.hpp>
//#include <opencv2/video/video.hpp>
//
//int main() {
//
//	cv::Mat frame;
//
//	// kalman filter params
//	int state_size = 6; // size of state
//	int measure_size = 4; // size of measurements
//	int control_params = 0; // control params size
//
//	unsigned int type = CV_32F; // type of data used in kalman filter
//
//	cv::KalmanFilter kf(state_size, measure_size, control_params, type);
//
//	cv::Mat state(state_size, 1, type); // [x, y, x_v, v_y, w, h]
//	cv::Mat measure(measure_size, 1, type); // [z_x, z_y, z_w, z_h]
//
//	// a matrix: transition state matrix
//	cv::setIdentity(kf.transitionMatrix); 
//
//	// set up measurement matrix h
//	kf.measurementMatrix = cv::Mat::zeros(measure_size, state_size, type);
//	kf.measurementMatrix.at<float>(0) = 1.0f;
//	kf.measurementMatrix.at<float>(7) = 1.0f;
//	kf.measurementMatrix.at<float>(16) = 1.0f;
//	kf.measurementMatrix.at<float>(23) = 1.0f;
//
//	// process noise covariance matrix Q
//	kf.processNoiseCov.at<float>(0) = 1e-2;
//	kf.processNoiseCov.at<float>(7) = 1e-2;
//	kf.processNoiseCov.at<float>(14) = 5.0f;
//	kf.processNoiseCov.at<float>(21) = 5.0f;
//	kf.processNoiseCov.at<float>(28) = 1e-2;
//	kf.processNoiseCov.at<float>(35) = 1e-2;
//
//	// Measure Noise covariance matrix R
//	cv::setIdentity(kf.measurementNoiseCov, cv::Scalar(1e-1));
//
//	// camera index - default test
//	int cam_idx = 0;
//
//	cv::VideoCapture cap;
//
//	if (!cap.open(cam_idx)) {
//		printf("Default webcam not connected!\n");
//		exit(EXIT_FAILURE);
//	}
//
//	cap.set(CV_CAP_PROP_FRAME_WIDTH, 1024);
//	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 768);
//
//	printf("Hit 'q' to exit\n");
//
//	char ch = 0;
//
//	double ticks = 0;
//	bool found = false;
//
//	int not_found_count = 0;
//
//	// run until q key hit
//	while (ch != 'q' && ch != 'Q') {
//		double pre_ticks = ticks; // stores current ticks before update
//		ticks = (double)cv::getTickCount();
//		double dt = (double) (ticks - pre_ticks) / cv::getTickFrequency(); // get dt in seconds
//
//		cap >> frame; // get current frame
//	}
//
//	return 0;
//}