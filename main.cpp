#include<iostream>
#include<vector>
#include<thread>
#include<mutex>
#include<stdlib.h>
#include<condition_variable>
#include <time.h>
#include<string>
#include <ctime>
using namespace std;
int cur_time = 0;
vector<int> Global_buffer(5);
const vector<int> restraints{5,5,4,3,3};
const vector<int> partW_manufacture_time{50, 50, 60, 60, 70};
const vector<int> move_time{20, 20, 30, 30,40};
const vector<int> prodW_assemble_time{60, 60, 70, 70, 80};
const vector<string> status_string{"New Load Order","New Pickup Order",
								   "Wakeup-Notified","Wakeup-Timeout"};
const int MaxTimePart{1600};
const int MaxTimeProduct{2000};

mutex m1;
condition_variable partW, productW;

template<class T>
ostream& operator<<(ostream& str, const vector<T>& vec)
{
	str << "<";
	for (auto const& i : vec) {
		str << " " << i;
	}
	str << " >";
	return str;
}
vector<int> operator+(const vector<int>&order, const vector<int>& buffer_state){
	vector<int> res(order.size());
	for(int i=0;i<order.size();i++){
		res[i]=order[i]+buffer_state[i];
	}
	return res;
}
vector<int> operator-(const vector<int>& buffer_state, const vector<int>&order){
	vector<int> res(order.size());
	for(int i=0;i<order.size();i++){
		res[i]=buffer_state[i]-order[i];
	}
	return res;
}
int getSum(const vector<int>& vec){
	int sum=0;
	for(auto const& i:vec){
		sum+=i;
	}
	return sum;
}

