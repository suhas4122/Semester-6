#include <bits/stdc++.h>  // This will work only for g++ compiler. 
using namespace std;

string to_string(string s) { return '"' + s + '"';}
string to_string(char s) { return string(1, s);}
string to_string(const char* s) { return to_string((string) s);}
string to_string(bool b) { return (b ? "true" : "false");}
template <typename A> string to_string(A);
template <typename A, typename B>string to_string(pair<A, B> p) {return "(" + to_string(p.first) + ", " + to_string(p.second) + ")";}
template <typename A> string to_string(A v) {bool f = 1; string r = "{"; for (const auto &x : v) {if (!f)r += ", "; f = 0; r += to_string(x);} return r + "}";}
void debug_out() { cerr << endl; }
template <typename Head, typename... Tail> void debug_out(Head H, Tail... T) {cerr << " " << to_string(H); debug_out(T...);}
#define pr(...) cerr << "[" << #__VA_ARGS__ << "] :", debug_out(__VA_ARGS__)
#define debarr(a, n) cerr << #a << " : "; for(int i = 0; i < n; i++) cerr << a[i] << " "; cerr << endl;
#define debmat(mat, row, col) cerr << #mat << " :\n"; for(int i = 0; i < row; i++){for(int j = 0; j < col; j++) cerr << mat[i][j] << " "; cerr << endl;}

#define pi atan2(0, -1)

struct point {
    double x, y;
    point(double x = 0, double y = 0) : x(x), y(y) {}
    point(const point &p) : x(p.x), y(p.y) {}

    friend ostream &operator<<(ostream &os, const point &p) {
        os << "(" << p.x << ", " << p.y << ")";
        return os;
    }
};

double polar_angle(point p, point q){
    double angle = atan2(q.y - p.y, q.x - p.x);
    if(angle < 0) angle += 2 * pi;
    return angle;
}

int orientation(point p, point q, point r){
    double val = (r.y - p.y) * (r.x - q.x) - (r.x - p.x) * (r.y - q.y);
    if(fabs(val) < 1e-9) return 0;
    return (val > 0) ? -1 : 1;
}

struct polygon {
    int n;
    vector<point> p;
    vector<double> angles;
    point internal;
    int low_index;

    polygon(int n = 0) : n(n), p(n), angles(n) {}
    polygon(vector<point> p, int n = 0) : n(n), p(p), angles(n, -100) {}
    polygon(const polygon &p) : n(p.n), p(p.p), angles(p.angles) {}

    friend ostream &operator<<(ostream &os, const polygon &p) {
        os << "{";
        for (int i = 0; i < p.n; i++) {
            os << p.p[i];
            if (i != p.n - 1) os << ", ";
        }
        os << "}";
        return os;
    }

    void set_internal() {
        double x = 0, y = 0;
        for (int i = 0; i < 3; i++) {
            x += p[i].x;
            y += p[i].y;
        }
        internal = point(x / 3, y / 3);
    }

    double get_angle(int index){
        if(angles[index] != -100)
            return angles[index];
        double angle = polar_angle(internal, p[index]);
        return (angles[index] = angle);
    }   

    double set_min(int low, int high){
        if (high < low)  return 0;

        if (high == low) return low;

        int mid = low + (high - low)/2; 

        if (mid < high && get_angle(mid+1) < get_angle(mid))
            return (mid+1);

        if (mid > low && get_angle(mid) < get_angle(mid-1))
            return mid;

        if (get_angle(high) > get_angle(mid))
            return set_min(low, mid-1);
            
        return set_min(mid+1, high);
    }

    void pre_process() {
        set_internal();
        low_index = set_min(0, n-1);
        for(int i = 0; i < n; i++){
            cout << get_angle(i) << " ";
        }
        cout << endl;
    }

