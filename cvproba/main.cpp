#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"

#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>

#include <QDebug>

using namespace cv;
using namespace std;


bool sortByCircularityPredicate(vector<double> p_1, vector<double> p_2)
{
    return p_1[2] < p_2[2];
}


bool pupilFound(RotatedRect p_rect)
{
    return p_rect.center.x != -1 && p_rect.center.y != -1;
}


void drawMeasurePoint(Mat& p_image, Point p_point)
{
    Scalar green = Scalar(0,255,0);
    Scalar red = Scalar(0,0,255);
    int cross_size = 20;

    circle(p_image, p_point, 10, red, 2, -1);
    line(p_image, Point(p_point.x-cross_size, p_point.y), Point(p_point.x+cross_size, p_point.y), green, 2);
    line(p_image, Point(p_point.x, p_point.y-cross_size), Point(p_point.x, p_point.y+cross_size), green, 2);
}


void drawDisplayPoint(Mat& p_image, Point p_point)
{
    Scalar yellow = Scalar(0,255,255);
    Scalar red = Scalar(0,0,255);
    int cross_size = 20;

    circle(p_image, p_point, 10, red, 2, -1);
    line(p_image, Point(p_point.x-cross_size, p_point.y), Point(p_point.x+cross_size, p_point.y), yellow, 2);
    line(p_image, Point(p_point.x, p_point.y-cross_size), Point(p_point.x, p_point.y+cross_size), yellow, 2);
}

Point calculatePosition(Point a, Point b, Point c, Point d, Point p, int screen_width, int screen_height, int calibPadding, double* relativePercentX, double* relativePercentY)
{
    double C = (double)(a.y - p.y) * (d.x - p.x) - (double)(a.x - p.x) * (d.y - p.y);
    double B = (double)(a.y - p.y) * (c.x - d.x) + (double)(b.y - a.y) * (d.x - p.x) - (double)(a.x - p.x) * (c.y - d.y) - (double)(b.x - a.x) * (d.y - p.y);
    double A = (double)(b.y - a.y) * (c.x - d.x) - (double)(b.x - a.x) * (c.y - d.y);

    double D = B * B - 4 * A * C;

    double u = (-B - sqrt(D)) / (2 * A);

    double p1x = a.x + (b.x - a.x) * u;
    double p2x = d.x + (c.x - d.x) * u;

    double px = p.x;

    double v = (px - p1x) / (p2x - p1x);

    // in display bounds
    /*
    if (u > 1)
        u = 1;
    else if (u < 0)
        u = 0;

    if (v > 1)
        v = 1;
    else if (v < 0)
        v = 0;
    */

    Point ret;
    // calculate screen coordinates
    ret.x = u*(screen_width-2*calibPadding)+calibPadding;
    ret.y = v*(screen_height-2*calibPadding)+calibPadding;

    // save relative coordinates
    *relativePercentX = u;
    *relativePercentY = v;

    return ret;
}

int capture();
int render(char **argv);

int main(int argc, char** argv)
{
    string str1 ("r");


    if (argv[1] == NULL)
    {
        capture();
    }
    else
    {
        string str2 (argv[1]);

        if (str1.compare(str2) == 0)
        {
            qDebug("rendering");
            render(argv);
        }
        else
        {
            qDebug("capturing");
            capture();
        }
    }
}


