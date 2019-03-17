#include "Scheduler.h"
#include <fstream>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cmath>

//#include <ilcplex\ilocplex.h>

//ILOSTLBEGIN;

inline int min_(int a, int b) {
	return (a < b) ? a : b;
}

Scheduler::Scheduler(std::vector<std::vector<Operation>> tasks, int number_of_tasks, int number_of_machines) :tasks{ tasks }, 
												  number_of_tasks{ number_of_tasks }, number_of_machines{number_of_machines} {}

void Scheduler::schedule(SchedulingAlgorithm type) {
	std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
	switch (type) {
	case SchedulingAlgorithm::ShiftingBottleneck:
		allocate_resources();
		shifting_bottleneck();
		break;
	}
	calc_makespan();
	std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
	std::cout << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "ms" << std::endl;
}
	
int Scheduler::machine_free_time(std::vector<Operation> &machine_schedule, int duration, int previous_operation_end, std::vector<Operation>::iterator & iter) {
	int start = 0;
	int free_time = 0;
	for (int i = 0;i<machine_schedule.size();++i) {
		Operation operation = machine_schedule[i];
		free_time = min_(operation.start - start, operation.start - previous_operation_end);
		if (free_time >= duration) {
			iter = machine_schedule.begin() + i;
			return std::max(start, previous_operation_end);
		}
		start = operation.start + operation.duration;
	}
	iter = machine_schedule.end();
	return std::max(start,previous_operation_end);
}

void Scheduler::save_schedule_to_file(std::string path) {
	std::ofstream file(path);
	if (file.is_open())
		for (const auto & machine : result_schedule) {
			for (const auto& operation : machine)
				file << operation.task << " " << operation.start << " ";
			file << std::endl;
		}
	file << makespan;
	file.close();
}

void Scheduler::calc_makespan()
{
	makespan = 0;
	for (const auto& machine : result_schedule)
		for (const auto& operation : machine)
			if (operation.start + operation.duration > makespan) makespan = operation.start + operation.duration;
}

Scheduler::OperationIndex::OperationIndex() :forward_task{ -1 }, forward_operation{ -1 }, backward_task{ -1 }, backward_operation{-1}, type{ Forward } {}
void Scheduler::OperationIndex::add(int task, int operation, Type type) {
	if (type == Forward) {
		forward_task = task;
		forward_operation = operation;
	}
	else {
		backward_task = task;
		backward_operation = operation;
	}
}
bool Scheduler::OperationIndex::forward() { return ((forward_task != -1) && (type == Forward)); }
bool Scheduler::OperationIndex::backward() { return ((backward_task != -1) && (type == Backward)); }