    int find_point_position(point query){
        double p_angle = polar_angle(internal, query);
        // cout << "p_angle: " << p_angle << endl;

        int p_index = -1;
        if(p_angle <= get_angle(low_index) || p_angle > get_angle((low_index - 1 + n)%n)){
            p_index = low_index;
        }
        // pr(get_angle(low_index), get_angle((low_index - 1 + n)%n));
        // pr(p_index);
        if(p_index == -1){
            int lo = 0, hi = n-1, mid;
            while(lo <= hi){
                // cout << lo << " " << hi << endl;
                mid = lo + (hi - lo) / 2;
                if(p_angle <= get_angle((mid + low_index)%n) && p_angle > get_angle((mid + low_index + n - 1)%n)){
                    p_index = mid;
                    break;
                }
                else if(p_angle < get_angle((mid + low_index)%n))
                    hi = mid - 1;
                else
                    lo = mid + 1;
            }
            p_index = (p_index + low_index) % n;
        }

        // cout << "p_index: " << p_index << endl;
        return orientation(p[(p_index - 1 + n)% n], p[p_index], query);
    }
};

double rand_double(double arg_min, double arg_max)
{
    double f = (double)rand() / RAND_MAX;
    return arg_min + f * (arg_max - arg_min);
}

polygon generate_random_polygon(int n){
    double r = rand_double(100, 200);
    // cout << "Radius: " << r << endl;
    set<double> angles;
    while(angles.size() < n){
        double angle = rand_double(0, 2 * pi);
        angles.insert(angle);
    }
    double x = rand_double(210, 290);
    double y = rand_double(210, 290);
    vector<point> p;
    for(auto ang: angles){
        p.push_back(point(x + r * cos(ang), y + r * sin(ang)));
    }
    int ramt = rand() % n;
    rotate(p.begin(), p.begin()+ramt, p.end());
    return polygon(p, n);
}

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
    // file << "\t<text x=\"" << p.x + 2 << "\" y=\"" << p.y << "\" fill=\"black\" font-size=\"10\">" << p.x << "," << p.y << "</text>" << endl;
}

void displayPolygon(ofstream &file, const polygon &poly) {
    for(int i = 0; i < poly.n; i++) {
        file << "\t<circle cx=\"" << poly.p[i].x << "\" cy=\"" << poly.p[i].y << "\" r=\"2\" stroke=\"black\" stroke-width=\"2\"/>" << endl;
        // file << "\t<text x=\"" << poly.p[i].x << "\" y=\"" << poly.p[i].y << "\" fill=\"black\" font-size=\"10\">" << poly.p[i].x << "," << poly.p[i].y << "</text>" << endl;
    }
    for(int i = 0; i < poly.n; i++) {
        file << "\t<line x1=\"" << poly.p[i].x << "\" y1=\"" << poly.p[i].y << "\" x2=\"" << poly.p[(i + 1) % poly.n].x << "\" y2=\"" << poly.p[(i + 1) % poly.n].y << "\" stroke=\"black\" stroke-width=\"1\"/>" << endl;
    }
    for(int i = 0; i < poly.n; i++) {
        file << "\t<line x1=\"" << poly.p[i].x << "\" y1=\"" << poly.p[i].y << "\" x2=\"" << poly.internal.x << "\" y2=\"" << poly.internal.y << "\" stroke=\"black\" stroke-width=\"1\"/>" << endl;
    }
}

int main() {
    srand(time(0));
    polygon poly = generate_random_polygon(8);
    poly.pre_process();
    // cout << poly << endl; 
    // cout << poly.internal << endl;
    // cout << "low_index: " << poly.low_index << "\n\n";

    ofstream file("demo.svg");
    header(file);
    startSVG(file, 500, 500);
    displayPolygon(file, poly);

    // for(int i = 0; i<11; i++){
    //     point p = point((poly.p[i].x + poly.p[(i + 1) % 12].x) / 2, (poly.p[i].y + poly.p[(i + 1) % 12].y) / 2);
    //     int val = poly.find_point_position(p);
    //     if(val == 0)
    //         displayPoint(file, p, "green");
    //     else if(val == 1)
    //         displayPoint(file, p, "red");
    //     else
    //         displayPoint(file, p, "blue");
    // }
    for(int i = 0; i<1; i++){
        point p = point(rand_double(0, 500), rand_double(0, 500));
        int val = poly.find_point_position(p);
        if(val == 0)
            displayPoint(file, p, "blue");
        else if(val == 1)
            displayPoint(file, p, "green");
        else
            displayPoint(file, p, "red");
    }

    endSVG(file);
}

