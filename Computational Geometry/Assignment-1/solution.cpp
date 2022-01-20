#include <bits/stdc++.h>  
using namespace std;

#define pi atan2(0, -1)                                                     // Defining the value of pi 

struct point {                                                              // Defining the structure of a point
    double x, y;                                                            // x and y coordinates of a point
    point(double x = 0, double y = 0) : x(x), y(y) {}                       // Constructor
    point(const point &p) : x(p.x), y(p.y) {}                               // Copy Constructor

    friend ostream &operator<<(ostream &os, const point &p) {               // Overloading the << operator
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }
};

double polar_angle(point p, point q){                                       // Function to calculate the polar angle of a point
    double angle = atan2(q.y - p.y, q.x - p.x);                             // Calculating the angle using atan2
    if(angle < 0) angle += 2 * pi;                                          // If the angle is less than 0, adding 2*pi to it
    return angle;
}

int orientation(point p, point q, point r){                                 // Function to calculate the orientation of a point             
    double val = (r.y - p.y) * (r.x - q.x) - (r.x - p.x) * (r.y - q.y);     // Calculating the value of the determinant
    if(fabs(val) < 1e-9) return 0;                                          // If the value is very small, returning 0 (point lies on the line)
    return (val > 0) ? -1 : 1;                                              // If the value is positive, returning -1 (point lies to the left of the line) else 1 (point lies to the right of the line)
}

struct polygon {                                                            // Defining the structure of a polygon
    int n;                                                                  // Number of vertices
    vector<point> p;                                                        // Vector of points
    vector<double> angles;                                                  // Vector of polar angles
    point internal;                                                         // Internal point of the polygon
    int low_index;                                                          // Index of the point with lowest polar angle

    polygon(int n = 0) : n(n), p(n), angles(n) {}                           // Constructor
    polygon(vector<point> p, int n = 0) : n(n), p(p), angles(n, -100) {}    // Constructor
    polygon(const polygon &p) : n(p.n), p(p.p), angles(p.angles) {}         // Copy Constructor

    friend ostream &operator<<(ostream &os, const polygon &p) {             // Overloading the << operator
        os << "{";
        for (int i = 0; i < p.n; i++) {
            os << p.p[i];
            if (i != p.n - 1) os << ", ";
        }
        os << "}";
        return os;
    }

    void set_internal() {                                                   // Function to set the internal point of the polygon    
        double x = 0, y = 0;
        for (int i = 0; i < 3; i++) {                                       
            x += p[i].x;
            y += p[i].y;
        }
        internal = point(x / 3, y / 3);                                     // Finding the centroid of 3 consecutive vertices
    }

    double get_angle(int index){                                            // Function to get the polar angle of a point on a give index
        if(angles[index] != -100)                                           // If the angle is already calculated, returning it
            return angles[index];
        double angle = polar_angle(internal, p[index]);                     // Calculating the angle using polar_angle
        return (angles[index] = angle);                                     // Returning the angle
    }   

    double set_min(int low, int high){                                      // Function to get the vertex of point with minimum angle
        if (high < low)  return 0;                                          // Array was already sorted 

        if (high == low) return low;                                        // If there is only one vertex, returning it

        int mid = low + (high - low)/2;                                     // Finding the midpoint of the array

        if (mid < high && get_angle(mid+1) < get_angle(mid))                // If the next point has smaller angle, returning it
            return (mid+1);

        if (mid > low && get_angle(mid) < get_angle(mid-1))                 // If the previous point has greater angle, returning it
            return mid;

        if (get_angle(high) > get_angle(mid))                               // Move towards left half 
            return set_min(low, mid-1);
            
        return set_min(mid+1, high);                                        // Move towards right half
    }

    void pre_process() {                                                    // Function to pre-process the polygon
        set_internal();                                                     // Setting the internal point of the polygon
        low_index = set_min(0, n-1);                                        // Setting the index of the point with minimum angle
    }

    int find_point_position(point query){                                   // Function to find if a point is inside the polygon
        double p_angle = polar_angle(internal, query);                      // Calculating the polar angle of the query point

        int p_index = -1;
        if(p_angle <= get_angle(low_index) || p_angle > get_angle((low_index - 1 + n)%n)){    // If polar angle of the query point is less than the minimum angle or greater than the maximum angle
            p_index = low_index;
        }
        if(p_index == -1){                                                  // Binary seach for the polar angle of the query point
            int lo = 0, hi = n-1, mid;
            while(lo <= hi){
                mid = lo + (hi - lo) / 2;                                   // Finding the midpoint of the array
                if(p_angle <= get_angle((mid + low_index)%n) && p_angle > get_angle((mid + low_index + n - 1)%n)){   // Note that we shift the index by low_index because the array is rotated by low_index
                    p_index = mid;                                          // If the polar angle of the query point is less than the midpoint and greater than the previous midpoint, returning it
                    break;
                }
                else if(p_angle < get_angle((mid + low_index)%n))           // If the polar angle of the query point is less than the midpoint, move towards left half
                    hi = mid - 1;
                else                                                        // If the polar angle of the query point is greater than the midpoint, move towards right half
                    lo = mid + 1;               
            }
            p_index = (p_index + low_index) % n;
        }

        return orientation(p[(p_index - 1 + n)% n], p[p_index], query);     // Finding the orientation of the query point and returing the result
    }
};