vector<int> generate_load_order(vector<int>& order) {
	srand((unsigned) time(NULL));
	int curr_loadOrder_sum=getSum(order);
	for (int i = curr_loadOrder_sum; i < 5; i++) {
		int pos_to_add = rand() % 5;
		++order[pos_to_add];
	}
	return order;
}
int hasNumber_onSpot(const vector<int>& order){
	int hasNumber{0};
	for(const auto&i:order){
		if(i!=0){
			hasNumber++;
		}
	}
	return hasNumber;
}
vector<int> generate_pickup_order(vector<int>& order){
	srand((unsigned) time(NULL));
	vector<int> pos_hasNumber;
	int curr_pickupOrder_sum=getSum(order);
	for(int i=curr_pickupOrder_sum;i<5;i++){
		if(hasNumber_onSpot(order)==3){
			break;
		}
		++order[rand()%5];
	}
	if(getSum(order)!=5){
		for(int i=0;i<5;i++){
			if(order[i]!=0){
				pos_hasNumber.push_back(i);
			}
		}
	}
	//cout<< "position has num: "<<pos_hasNumber<<"size is: "<<pos_hasNumber.size()<<endl;
	while(getSum(order)!=5){
		//++order[pos_hasNumber[rand()%pos_hasNumber.size()]];
		int pos_to_add = rand()%5;
		for(auto const&i:pos_hasNumber){
			if(pos_to_add==i){
				++order[pos_to_add];
			}
		}
	}
	return order;
}
bool has_left(const vector<int>&vec){
	for(auto const&i:vec){
		if(i!=0){
			return true;
		}
	}
	return false;
}
bool is_theSame(const vector<int>& vec1, const vector<int>& vec2)
{
	if (vec1.size()!=vec2.size()) {
		return false;
	}
	for (int i = 0; i<vec1.size(); i++) {
		if (vec1[i]!=vec2[i]) {
			return false;
		}
	}
	return true;
}
bool is_initial(const vector<int>& checking_vec){
	for(auto const&i: checking_vec){
		if(i!=0){
			return false;
		}
	}
	return true;
}
bool is_overFlow(const vector<int>& order, const vector<int>& buffer_state){
	vector<int> res=order+buffer_state;
	for(int i=0;i<order.size();i++){
		if(res[i]>restraints[i]){
			return true;
		}
	}
	return false;
}
bool is_notEnough(const vector<int>& buffer_state, const vector<int>& order){
	vector<int> res=buffer_state-order;
	for(int i=0;i<order.size();i++){
		if(res[i]<0){
			return true;
		}
	}
	return false;
}
//bool partW_pred(){
//	vector<int> check;
//	for (int i = 0; i<5;i++) {
//		if(Global_buffer[i]==restraints[i])
//			check.push_back(i);
//		//cannot be put in, find those slots are full
//	}
//	for(auto const &i:check){
//		//those slots are avail, return true
//		if(Global_buffer[i]<restraints[i]){
//			return true;
//		}
//	}
//	return false;
//}
//bool productW_pred(){
//	vector<int> check;
//	for (int i = 0; i<5;i++) {
//		if(Global_buffer[i]==0){
//			//where cannot withdraw anymore
//			check.push_back(i);
//		}
//	}
//	for(auto const&i:check){
//		if (Global_buffer[i]>0) {
//			return true;
//		}
//	}
//	return false;
//}
//bool time_exceed(int wait_time){//to set a timer
//	auto begin_time = chrono::system_clock::now();
//	while(1){
//		if(chrono::system_clock::now()>begin_time+chrono::microseconds (wait_time)){
////			auto elapsed_time = chrono::system_clock::now()-begin_time;
////			cout << "elapased time: " << elapsed_time.count();
//			break;
//		}
//	}
//	return true;
//}
auto start_tp = chrono::steady_clock::now();
void part_worker(int id)
{

	vector<int> load_order(5);
	for (int i = 0; i<5; i++) {//5 interations
		unique_lock<mutex> u1(m1);
//		auto iteration_begin = chrono::system_clock::now();
//		cout << "Iteration " << i << endl;
//		cout << "Part Worker ID: " << id << endl;
		vector<int> save_loadOrder = load_order;

		load_order = generate_load_order(load_order);
		vector<int> printout_loadOrder = load_order;
		vector<int> printout_Buffer = Global_buffer;
		for (int i = 0; i<5; i++) {
			//to manufacture parts
			if (load_order[i]!=save_loadOrder[i]) {
				int sleep_time = (load_order[i]-save_loadOrder[i])*partW_manufacture_time[i];
				this_thread::sleep_for(chrono::microseconds(sleep_time));
			}
		}
		vector<int> tmp;
//		cout << "Load Order: " << load_order << endl;
//		cout << "Buffer State: " << Global_buffer << endl;
		if (is_overFlow(Global_buffer, load_order)==false) {
			Global_buffer = Global_buffer+load_order;
			for (int i = 0; i<5; i++) {
				if (load_order[i]!=0) {
					int sleep_time = load_order[i]*move_time[i];
					this_thread::sleep_for(chrono::microseconds(sleep_time));
				}
			}
			load_order = { 0, 0, 0, 0, 0 };
		}
		//

		else {//overflow_
			tmp = load_order+Global_buffer;
			for (int i = 0; i<5; i++) {
				if (tmp[i]>restraints[i]) {
					int sleep_time = (restraints[i]-Global_buffer[i])*move_time[i];
					this_thread::sleep_for(chrono::microseconds(sleep_time));
					Global_buffer[i] = restraints[i];
					load_order[i] = tmp[i]-restraints[i];
				}
				else {
					int sleep_time = (tmp[i]-Global_buffer[i])*move_time[i];
					this_thread::sleep_for(chrono::microseconds(sleep_time));
					Global_buffer[i] = tmp[i];
					load_order[i] = 0;
				}
			}
		}
//		cout << "Updated Buffer State: " << Global_buffer << endl;
//		cout << "Update Load Order: " << load_order << endl;
		auto myPred=[&load_order]{
		  for (int i = 0; i<5;i++) {
		  	if(load_order[i]!=0&&Global_buffer[i]<restraints[i]){
		  		return true;
		  	}
		  }
		  return false;
		};
		int copy_MaxTimePart = MaxTimePart;
		if (has_left(load_order)) {
			auto begin_time = chrono::steady_clock::now();//start the timer
			while (1) {
				auto cycle_begin = chrono::steady_clock::now();
				if (has_left(load_order) && partW.wait_for(u1, chrono::microseconds(copy_MaxTimePart), myPred)) {
					//while load_order is not empty also part_worker is not blocked when
					//global_buffer has avail spot to place part
//					cout << "something running partWorker" << endl;
					productW.notify_all();
					for (int i = 0; i<5;i++) {
						if(load_order[i]>0&&Global_buffer[i]<restraints[i]){
							//if we could possibly put one part in
							--load_order[i];
							++Global_buffer[i];

							this_thread::sleep_for(chrono::microseconds(move_time[i]));
						}
					}
				}
				auto cycle_end = chrono::steady_clock::now();
				auto cycle_time = cycle_end-cycle_begin;
				auto cycle_elapsed = chrono::duration_cast<chrono::microseconds>(cycle_time);
				copy_MaxTimePart -= cycle_elapsed.count();
				//cout << "cycle_elapsed.count() " << cycle_elapsed.count()<< endl;
				if (chrono::steady_clock::now()>begin_time+chrono::microseconds(MaxTimePart)) {
					//timeout, break
					break;
				}
			}
		}
		for (int i = 0; i<5;i++) {
			//move the remain parts back
			if(load_order[i]!=0){
				this_thread::sleep_for(chrono::microseconds(load_order[i]*move_time[i]));
			}
		}
//		auto iteration_end = chrono::system_clock::now();
//		auto iteration_time= iteration_end-iteration_begin;
//		auto iteration_elapsed = chrono::duration_cast<chrono::seconds>(iteration_time);
//		cur_time += iteration_elapsed.count();
		auto current_tp = chrono::steady_clock::now();
		cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
		cout << "Part_worker ID: " << id << ", iteration " << i << endl;
		if(is_initial(save_loadOrder)){
			cout << "Status: " << status_string[0] << endl;
		}
		else{
			cout << "Status: " << status_string[0] << " from Partial Order " << save_loadOrder << endl;
		}
		cout << "Accumulated wait time: " << MaxTimePart-copy_MaxTimePart << endl;
		cout << "Load_order is: " << printout_loadOrder << endl;
		cout << "Buffer State: " << printout_Buffer << endl;
		cout << "Updated Buffer State: " << Global_buffer << endl;
		cout << "Update Load Order: " << load_order << endl << endl;
	}
}
void product_worker(int id){
	vector<int> pickup_order(5);
	for (int i = 0; i<5;i++) {
		unique_lock<mutex> u1(m1);
//		auto iteration_begin = chrono::system_clock::now();
//		cout << "Iteration " << i << endl;
//		cout << "Product Worker ID: " << id << endl;
		vector<int> save_pickupOrder = pickup_order;

		pickup_order = generate_pickup_order(pickup_order);
		vector<int> printout_pickupOrder = pickup_order;
		vector<int> printout_Buffer = Global_buffer;
//		cout<<"Pickup Order: "<<pickup_order<<endl;
//		cout << "Buffer State: " << Global_buffer << endl;
		vector<int> tmp;
		if(is_notEnough(Global_buffer,pickup_order)==false){
			Global_buffer = Global_buffer-pickup_order;
			pickup_order={0,0,0,0,0};
			for (int i = 0; i<5;i++) {
				//move also assemble
				if(pickup_order[i]!=0){
					this_thread::sleep_for(chrono::microseconds(move_time[i]+prodW_assemble_time[i]));

				}
			}
		}
		else{
			tmp = Global_buffer-pickup_order;
			for (int i = 0; i<5;i++) {
				if(tmp[i]<0){
					int sleep_time = (Global_buffer[i])*(move_time[i]+prodW_assemble_time[i]);
					this_thread::sleep_for(chrono::microseconds(sleep_time));
					Global_buffer[i] = 0;
					pickup_order[i] = (-tmp[i]);
				}
				else{
					int sleep_time = pickup_order[i]*(move_time[i]+prodW_assemble_time[i]);
					this_thread::sleep_for(chrono::microseconds(sleep_time));
					Global_buffer[i] = tmp[i];
					pickup_order[i] = 0;
				}
			}
		}
//		cout<<"Updated pickup order: "<<pickup_order<<endl;
//		cout << "Updated Buffer State: " << Global_buffer << endl<<endl;
		auto mypred=[&pickup_order]{
		  for (int i = 0; i<5;i++) {
		  	if(pickup_order[i]>0&&Global_buffer[i]>0){
		  		return true;
		  	}
		  }
		  return false;
		};
		int copy_MaxTimeProduct = MaxTimeProduct;
		if(has_left(pickup_order)){
			auto begin_time = chrono::steady_clock::now();
			while(1){
				auto cycle_begin = chrono::steady_clock::now();
				if(has_left(pickup_order)&&productW.wait_for(u1,chrono::microseconds(copy_MaxTimeProduct),mypred)){
					partW.notify_all();
//					cout << "something running prodw" << endl;
					for (int i = 0; i<5;i++) {
						if(pickup_order[i]>0&&Global_buffer[i]>0){
							--pickup_order[i];
							--Global_buffer[i];

							this_thread::sleep_for(chrono::microseconds(move_time[i]+prodW_assemble_time[i]));
						}
					}
				}
				auto cycle_end = chrono::steady_clock::now();
				auto cycle_time = cycle_end-cycle_begin;
				auto cycle_elapsed = chrono::duration_cast<chrono::microseconds>(cycle_time);
				copy_MaxTimeProduct -= cycle_elapsed.count();

				if(chrono::steady_clock::now()>begin_time+chrono::microseconds(MaxTimeProduct)){
					break;
				}
			}
		}
//		auto iteration_end = chrono::system_clock::now();
//		auto iteration_time= iteration_end-iteration_begin;
//		auto iteration_elapsed = chrono::duration_cast<chrono::seconds>(iteration_time);
//		cur_time += iteration_elapsed.count();
		auto current_tp = chrono::steady_clock::now();
		cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
		if(is_initial(save_pickupOrder)){
			cout << "Status: " << status_string[0] << endl;
		}
		else{
			cout << "Status: " << status_string[0] << " from Partial Order " << save_pickupOrder << endl;
		}
		cout << "Accumulated wait time: " << MaxTimeProduct-copy_MaxTimeProduct << endl;
		cout << "Product_worker ID: " << id << ", itertaion " << i << endl;
		cout<<"Pickup Order: "<<printout_pickupOrder<<endl;
		cout << "Buffer State: " << printout_Buffer << endl;
		cout<<"Updated pickup order: "<<pickup_order<<endl;
		cout << "Updated Buffer State: " << Global_buffer << endl<<endl;
	}
	//pickup move back time?
}
int main()
{
	const int m = 20, n = 16; //m: number of Part Workers

	thread partW[m];
	thread prodW[n];
	for (int i = 0; i < n; i++) {
		partW[i] = thread(part_worker, i);
		prodW[i] = thread (product_worker, i);
	}
	for (int i = n; i < m; i++) {
		partW[i] = thread(part_worker, i);
	}
	/* Join the threads to the main threads */
	for (int i = 0; i < n; i++) {
		partW[i].join();
		prodW[i].join();
	}
	for (int i = n; i < m; i++) {
		partW[i].join();
	}
	cout << "Finish!" << endl;
	return 0;

}