int render(char** argv)
{
    stringstream filename;
    filename << argv[2];

    ifstream infile;
    infile.open(filename.str().c_str());

    int x,y;
    char line[255];
    int lines = 0;

    vector<Point> points;

    while (infile)
    {
        infile >> x;
        infile >> y;

        infile.getline(line, 255);

        //qDebug("%d, %d", x, y);
        lines++;

        if (lines > 4)
        {
            Point p;
            p.x = x;
            p.y = y;

            points.push_back(p);
        }
    }

    infile.close();

    int screen_width = 1280;
    int screen_height = 1024;

    Mat displayImage = Mat(screen_height, screen_width, CV_8UC3);

    Mat testImage = imread("repin_FINAL.jpg");
    resize(testImage, testImage, Size(), 2, 2);

    Rect roi = Rect((displayImage.cols-testImage.cols)/2, (displayImage.rows-testImage.rows)/2, testImage.cols, testImage.rows);
    Mat roiImg(displayImage, roi);
    testImage.copyTo(roiImg);

    namedWindow("Display", CV_WINDOW_AUTOSIZE | CV_GUI_NORMAL);
    cvSetWindowProperty("Display", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

    for (unsigned int i = 0; i<points.size(); i++)
    {
        if (points[i].x < 0 || points[i].y < 0 || points[i].x > screen_width || points[i].y > screen_height)
        {
            points.erase(points.begin()+i);
            i--;
        }
    }

    for (unsigned int i = 0; i<points.size(); i++)
    {
        circle(displayImage, points[i], 2, Scalar(0,255,0), 2);

        if (i < points.size())
        {
            cv::line(displayImage, points[i], points[i+1], Scalar(0, 255,255), 1, 8);
        }
    }

    imshow("Display", displayImage);

    int c;
    do
    {
        c = waitKey(1);
    }
    while (c != 27);

    return 0;
}

int capture()
{
    VideoCapture cap;

    cap.open(1);
    if( cap.isOpened() )
            cout << "Video " <<
                ": width=" << cap.get(CV_CAP_PROP_FRAME_WIDTH) <<
                ", height=" << cap.get(CV_CAP_PROP_FRAME_HEIGHT) <<
                ", nframes=" << cap.get(CV_CAP_PROP_FRAME_COUNT) << endl;

    if( !cap.isOpened() )
    {
        cout << "Could not initialize capturing...\n";
        return -1;
    }

    // set width and height
    cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
    cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);

    // for benchmarking
    time_t start, end;
    double fps;
    int counter = 0;
    double sec;

    // init frames
    Mat frame, gray, equalized, thresholded, inverted;
    int thresh = 8;

    // contours
    vector<vector<Point> > contours;
    vector<Vec4i> hierarchy;

    // object properties
    vector<vector<double> > properties;

    // previous objects
    vector<RotatedRect> positions;
    bool notFound = false;
    int errors = 0;
    RotatedRect init = RotatedRect(Point(320,240), Size(100,100), 0);

    // measurement vector
    vector<Point> measurePoints;
    vector<RotatedRect> measureValues;

    int screen_width = 1280;
    int screen_height = 1024;
    int grid_size = 100;

    int start_x = (screen_width%grid_size)/2;
    int start_y = (screen_height%grid_size)/2;

    for (int i=0; i<(screen_height/grid_size)+1; i++)
    {
        for (int j=0; j<(screen_width/grid_size)+1; j++)
        {
            measurePoints.push_back(Point(start_x+j*grid_size, start_y+i*grid_size));
        }
    }

    Mat measureImage = Mat(screen_height, screen_width, CV_8UC3);
    int measureIndex = -1;

    // calibration
    Vector<Point> calibPoints;
    Vector<Point> calibValues;

    int calibPadding = 0;

    calibPoints.push_back(Point(calibPadding, calibPadding));
    calibPoints.push_back(Point(screen_width-calibPadding, calibPadding));
    calibPoints.push_back(Point(screen_width-calibPadding, screen_height-calibPadding));
    calibPoints.push_back(Point(calibPadding, screen_height-calibPadding));

    Mat calibImage = Mat(screen_height, screen_width, CV_8UC3);
    int calibIndex = -1;

    Point screenPosition;
    double relativePercentX;
    double relativePercentY;

    // display
    Mat displayImage = Mat(screen_height, screen_width, CV_8UC3);
    bool displayOn = false;

    Vector<Point> displayValues;

    bool targetRendered = true;

    Mat testImage = imread("repin_FINAL.jpg");
    resize(testImage, testImage, Size(), 2, 2);

    // init GUI
    namedWindow("Original", WINDOW_AUTOSIZE);
    cvMoveWindow("Original", 20, 20);
    namedWindow("Objects", WINDOW_AUTOSIZE);
    cvMoveWindow("Objects", 420, 20);

    createTrackbar("Threshold", "Objects", &thresh, 255);

    // start measuring fps
    time(&start);

    // main loop
    for(;;)
    {
        // get frame and flip it
        cap >> frame;
        if( frame.empty() )
            break;
        flip(frame, frame, 0);

        // greyscale
        cvtColor(frame, gray, CV_BGR2GRAY);
        equalizeHist(gray, equalized);
        threshold(equalized, thresholded, thresh, 255, THRESH_BINARY);

        // inverse for contour finding
        inverted=255-thresholded ;

        // image for objects
        Mat objects = Mat::zeros(inverted.rows, inverted.cols, CV_8UC3);

        // reset found counter
        notFound = false;

        // find contours
        findContours(inverted, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

        // if contours found
        if (contours.size() > 0)
        {
            properties.clear();

            // gather contour information
            for(int idx=0; idx >= 0; idx = hierarchy[idx][0] )
            {
                // set contour's convex hull
                convexHull(contours[idx], contours[idx], true);

                // calculate properties
                double area = contourArea(contours[idx]);
                double perimeter = arcLength(contours[idx], true);
                double circularity = perimeter/(2*sqrt(M_PI*area));

                // only for significant contours
                if (area > 2500)
                {
                    // save properties
                    vector<double> props;
                    props.push_back(idx);           // 0
                    props.push_back(area);          // 1
                    props.push_back(circularity);   // 2

                    properties.push_back(props);

                    // draw object with green
                    Scalar green(0, 255, 0);
                    drawContours(objects, contours, idx, green, CV_FILLED, 8, hierarchy, 0);
                }
            }

            if (properties.size() > 0)
            {
                // sort by fitness value
                sort(properties.begin(), properties.end(), sortByCircularityPredicate);

                // select pupil center
                int pupilIndex = properties[0][0];

                // if circular enough
                if(properties[0][2] < 1.07)
                {
                    // must have enough points
                    if(contours[pupilIndex].size() > 5)
                    {
                        // -----------
                        // PUPIL FOUND
                        RotatedRect ell = fitEllipse(contours[pupilIndex]);
                        positions.push_back(ell);

                        // draw object with red
                        Scalar red(0, 0, 255);
                        drawContours(objects, contours, pupilIndex, red, CV_FILLED, 8, hierarchy, 0);
                    }
                    else
                    {
                        notFound = true;
                    }
                }
                else
                {
                    notFound = true;
                }
            }
            else
            {
                notFound = true;
            }
        }
        else
        {
            notFound = true;
        }


        // if no pupil found use the last one
        if (notFound == true)
        {
            errors++;

            if (positions.size() == 0)
            {
                positions.push_back(init);
            }
            else
            {
                if (errors < 5)
                {
                    // just use last value
                    positions.push_back(positions.back());
                }
                else
                {
                    positions.push_back(RotatedRect(Point(-1,-1),Size(0,0),0));
                }
            }
        }
        else
        {
            errors = 0;
        }


        // ellipse of the eye
        RotatedRect eyePosition = positions.back();


        // if eye is opened
        if (pupilFound(eyePosition))
        {
            // ----------
            // DRAW PUPIL
            Scalar red(0, 0, 255);

            ellipse(frame, eyePosition, red, 2);
            circle(frame, eyePosition.center, 2, red, 2);
        }

        // render calibration quadriliteral and calculate actual position
        if (calibValues.size() == 4)
        {
            // draw Eye region
            Scalar yellow(0,255,255);
            Point a = calibValues[0];
            Point b = calibValues[1];
            Point c = calibValues[2];
            Point d = calibValues[3];

            line(frame, a, b, yellow, 2, 8);
            line(frame, b, c, yellow, 2, 8);
            line(frame, c, d, yellow, 2, 8);
            line(frame, d, a, yellow, 2, 8);

            // calculate position
            Point p = eyePosition.center;

            screenPosition = calculatePosition(a, b, c, d, p, screen_width, screen_height, calibPadding, &relativePercentX, &relativePercentY);

            if (!targetRendered)
            {
                // save to display values
                displayValues.push_back(screenPosition);
            }
        }

        // display stuff on screen position
        if (displayOn)
        {
            // clear and draw actual point
            displayImage.setTo(Scalar(0,0,0));

            // draw to ROI
            Rect roi = Rect((displayImage.cols-testImage.cols)/2, (displayImage.rows-testImage.rows)/2, testImage.cols, testImage.rows);
            Mat roiImg(displayImage, roi);
            testImage.copyTo(roiImg);

            if (targetRendered)
            {
                // draw display point
                drawDisplayPoint(displayImage, screenPosition);
            }

            // show image
            imshow("Display", displayImage);
        }

        // calculate current fps
        time(&end);
        ++counter;
        sec = difftime(end, start);
        fps = counter/sec;

        stringstream s;
        s.precision(2);
        s << fixed << "FPS: " << fps << " | ";

        if (pupilFound(eyePosition))
        {
            s << "Eye position: (" << eyePosition.center.x << "," << eyePosition.center.y << ")";
        }
        else
        {
            s << "Eye closed";
        }

        s << " | Screen position: (" << screenPosition.x << "," << screenPosition.y << ")";

        s << " | Inside position: (" << relativePercentX << "," << relativePercentY << ")";

        displayStatusBar("Original", s.str(), 1000);

        // render images
        imshow("Original", frame);
        imshow("Objects", objects);

        // quit
        int c = waitKey(1);
        if (c == 'q' || c == 'Q' || (c & 255) == 27)
        {
           break;
        }
        // save data
        else if (c == 'k')
        {
            if (measureIndex < (int)measurePoints.size())
            {
                if (measureIndex == -1)
                {
                    // open measurement window
                    namedWindow("Measurement", CV_WINDOW_AUTOSIZE | CV_GUI_NORMAL);
                    cvSetWindowProperty("Measurement", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
                }
                else
                {
                    // save stuff
                    measureValues.push_back(eyePosition);
                }

                // next point
                measureIndex++;

                // render next point
                measureImage.setTo(Scalar(0,0,0));
                drawMeasurePoint(measureImage, measurePoints[measureIndex]);
                imshow("Measurement", measureImage);

                // destroy window if necessary
                if ((unsigned int)measureIndex == measurePoints.size())
                {
                    destroyWindow("Measurement");
                }
            }
        }
        // calibrate
        else if (c == 'c')
        {
            if (calibIndex == -1)
            {
                // clear previous calibration data
                calibValues.clear();

                // open measurement window
                namedWindow("Calibration", CV_WINDOW_AUTOSIZE | CV_GUI_NORMAL);
                cvSetWindowProperty("Calibration", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
            }
            else
            {
                calibValues.push_back(eyePosition.center);
            }

            // next point
            calibIndex++;

            // render next point
            calibImage.setTo(Scalar(0,0,0));
            drawMeasurePoint(calibImage, calibPoints[calibIndex]);
            imshow("Calibration", calibImage);

            // destroy window if necessary
            if ((unsigned int)calibIndex == calibPoints.size())
            {
                destroyWindow("Calibration");
                calibIndex = -1;
            }
        }
        // display
        else if (c == 'd')
        {
            if (!displayOn)
            {
                // open measurement window
                namedWindow("Display", CV_WINDOW_AUTOSIZE | CV_GUI_NORMAL);
                cvSetWindowProperty("Display", CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);

                displayOn = true;
            }
            else
            {
                destroyWindow("Display");

                displayOn = false;
            }
        }
        else if (c == 'p')
        {
            targetRendered = targetRendered?false:true;
        }
    }


    // save if measurement if complete
    if ((unsigned int)measureIndex == measurePoints.size())
    {
        // set filename to current date
        time_t t = time(0);
        struct tm *now = localtime(&t);

        stringstream filename;
        filename << "calib_" << (now->tm_year + 1900) << "-" << (now->tm_mon + 1) << "-" << now->tm_mday << "_" << now->tm_hour << "-" << now->tm_min << "-" << now->tm_sec << ".txt";

        ofstream outfile;
        outfile.open(filename.str().c_str());

        for (unsigned int i = 0; i<measurePoints.size(); i++)
        {
            outfile << measurePoints[i].x << ";"
                    << measurePoints[i].y << ";"
                    << measureValues[i].center.x << ";"
                    << measureValues[i].center.y << ";"
                    << measureValues[i].size.width << ";"
                    << measureValues[i].size.height << ";"
                    << measureValues[i].angle << endl;
        }

        outfile.close();
    }

    // save display data if present
    if (displayValues.size() != 0)
    {
        // set filename to current date
        time_t t = time(0);
        struct tm *now = localtime(&t);

        stringstream filename;
        filename << "display_" << (now->tm_year + 1900) << "-" << (now->tm_mon + 1) << "-" << now->tm_mday << "_" << now->tm_hour << "-" << now->tm_min << "-" << now->tm_sec << ".txt";

        ofstream outfile;
        outfile.open(filename.str().c_str());

        for (unsigned int i = 0; i<calibValues.size(); i++)
        {
            outfile << calibValues[i].x << " "
                    << calibValues[i].y << " "
                    << "calib" << endl;
        }

        for (unsigned int i = 0; i<displayValues.size(); i++)
        {
            outfile << displayValues[i].x << ";"
                    << displayValues[i].y << endl;
        }

        outfile.close();
    }


    return 0;
}