double rand_double(double arg_min, double arg_max){                         // Function to generate a random double between two values
    double f = (double)rand() / RAND_MAX;                   
    return arg_min + f * (arg_max - arg_min);
}

polygon generate_random_polygon(int n){                                     // Function to generate a random convex polygon
    double r = rand_double(150, 200);                                       // Radius of the circle
    set<double> angles;                                                     // Set of angles
    while(angles.size() < n){                                               // Generating n random angles
        double angle = rand_double(0, 2 * pi);                              // Generating a random angle
        angles.insert(angle);                                               
    }
    double x = rand_double(210, 290);                                       // X cordinate of the center of the circle
    double y = rand_double(210, 290);                                       // Y cordinate of the center of the circle
    vector<point> p;                                                        
    for(auto ang: angles){
        p.push_back(point(x + r * cos(ang), y + r * sin(ang)));             // Generating the points on the circle    
    }
    int ramt = rand() % n;                                                  
    rotate(p.begin(), p.begin()+ramt, p.end());                             // Rotating the points by some random amount
    return polygon(p, n);                                                   // Returning the polygon
}


// THE FUNCTIONS BELOW ARE USED TO GENERATE THE SVG FILE

void header(ofstream &file) {
    file << "<?xml version=\"1.0\" standalone=\"no\" ?>" << endl;
    file << "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" \"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">" << endl;
}

void startSVG(ofstream &file, int width, int height) {
    file << "<svg width=\"" << width << "px\" height = \"" << height << "px\" xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">" << endl;
}

void endSVG(ofstream &file) {
    file << "</svg>" << endl;
}

void displayPoint(ofstream &file, const point &p, string col) {
    file << "\t<circle cx=\"" << p.x << "\" cy=\"" << p.y << "\" r=\"2\" stroke=\"" << col << "\" stroke-width=\"2\" fill=\"" << col <<"\"/>" << endl;
}

void displayPolygon(ofstream &file, const polygon &poly) {
    for(int i = 0; i < poly.n; i++) {
        file << "\t<circle cx=\"" << poly.p[i].x << "\" cy=\"" << poly.p[i].y << "\" r=\"2\" stroke=\"black\" stroke-width=\"2\"/>" << endl;
    }
    for(int i = 0; i < poly.n; i++) {
        file << "\t<line x1=\"" << poly.p[i].x << "\" y1=\"" << poly.p[i].y << "\" x2=\"" << poly.p[(i + 1) % poly.n].x << "\" y2=\"" << poly.p[(i + 1) % poly.n].y << "\" stroke=\"black\" stroke-width=\"1\"/>" << endl;
    }
    // for(int i = 0; i < poly.n; i++) {
    //     file << "\t<line x1=\"" << poly.p[i].x << "\" y1=\"" << poly.p[i].y << "\" x2=\"" << poly.internal.x << "\" y2=\"" << poly.internal.y << "\" stroke=\"black\" stroke-width=\"1\"/>" << endl;
    // }
}


int main() {
    // srand(time(0));
    vector <int> sides = {5, 8, 15, 23, 30};                                    // The number of sides of the polygon
    for(auto num: sides){                                                       // For each number of sides
        polygon poly = generate_random_polygon(num);                            // Generating a random convex polygon
        poly.pre_process();                                                     // Preprocessing the polygon

        ofstream file("figure_" + to_string(num) + ".svg");                     // Creating the SVG file
        header(file);                                                           // Writing the header    
        startSVG(file, 500, 500);                                               // Writing the start of the SVG file
        displayPolygon(file, poly);                                             // Displaying the polygon

        vector <point> points;                                                  // Vector of points to be queried  

        for(int i = 0; i<poly.n; i+=2){                                         // First we take a few points on the boundary of the polygon 
            points.push_back(point((poly.p[i].x + poly.p[(i + 1) % poly.n].x) / 2, (poly.p[i].y + poly.p[(i + 1) % poly.n].y) / 2));
        }

        int num_points = rand() % 50 + 100;                                     // Adding random number of points to the query set
        for(int i = 0; i<num_points; i++){                                      // Generating random points
            points.push_back(point(rand_double(0, 500), rand_double(0, 500)));
        }

        for(auto points_iter: points){
            int val = poly.find_point_position(points_iter);                    // Finding the position of the point in the polygon
            if(val == 0)                                                        // If the point is on the boundary
                displayPoint(file, points_iter, "blue");    
            else if(val == 1)                                                   // If the point is inside the polygon
                displayPoint(file, points_iter, "green");
            else                                                                // If the point is outside the polygon
                displayPoint(file, points_iter, "red");
        }

        endSVG(file);
    }
}

