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
int total_completed = 0;
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
vector<int> generate_pickup_order(vector<int> order){
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
//		cout << "in begin" << endl;
//		cout << "Part Worker ID: " << id <<",Iteration " << i << endl;
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
		if(is_initial(load_order)) {

			cout << "Part Worker ID: " << id <<",Iteration " << i << endl;
			cout << "Accumulated Wait Time: 0us" << endl;
//			cout << "Status: "<<chrono::steady_clock::now-start_tp
			auto current_tp = chrono::steady_clock::now();
			if(is_initial(save_loadOrder)){
				cout << "New Load Order from Scratch" << endl;
			}
			else{
				cout << "New Load Order from Partial Order " << save_loadOrder << endl;
			}
			cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
			cout << "Load Order: " << printout_loadOrder << endl;
			cout << "Buffer State: " << printout_Buffer << endl;
			cout << "Updated Buffer State: " << Global_buffer << endl;
			cout << "Update Load Order: " << load_order << endl<<endl;
		}
		else {
			auto myPred = [&load_order] {
			  for (int i = 0; i<5; i++) {
				  if (load_order[i]!=0 && Global_buffer[i]<restraints[i]) {
					  return true;
				  }
			  }
			  return false;
			};
			int copy_MaxTimePart = MaxTimePart;
			vector<int> save_maxDeliver = load_order;

			auto begin_time = chrono::steady_clock::now();//start the timer
			while (1) {
				auto cycle_begin = chrono::steady_clock::now();
				if (has_left(load_order) && partW.wait_for(u1, chrono::microseconds(copy_MaxTimePart), myPred)) {
					//while load_order is not empty also part_worker is not blocked when
					//global_buffer has avail spot to place part
//					cout << "something running partWorker" << endl;
					//productW.notify_one();
					for (int i = 0; i<5; i++) {
						if (load_order[i]>0 && Global_buffer[i]<restraints[i]) {
							//if we could possibly put one part in
							--load_order[i];
							++Global_buffer[i];
							cout << "Part Worker ID: " << id <<",Iteration " << i << endl;
							cout << "Status: Wakeup-Notified" << endl;
							cout << "Accumulated Wait Time: " << endl;
							auto current_tp = chrono::steady_clock::now();
							cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
							cout << "Load Order: " << printout_loadOrder << endl;
							cout << "Buffer State: " << printout_Buffer << endl;
							cout << "Updated Buffer State: " << Global_buffer << endl;
							cout << "Update Load Order: " << load_order << endl<<endl;
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

					cout << "Part Worker ID: " << id <<",Iteration " << i << endl;
					cout << "Status: Wakeup-Timeout" << endl;
					auto current_tp = chrono::steady_clock::now();
					cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
					cout << "Load Order: " << printout_loadOrder << endl;
					cout << "Buffer State: " << printout_Buffer << endl;
					cout << "Updated Buffer State: " << Global_buffer << endl;
					cout << "Update Load Order: " << load_order << endl << endl;
					break;
				}
			}
//			cout << "breakout" << endl;
//			cout << "Part Worker ID: " << id <<",Iteration " << i << endl;
//			cout << "Updated Buffer State: " << Global_buffer << endl;
//			cout << "Update Load Order: " << load_order << endl << endl;

		}

		for (int i = 0; i<5;i++) {
			//move the remain parts back
			if(load_order[i]!=0){
				this_thread::sleep_for(chrono::microseconds(load_order[i]*move_time[i]));
			}
		}

	}
}
void product_worker(int id){

	vector<int> local_state(5);
	vector<int> cart_state(5);
	vector<int> pickup_order(5);
	for (int i = 0; i<5;i++) {
		unique_lock<mutex> u1(m1);
//		cout << "Product Worker ID: " << id <<",Iteration " << i << endl;

		pickup_order = generate_pickup_order(local_state);
		pickup_order = pickup_order-local_state;
		vector<int> printout_pickupOrder = pickup_order;
		vector<int> printout_Buffer = Global_buffer;
		vector<int> tmp;

		if(is_notEnough(Global_buffer,pickup_order)==false){
			//in this branch, we fetched all the requested pickups
			Global_buffer = Global_buffer-pickup_order;

			for (int i = 0; i<5;i++) {
				//since we have met the request of pickup order
				//update cart state_ according to pickup order
				if(pickup_order[i]!=0){
					this_thread::sleep_for(chrono::microseconds(move_time[i]*pickup_order[i]));
					cart_state[i] += pickup_order[i];
				}
			}
//			cout << "localState_before" << local_state << endl;
			local_state = local_state+cart_state;
//			cout << "localState_after" << local_state << endl;
			for (int i = 0; i<5;i++) {
				this_thread::sleep_for(chrono::microseconds (local_state[i]*prodW_assemble_time[i]));
			}

			pickup_order={0,0,0,0,0};
			local_state={0,0,0,0,0};
			cout << "Product Worker ID: " << id <<",Iteration " << i << endl;
			cout << "localState" << local_state << endl;
			auto current_tp = chrono::steady_clock::now();
			cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
			cout << "Status: is_initial Branch" << endl;
			cout<<"Pickup Order: "<<printout_pickupOrder<<endl;
			cout << "Buffer State: " << printout_Buffer << endl;
			cout << "Updated pickup order: " << pickup_order << endl;
			cout << "Updated Buffer State: " << Global_buffer << endl << endl;


		}

		else {
			tmp = Global_buffer-pickup_order;
			for (int i = 0; i<5;i++) {
				cart_state[i] = min(pickup_order[i], Global_buffer[i]);
				if(tmp[i]<0){
					//tmp res has some element smaller than 0
					//meaning, there are parts cannot be fetched
					int sleep_time = (Global_buffer[i])*(move_time[i]);
					this_thread::sleep_for(chrono::microseconds(sleep_time));
					Global_buffer[i] = 0;
					pickup_order[i] = (-tmp[i]);
				}
				else{
					//>=0, after minus there are stuff left in buffer
					//indicating what we fetch is the number of pickups
					int sleep_time = pickup_order[i]*(move_time[i]);
					this_thread::sleep_for(chrono::microseconds(sleep_time));
					Global_buffer[i] = tmp[i];
					pickup_order[i] = 0;
				}
			}
//			cout << "Product Worker ID: " << id <<",Iteration " << i << endl;
//			cout<<"Pickup Order: "<<printout_pickupOrder<<endl;
//			cout << "Buffer State: " << printout_Buffer << endl;
//			cout << "cart____" << cart_state << endl<<endl;
			auto mypred = [&pickup_order] {
			  for (int i = 0; i<5; i++) {
				  if (pickup_order[i]>0 && Global_buffer[i]>0) {
					  return true;
				  }
			  }
			  return false;
			};
			vector<int> save_maxDeliver = pickup_order;
			int copy_MaxTimeProduct = MaxTimeProduct;

			auto begin_time = chrono::steady_clock::now();
			while (1) {
				auto cycle_begin = chrono::steady_clock::now();
				if (has_left(pickup_order)
						&& productW.wait_for(u1, chrono::microseconds(copy_MaxTimeProduct), mypred)) {
					//partW.notify_one();
					for (int i = 0; i<5; i++) {
						if (pickup_order[i]>0 && Global_buffer[i]>0) {
//							cout << "cart__" << cart_state << endl;
							--pickup_order[i];
							--Global_buffer[i];
							++cart_state[i];
//							cout << "localState__" << local_state << endl;
//							cout << "cart__" << cart_state << endl;
							this_thread::sleep_for(chrono::microseconds(move_time[i]));
							vector<int> tmp_add = cart_state+local_state;
							if(getSum(tmp_add)==5){
								cout << "do sth" << endl;
								vector<int> check_assemble;
								check_assemble = cart_state+local_state;
								local_state = check_assemble;
								//if number of parts in local_state reaches 5
								//simulate move to local state
								for (i = 0; i<5; i++) {
									this_thread::sleep_for(chrono::microseconds(cart_state[i]*move_time[i]));
								}
								//assemble in the local state:
								for (int i = 0; i<5;i++) {
									this_thread::sleep_for(chrono::microseconds(local_state[i]*prodW_assemble_time[i]));
								}

								//also pour the local_state
								cart_state = { 0, 0, 0, 0, 0 };
								local_state = { 0, 0, 0, 0, 0 };

							}
							cout << "Product Worker ID: " << id <<",Iteration " << i << endl;
							cout << "localState" << local_state << endl;
							auto current_tp = chrono::steady_clock::now();
							cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
							cout << "Status: wakeup" << endl;
							cout<<"Pickup Order: "<<printout_pickupOrder<<endl;
							cout << "Buffer State: " << printout_Buffer << endl;
							cout << "Updated pickup order: " << pickup_order << endl;
							cout << "Updated Buffer State: " << Global_buffer << endl << endl;
						}
					}
				}
				auto cycle_end = chrono::steady_clock::now();
				auto cycle_time = cycle_end-cycle_begin;
				auto cycle_elapsed = chrono::duration_cast<chrono::microseconds>(cycle_time);
				copy_MaxTimeProduct -= cycle_elapsed.count();

				if (chrono::steady_clock::now()>begin_time+chrono::microseconds(MaxTimeProduct)) {
					//timeout
					//move parts from cart to local_state
					cout << "timeout carts" << cart_state << endl;
					cout << "timeout local" << local_state << endl;
					for (int i = 0; i<5; i++) {
						if (cart_state[i]!=0) {
							int save_ori = cart_state[i];
							local_state[i] += cart_state[i];
							if (local_state[i]>restraints[i]) {
								int overflow = local_state[i];
								local_state[i] = restraints[i];
								cart_state[i] = overflow-restraints[i];
							}
							else{
								cart_state[i] = 0;
							}
							this_thread::sleep_for(chrono::microseconds((save_ori-cart_state[i])*move_time[i]));
						}
					}
					//cart_state = { 0, 0, 0, 0, 0 };
					//check if can perform assemble
					if(is_initial(pickup_order)&&getSum(local_state)==5){
						cout << "entering this branch" << endl;
						for (int i = 0; i<5;i++) {
							this_thread::sleep_for(chrono::microseconds(local_state[i]*prodW_assemble_time[i]));
							local_state[i] = 0;
						}
					}
					cout << "Product Worker ID: " << id <<",Iteration " << i << endl;
					cout << "localState" << local_state << endl;
					auto current_tp = chrono::steady_clock::now();
					cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
					cout << "Status: timeout" << endl;
					cout<<"Pickup Order: "<<printout_pickupOrder<<endl;
					cout << "Buffer State: " << printout_pickupOrder<< endl;
					cout << "Updated pickup order: " << pickup_order << endl;
					cout << "Updated Buffer State: " << Global_buffer << endl << endl;
					break;
				}
			}
//			cout << "breakout" << endl;
//			cout << "Updated pickup order: " << pickup_order << endl;
//			cout << "Updated Buffer State: " << Global_buffer << endl ;
//			cout << "Part Worker ID: " << id <<",Iteration " << i << endl<<endl;

		}
//		auto iteration_end = chrono::system_clock::now();
//		auto iteration_time= iteration_end-iteration_begin;
//		auto iteration_elapsed = chrono::duration_cast<chrono::seconds>(iteration_time);
//		cur_time += iteration_elapsed.count();
//		auto current_tp = chrono::steady_clock::now();
//		cout << "Current time: " << chrono::duration_cast<chrono::microseconds>(current_tp-start_tp).count()<<"microseconds"<< endl;
//		if(is_initial(save_pickupOrder)){
//			cout << "Status: " << status_string[1] <<" from Scratch " << endl;
//		}
//		else{
//			cout << "Status: " << status_string[1] << " from Partial Order " << save_pickupOrder << endl;
//		}
//		if(is_theSame(pickup_order,save_maxDeliver)&&has_left(pickup_order)){
//			//meaning that there is no change after entering the wait_for
//			//so it must be timed out
//			cout << "Status: " << status_string[3] << endl;
//		}
//		else if(!is_theSame(pickup_order,save_maxDeliver)&&has_left(pickup_order)){
//			//two vectors are different, the max_deliver and final pick up
//			cout << "Status: " << status_string[2] << endl;
//		}
//		cout << "Accumulated wait time: " << MaxTimeProduct-copy_MaxTimeProduct << endl;
//		cout << "Product_worker ID: " << id << ", itertaion " << i << endl;
//		cout<<"Pickup Order: "<<printout_pickupOrder<<endl;
//		cout << "Buffer State: " << printout_Buffer << endl;
//		cout<<"Updated pickup order: "<<pickup_order<<endl;
//		cout << "Updated Buffer State: " << Global_buffer << endl<<endl;
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
