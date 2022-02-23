#include <bits/stdc++.h>
using namespace std;

/*
    Struct for storing the information regarding an object 
    @start: starting angle of the object
    @end: ending angle of the object
*/
struct intervals {
    int start;
    int end;    
};

void merge(intervals arr[], int n, int l, int r) {
    int mid = (l + r) >> 1;
    intervals arr1[mid - l + 1], arr2[r - mid];
    for (int i = l; i <= mid; i++) {
        arr1[i - l].start = arr[i].start;
        arr1[i - l].end = arr[i].end;
    }
    for (int i = mid + 1; i <= r; i++) {
        arr2[i - mid - 1].start = arr[i].start;
        arr2[i - mid - 1].end = arr[i].end;
    }
    int i = 0;
    int j = 0;
    int l1 = mid - l + 1;
    int l2 = r - mid;
    int idx = l;
    while (i < l1 && j < l2) {
        if (arr1[i].end < arr2[j].end) {
            arr[idx].start = arr1[i].start;
            arr[idx].end = arr1[i].end;
            i++;
        } else {
            arr[idx].start = arr2[j].start;
            arr[idx].end = arr2[j].end;
            j++;
        }
        idx++;
    }
    while (i < l1) {
        arr[idx].start = arr1[i].start;
        arr[idx].end = arr1[i].end;
        i++;
        idx++;
    }
    while (j < l2) {
        arr[idx].start = arr2[j].start;
        arr[idx].end = arr2[j].end;
        j++;
        idx++;
    }
}

void mergesort(intervals arr[], int n, int l, int r) {
    if (l >= r)
        return;
    int mid = (l + r) >> 1;
    mergesort(arr, n, l, mid);
    mergesort(arr, n, mid + 1, r);
    merge(arr, n, l, r);
}

int main() {
    int n;
    cout << "Write the number of objects: ";
    cin >> n;
    intervals *intervals_array = new intervals[n];                                  
    for (int i = 0; i < n; i++) {
        cout << "Start of object " << i + 1 << ": ";
        cin >> intervals_array[i].start;
        cout << "End of object " << i + 1 << ": ";
        cin >> intervals_array[i].end;
    }

    mergesort(intervals_array, n, 0, n - 1);

    int count = 0, best_count = 0, best_i = 0;
    /*
        Usually in a linear problem where we have to find maximum number of disjoint intervals
        we would just sort by the order of end times and take the first interval and then select 
        further intervals which have a start time greater than the end time of the previous interval.

        In this problem because the intervals are circular, we cannot be sure as to what will be the 
        starting index, so we iterate over the start index of the intervals in sorted oder and check
        which starting index gives us maximum number of disjoint intervals.

        Note: [l1, r1] and [l2, r2] are considered overlapping if r1 = l2
    */
    for(int i = 0; i < n; i++){
        count = 0;
        count++;
        int last = i;
        for(int j = i + 1; j < n; j++){
            if(intervals_array[i].start > intervals_array[i].end && intervals_array[j].end >= intervals_array[i].start){
                break;
            }
            if(intervals_array[j].start > intervals_array[last].end && intervals_array[j].start < intervals_array[j].end){
                last = j;
                count++;
            }
        }
        if(count > best_count){
            best_count = count;
            best_i = i;
        }
        if(intervals_array[i].start < intervals_array[i].end){
            break;
        }
    }
    cout << "\nObjects selected: " << endl;
    cout << "Start: " << intervals_array[best_i].start << " End: " << intervals_array[best_i].end << endl;
    int last = best_i;
    for(int j = best_i + 1; j < n; j++){
        if(intervals_array[best_i].start > intervals_array[best_i].end && intervals_array[j].end >= intervals_array[best_i].start){
            break;
        }
        if(intervals_array[j].start > intervals_array[last].end && intervals_array[j].start < intervals_array[j].end){
            last = j;
            cout << "Start: " << intervals_array[j].start << " End: " << intervals_array[j].end << endl;
        }
    }
}