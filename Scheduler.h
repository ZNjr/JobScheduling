#pragma once
#include<vector>
#include<string>
#include "Operation.h"

enum SchedulingAlgorithm{ShiftingBottleneck, Genetic};

class Scheduler {
private:
	
	struct OperationIndex {

		enum Type{Forward, Backward	};

		int forward_task;
		int forward_operation;
		int backward_task;
		int backward_operation;
		Type type;
		OperationIndex();
		void add(int task, int operation, Type type);
		bool forward();
		bool backward();
	};

	static const int POPULATION_SIZE = 400;
	static const int CROSSOVER_RATE = 55;
	static const int MUTATION_RATE = 95; //80
	static const int CROSSOVER_PROBABILITY = 90;
	static const int GENERATIONS = 200;

	const int number_of_machines;
	const int number_of_tasks;
	
	int makespan;
	std::vector<std::vector<Operation>> tasks;
	std::vector<std::vector<Operation>> result_schedule;
	
	//ShiftingBottleneck
	std::vector<std::vector<int>> machine_map;
	std::vector<std::vector<OperationIndex>> additional_edges;
	std::vector<std::vector<int>> constrains;
	std::vector<std::vector<int>> operation_release_time;
	std::vector<std::vector<int>> time_to_end_task;

	void allocate_resources();
	void shifting_bottleneck();
	void calc_constrains(int c_max, int machine);
	int calc_basic_values();

	//void set_min_release_time(int task, int operation, int machine, int current_duration);
	//int max_operation_end_time(int task, int operation);
	std::vector<int> min_delay_schedule();	

	int max_delay(std::vector<std::vector<Operation>>& schedule);
	int minimize_delay(int machine, std::vector<std::vector<Operation>> &min_delay_schedule);
	int evol(int machine, std::vector<std::vector<int>> &population, std::vector<std::vector<Operation>>& schedule, std::vector<int>& weights);
	void crossover(std::vector<int>& parent_x, std::vector<int>& parent_y, std::vector<int>& child);
	void mutation(std::vector<int>& chromosome);
	void distribution_weights(std::vector<int> &dalays);
	int population_delay(int machine,std::vector<std::vector<int>> &population,std::vector<std::vector<Operation>> &schedule, std::vector<int>& weights);
	void single_machine_schedule_from_permutation(std::vector<int> &permutation, int machine, std::vector<std::vector<Operation>>& schedule);
	void add_edges(std::vector<std::vector<Operation>>& schedule);
	void add_edge(int start_task,int start_operation, int end_task, int end_operation);
	void update_release_time(int task,int operation);
	void update_time_to_end(int task, int operation);
	
	void calc_makespan();
	int machine_free_time(std::vector<Operation> &machine_schedule, int durationa, int previous_operation_starat, std::vector<Operation>::iterator & iter);
	
public:
	Scheduler(std::vector<std::vector<Operation>> tasks, int number_of_tasks, int number_of_machines);
	int k = 1;
	void schedule(SchedulingAlgorithm type);
	void save_schedule_to_file(std::string path);
};

//23151, 22969