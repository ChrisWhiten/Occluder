// Chris Whiten - 2012 
// Quick and dirty script to add occlusions to images for evaluation purposes.
// To use: Modify variables "source" and "dest" to set source and destination directories.
// Source directory should have the images in it.  Then, we will cycle through
// the sequence and add occlusions to each.  Modifiable via mouse.
// Hit 'enter' to save.
// Alternatively, setting "automate" to true will just automatically place
// occluded regions and save the files without user input.

// To keep things quick, only use OpenCV's highgui stuff for now.
// Consider upgrading to Qt for a better GUI later.
#include <opencv2\core\core.hpp>
#include <opencv2\highgui\highgui.hpp>
#include <boost\filesystem.hpp>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;
namespace fs = boost::filesystem;

string source = "C:/Projects/Experiments/Shapes1032b";
string dest = "C:/Projects/Experiments/Shapes1032bOccluded";

cv::Rect occluder;
cv::Mat *current_img;
bool automate = false;

// update window showing current occluding region.
void updateVisualization()
{
	cv::Mat visualization = current_img->clone();
	cv::rectangle(visualization, occluder, CV_RGB(0, 0, 0), CV_FILLED);
	cv::imshow("Occluded image", visualization);
}

// save image with occluded region added.
void saveImage(string s)
{
	cv::Mat to_save = current_img->clone();
	cv::rectangle(to_save, occluder, CV_RGB(0, 0, 0), CV_FILLED);
	stringstream ss;
	ss << dest << "/" << s;
	cv::imwrite(ss.str(), to_save);
}

// process the source folder to gather all image files to occlude.
void getImagesToProcess(vector<string> &files)
{
	fs::path src_dir(source);
	fs::directory_iterator end_iter;

	if (fs::exists(src_dir) && fs::is_directory(src_dir))
	{
		for (fs::directory_iterator dir_iter(src_dir); dir_iter != end_iter; ++dir_iter)
		{
			if (fs::is_regular_file(dir_iter->status()))
			{
				files.push_back(dir_iter->path().filename().string());
			}
		}
	}
}

// Add a rectangular region to occlude part of the image.
// Return that rectangle in case we need it later...
void getInitialOcclusionFigure(cv::Mat *img)
{
	unsigned height = img->rows;
	unsigned width = img->cols;

	// initialize the occluding rectangle as 80% of the height, 10% from top,
	// 30% of the width, 10% from left.
	occluder = cv::Rect(.1 * width, .1 * height, width * .3, height * .8);
	updateVisualization();
}

// compute the Manhattan Distance between two points.
int manhattanDistance(const cv::Point p1, const cv::Point p2)
{
	return (abs(p1.x - p2.x) + abs(p1.y - p2.y));
}

// Handle mouse clicks.
void onMouse(int event, int x, int y, int, void *)
{
	enum corner_to_drag {tl, tr, bl, br, middle};
	static bool dragging = false;
	static int selected_corner = tl;
	const int CLICK_LENIANCY_THRESHOLD = 10;
	cv::Point clicked_point(x, y);
	static cv::Point dragging_displacement(0, 0); // representing the displacement vector with a point.

	switch (event)
	{
		case CV_EVENT_LBUTTONDOWN:
			dragging = true;
			dragging_displacement.x = occluder.x - x;
			dragging_displacement.y = occluder.y - y;

			// select a corner to drag.
			if (manhattanDistance(occluder.tl(), clicked_point) < CLICK_LENIANCY_THRESHOLD)
				selected_corner = tl;
			else if (manhattanDistance(occluder.br(), clicked_point) < CLICK_LENIANCY_THRESHOLD)
				selected_corner = br;
			else if (manhattanDistance(cv::Point(occluder.tl().x + occluder.width, occluder.tl().y), clicked_point) < CLICK_LENIANCY_THRESHOLD)
				selected_corner = tr;
			else if (manhattanDistance(cv::Point(occluder.tl().x, occluder.tl().y + occluder.height), clicked_point) < CLICK_LENIANCY_THRESHOLD)
				selected_corner = bl;
			else if (occluder.contains(clicked_point))
			{
				selected_corner = middle;
			}
			else
				dragging = false; // wasn't close enough to any corner, no dragging.
			break;

		case CV_EVENT_LBUTTONUP:
			dragging = false;
			break;

		// for now, only allow resizing from top-left
		// and move the rectangle from the middle...
		// This doesn't take away any functionality, only ease of use.
		// But time is probably better spent on other things right now.
		case CV_EVENT_MOUSEMOVE:
			// if we've got a corner, drag.
			if (dragging)
			{
				// update corresponding corner of occluder.
				switch (selected_corner)
				{
					case tl:
						occluder.height += (occluder.y - y);
						occluder.width += (occluder.x - x);
						occluder.x = x;
						occluder.y = y;
						break;
					case middle:
						occluder.x = x + dragging_displacement.x;
						occluder.y = y + dragging_displacement.y;
						break;
					case tr:
						occluder.width = x - occluder.x;
						occluder.height += (occluder.y - y);
						occluder.y = y;
						break;
					case bl:
						occluder.height += y - (occluder.y + occluder.height);
						occluder.width += (occluder.x - x);
						occluder.x = x;
						break;
					case br:
						occluder.height += y - (occluder.y + occluder.height);
						occluder.width += x - (occluder.x + occluder.width);
						break;
				}
				updateVisualization();
			}
			break;
	}
}

void main()
{
	vector<string> files_to_process;
	getImagesToProcess(files_to_process);
	
	stringstream ss;
	for (auto file = files_to_process.begin(); file != files_to_process.end(); ++file)
	{
		ss.str("");
		ss.clear();

		ss << source << "/" << *file;
		current_img = new cv::Mat(cv::imread(ss.str()));
		getInitialOcclusionFigure(current_img);
		
		cv::namedWindow("Occluded image");
		cv::setMouseCallback("Occluded image", onMouse, 0);
		if (automate)
		{
			cv::waitKey(1);
		}
		else
		{
			cv::waitKey();
		}
		saveImage(*file);
	}

	delete current_img;
}