#include <iostream>
#include <time.h>
#include <vector>
#include <algorithm>
#include <random>
#include "opencv2\opencv.hpp"  // Include the OpenCV header

class Cell;

cv::Mat image; // (size x size) pixels, initially black. Plan is for perimeter cells to be one color and living cells to be white

int tracker = 0;
class Group {
public:
	int randInt = rand();	//secondary metric for sorting
	int tertiary = (tracker++);
	std::set<Cell*> liveCells;
	std::set<Cell*> perimeterCells;
};

struct GroupComparator {
	bool operator()(const Group* a, const Group* b) const {
		if (a->liveCells.size() == b->liveCells.size() && a->randInt == b->randInt) return a->tertiary < b->tertiary;
		else if (a->liveCells.size() == b->liveCells.size()) return a->randInt < b->randInt;
		else return a->liveCells.size() < b->liveCells.size();
		//return a < b;
	}
};

std::set<Group*, GroupComparator> allGroups;

class Cell {
public:
	Group* G = nullptr;		//points to group that this cell belongs to
	Cell* right = nullptr;
	Cell* left = nullptr;
	Cell* up = nullptr;
	Cell* down = nullptr;
	int x, y;	//x goes top down, y goes left right
	Cell(int x, int y) : x(x), y(y) {}
	void fix() {
		if (right != nullptr) right->left = this;
		if (down != nullptr) {
			down->up = this;
			if (right != nullptr) right->down = down->right;
		}
	}
	void fill() {	//only changes image and nothing else!!!
		std::set<Group*> sg;
		if (up != nullptr) sg.insert(up->G);
		if (down != nullptr) sg.insert(down->G);
		if (left != nullptr) sg.insert(left->G);
		if (right != nullptr) sg.insert(right->G);
		if (sg.size() == 1) {
			image.at<cv::Vec3b>(x, y) = cv::Vec3b(255, 255, 255);
		}
	}
	void makeGroup() {
		if ((x % 2 == 0 && y % 2 == 0) || (x % 2 != 0 && y % 2 != 0)) {
			G = new Group();
			G->liveCells.insert(this);
			G->perimeterCells.insert(up);
			G->perimeterCells.insert(down);
			G->perimeterCells.insert(left);
			G->perimeterCells.insert(right);
			G->perimeterCells.erase(nullptr);
			allGroups.insert(G);
			image.at<cv::Vec3b>(x, y) = cv::Vec3b(255, 255, 255);
		}
	}
	void makeAlive();
};

void Cell::makeAlive() {

	// finding surrounding groups
	std::set<Group*> sg;
	if (up != nullptr) sg.insert(up->G);
	if (down != nullptr) sg.insert(down->G);
	if (left != nullptr) sg.insert(left->G);
	if (right != nullptr) sg.insert(right->G);

	// since all these groups could possibly be edited
	// (it's a horrible idea to edit something in a BST with custom sorting, because what you change could affect where it should be in the tree)
	// removing from allGroups
	for (Group* group : sg) {
		allGroups.erase(group);
	}

	// merge groups until 1 remains
	while (sg.size() > 1) {
		auto it = sg.begin();
		Group* a = *it;
		it++;
		Group* b = *it;
		if (a->liveCells.size() < b->liveCells.size()) std::swap(a, b);
		for (Cell* cell : b->liveCells) {
			a->liveCells.insert(cell);
			cell->G = a;
		}
		for (Cell* cell : b->perimeterCells) a->perimeterCells.insert(cell);
		delete b;
		sg.erase(b);
	}

	// once this dead perimeter cell is in contact with only one group
	(*sg.begin())->perimeterCells.erase(this);
	(*sg.begin())->liveCells.insert(this);
	allGroups.insert((*sg.begin()));
	G = (*sg.begin());
	image.at<cv::Vec3b>(x, y) = cv::Vec3b(255, 255, 255);

}

int main() {
	srand(time(nullptr));
	int width, height;
	std::cout << "width in pixels: ";
	std::cin >> width;
	std::cout << "height in pixels: ";
	std::cin >> height;
	image = cv::Mat(height, width, CV_8UC3, cv::Scalar(0, 0, 0));

	//huge initialization phase
	Cell* grid = new Cell(-1, 0);	//used to easily go into the for loops
	Cell* gridStore = grid;
	for (int i = 0; i < height; i++) {	//creating the cells, not dealing with up/down/left/right pointers yet
		grid->down = new Cell(i, 0);
		grid = grid->down;
		Cell* temp = grid;
		for (int j = 0; j < width - 1; j++) {
			grid->right = new Cell(i, j + 1);
			grid = grid->right;
		}
		grid = temp;
	}
	grid = gridStore;
	for (int i = 0; i < height; i++) {	//dealing with all the pointers and creating groups
		grid = grid->down;
		grid->fix();
		grid->makeGroup();
		Cell* temp = grid;
		for (int j = 0; j < width - 1; j++) {
			grid = grid->right;
			grid->fix();
			grid->makeGroup();
		}
		grid = temp;
	}

	cv::namedWindow("Image", cv::WINDOW_NORMAL);
	cv::resizeWindow("Image", width * 4, height * 4);
	cv::imshow("Image", image);
	cv::waitKey(0);

	//grid->down->right->makeAlive();
	while (allGroups.size() > 2) {

		int lowestPotentialGroup = width * height + 1;
		Cell* bestCell = nullptr;
		for (Cell* cell : (*allGroups.begin())->perimeterCells) {
			int combinedGroupSize = 0;
			std::set<Group*> sg;
			if (cell->up != nullptr) sg.insert(cell->up->G);
			if (cell->down != nullptr) sg.insert(cell->down->G);
			if (cell->left != nullptr) sg.insert(cell->left->G);
			if (cell->right != nullptr) sg.insert(cell->right->G);
			for (Group* group : sg) {
				combinedGroupSize += group->liveCells.size();
			}
			if (combinedGroupSize < lowestPotentialGroup) {
				lowestPotentialGroup = combinedGroupSize;
				bestCell = cell;
			}
		}

		bestCell->makeAlive();

	}

	//image cleanup phase
	grid = gridStore;
	for (int i = 0; i < height; i++) {	//dealing with all the pointers and creating groups
		grid = grid->down;
		grid->fill();
		Cell* temp = grid;
		for (int j = 0; j < width - 1; j++) {
			grid = grid->right;
			grid->fill();
		}
		grid = temp;
	}

	cv::imwrite("output.png", image);
	cv::imshow("Image", image);
	cv::waitKey(0);

	cv::destroyWindow("Image");


	return 0;
}
