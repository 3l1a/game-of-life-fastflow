
#ifndef GAMEOFLIFE_OPENCV_DRAW_H
#define GAMEOFLIFE_OPENCV_DRAW_H

#include <cstddef>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include "gol.hpp"

    #define OPENCV_SLEEP 10000


class opencv_visualizer {
public:
    opencv_visualizer(int n_rows_, int n_coloumns_) : n_rows(n_rows_), n_columns(n_coloumns_){
        ocv_matrix = cv::Mat::zeros(n_rows_, n_coloumns_, CV_8UC3);
    }

    void inline draw(value_type **matrix){
        for (size_t i = 1; i < n_rows-1; ++i)
            for (size_t j = 1; j < n_columns-1; ++j) {
                uint8_t c = matrix[i][j] == 1? 127 : 0;
                ocv_matrix.at<cv::Vec3b>(cv::Point(j, i)) = cv::Vec3b(c, c, c);
            }
        usleep(OPENCV_SLEEP);
        cv::imshow("Game of Life", ocv_matrix);
        cv::waitKey(1);
    }

    void inline end_draw(value_type ** matrix){

    }

    cv::Mat ocv_matrix;
    size_t n_rows, n_columns;

};


#endif //GAMEOFLIFE_OPENCV_DRAW_H
